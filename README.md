# Myshell
서강대학교 컴퓨터공학과 전공과목 '멀티코어 프로그래밍(구. 시스템 프로그래밍)'의 첫 번째 과제입니다.

Bash와 같은 Linux shell을 직접 구현해 보는 과제로, 각 Phase별 구현 목표는 아래와 같고, 각 phase의 구현 방법에 대한 세부 설명은 document.docx 문서 및 디렉토리 내의 ReadMe에 기재되어 있습니다.

## Phase1: Building and Testing Your Shell
기본 linux 명령어 수행이 가능한 쉘 만들기.

아래 명령어를 포함한 다양한 linux 기본 명령어 수행이 가능합니다.

```
cd, ls, mkdir, rmdir, touch, cat, echo, history, !!, !# , exit
```

Linux의 fork(), execvp() 등의 system call을 활용해 구현했습니다.

## Phase2: Redirection and Pipe
Linux의 Pipeline 기능 구현하기.

아래와 같이 pipeline을 활용한 명령어 수행이 가능합니다.

```
> ls | grep f ilename
> cat f ilename | less
> cat f ilename | grep -i "abc" | sort -r
```

Linux의 pipe(), dup2() 등의 system call을 활용해 구현했습니다.

## Phase3: Run Processes in Background
Linux의 job control 기능 구현하기.

명령어를 통해 Foreground와 Background에서 프로세스 실행 및 이동, kill이 가능하도록 구현했습니다.

아래와 같은 job control 명령어와, Ctrl+Z 등 Keyboard interrupt를 통한 signal 전달이 가능합니다.

```
> jobs        # 수행 중인 job 확인
> ./a.out &   # Background에서 job 수행
> fg <job>    # 해당 job을 foreground로 이동
> bg <job>    # 해당 job을 background로 이동
> kill <job>  # 해당 job에 terminate signal 전달
```

Linux의 sigsuspend(), tcsetpgrp() 등의 system call과, SIGCHLD / SIGINT / SIGTSTP / SIGTTOU 등의 signal을 적절히 활용해 구현했습니다.

## 실행 방법
```
> git clone https://github.com/SEEWON/Myshell.git
> cd Myshell
> cd phase1   # or phase2, phase3
> make        # compile with make
> ./myshell   # execute shell
```

## Evaluation
프로젝트 점수: 120/120 (만점)

분반 평균 점수: 63.235
