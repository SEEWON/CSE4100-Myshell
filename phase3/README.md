# Phase 3
## 구현 내용
### Foreground / Background 실행 로직
1. main() 내에서 Signal handler install
SIGCHLD는 직접 구현한 핸들러로, SIGCHLD / SIGTSTP 등이 발생했을 경우 호출되어 호출한 자식 프로세스에 대한 정보를 업데이트한다. 자식 프로세스가 terminated되었다면, reaping 또한 수행해 준다.
SIGINT, SIGTSTP는 쉘 내에서 발생 시 무시되도록, 거의 아무런 역할을 하지 않는 핸들러이다.

2. eval() 함수 시그널 핸들링 로직 구현
eval()은 수행되자마자, SIGCHLD / SIGINT / SIGTSTP / SIGTTOU 시그널을 Blocking한다(Sigprocmask).
Fork된 자식 프로세스에서 exec()을 통해 덮어씌워져도, Block된 마스크는 상속되기 때문에 유지된다.
다만 signal handler는 상속되지 않고, 실행되는 프로세스의 시그널 핸들러가 동작한다. 가령, a.out 프로그램이 자식 프로세스에서 exec()을 통해 실행된다면 해당 프로세스에 SIGINT 시그널 전달 시 a.out의 시그널 핸들러에 의해 시그널이 막히지 않고 전달된다. a.out 프로그램에서 별도로 시그널 핸들링을 구현하지 않았다면, SIGINT가 전달되어 프로그램이 종료된다.

eval에서 Fork되어 수행되는 자식 프로세스는, 부모 프로세스인 myshell의 프로세스 그룹과 별도의 프로세스 그룹에 할당된다. setpgid(0, 0)을 통해 새로운 pgid가 할당되며, 실제 bash 쉘과 동일하게 각각 별도의 job으로 관리될 수 있도록 구현했다.

부모 프로세스에서는, 실행한 자식 프로세스가 Foreground에서 동작한다면 터미널의 제어권을 자식 프로세스에게 전달한다(tcsetpgrp). 이후 자식 프로세스가 종료되거나, 다른 시그널이 발생하기 전까지 기다린다(Sigsuspend). 만약 자식에게 SIGINT(Ctrl-C), SIGTSTP(Ctrl-Z) 등의 keyboard interrupt가 발생하거나, 자식 프로세스의 수행이 끝난다면, 자식 프로세스에서 부모 프로세스로 SIGCHLD가 전달되며 Sigsuspend가 풀린다. 이후 부모 프로세스로 터미널 제어권을 복원시킨다(tcsetpgrp). 터미널 제어권이 부모 프로세스 그룹에서 다른 프로세스 그룹으로 넘어갔다면, Ctrl-C나 Ctrl-Z 등의 터미널 입력은 모두 현재 제어권을 갖고 있는 프로세스 그룹으로 전달된다. 또, 해당 키보드 입력은 특정 프로세스가 아닌 프로세스 그룹 전체에 전달된다. 앞서 언급했듯 시그널 핸들러는 상속되지 않으므로, Foreground 프로세스에 그대로 전달되고, 그로 인해 SIGCHLD가 발생하면 부모 프로세스에서는 Reaping 및 터미널 제어권 복원을 수행한다.
추가로, 자식 프로세스가 종료 후 부모 프로세스에게 SIGTTOU 시그널을 보내 블로킹되지 않도록 미리 해당 시그널을 마스킹했다. 

Background에서의 동작은, 자식 프로세스의 수행을 기다리거나 터미널 제어권 수정 없이 eval() 문을 종료시켜 방해 없이 동작하도록 한다. 다만 시그널이 전달되거나 종료되면, 마찬가지로 부모 프로세스로 SIGCHLD를 발생시켜 reaping될 것이다.

3. SIGCHLD handler 구현
sigchld_handler는, 호출 시 waitpid를 통해 종료된 자식 프로세스를 reaping한다.
sigchld_handler가 동작하는 동안은, 새로운 SIGCHLD가 발생하지 않는다. (Sigprocmask로 SIGCHLD 시그널에 대한 blocking, 핸들러 종료 직전 unblock)
jobs[] 구조체 배열에서 수행이 완료된 자식 프로세스에 대한 정보를 업데이트한다.

### fg, bg, kill 로직
1. fg, bg, kill은 모두 수행 중인 프로세스에 적절한 Signal을 보내 진행된다.
멈춰 있는 프로세스를 실행하기 위해서는 SIGCONT 시그널을, 종료를 위해서는 SIGKILL 시그널을 활용했다. fg 명령 실행 시 Background의 프로세스에게 다시 터미널 제어권을 전달하고 종료 시까지 기다려야 하기 때문에, eval()에서와 동일한 로직을 수행한다. 자식 프로세스의 종료 시까지 기다리는 로직은 부모 프로세스에서만 진행할 수 있기 때문에, fork가 일어나지 않고 부모 프로세스에서 그대로 로직을 수행하는 builtin_command() 내에서 실행한다. 

2. jobs[] 구조체 배열 내에 저장해 두었던 정보를 활용해, %1 - %2 등으로 들어오는 number을 파싱해 적절한 프로세스에 시그널을 전달한다.
