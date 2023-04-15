# Phase 3
## 구현 내용
### 구현 로직
1. main() 내에서 SIGCHLD, SIGINT handler install

2. eval() 함수 시그널 핸들링 로직 구현
eval()은 수행되자마자, SIGCHLD 시그널을 Blocking한다. (Sigprocmask)

Foreground에서 동작하는 job 수행 시, child 프로세스가 작업을 수행하고, 부모 프로세스는 자식이 종료 후 SIGCHLD handler에서 reaping될 때까지 기다린다. while문 내에서 Sigsuspend()를 사용해, 자식 프로세스가 종료된 후 SIGCHLD가 발생해 자식이 reaping되도록 한다. 이후 SIGCHLD를 Unblock한다. (Sigprocmask) 이는 Background에서 동작하는 프로세스가 종료 시 reaping될 수 있도록 하기 위함이다.

Background에서 동작하는 job 수행 시, 마찬가지로 child process가 작업을 수행하지만, 부모 프로세스는 바로 SIGCHLD를 Unblock하고 eval()을 종료한다. 자식 프로세스가 종료되면, SIGCHLD가 발생할 수 있기 때문에 handler에서 reaping한다.

부모 프로세스는 SIGCHLD 시그널을 Unblock하기 전에, 수행된 job들에 대한 정보를 jobs[] 구조체 배열에 저장한다.

3. SIGCHLD handler 구현
sigchld_handler는, 호출 시 waitpid를 통해 종료된 자식 프로세스를 reaping한다.
sigchld_handler가 동작하는 동안은, 새로운 SIGCHLD가 발생하지 않는다. (Sigprocmask로 SIGCHLD 시그널에 대한 blocking, 핸들러 종료 직전 unblock)
jobs[] 구조체 배열에서 수행이 완료된 자식 프로세스에 대한 정보를 업데이트한다.