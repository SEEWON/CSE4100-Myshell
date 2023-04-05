# Phase 2
## 구현 내용

main -> eval -> strtok -> while() 안에서 넘김.
A의 출력 B로, B 

A | B | C | D

eval_pipeline 안에서 | 기준으로 파싱
fd 열고, 하나의 명령어를 eval() 시킴
eval() 파라미터로 flag 하나 넘겨서 안에서는 히스토리 저장 안하도록.

리턴해서 eval_pipeline로 돌아왔을 때는 그 결과 그대로 넣도록.
pipeline 개수만큼 while 안에서.
마지막 C | D 남았을때는 표준 출력 stdout으로 출력하도록.


int main() {
    int fd[2];
    pid_t pid;
    char buf[MAXLINE];

    // 파이프 생성
    if (pipe(fd) < 0) {
        perror("pipe error");
        exit(1);
    }

    if ((pid = fork()) < 0) {
        perror("fork error");
        exit(1);
    } else if (pid == 0) {  // 자식 프로세스
        close(fd[0]); // 파이프의 읽기용 디스크립터를 닫음

        // 파이프의 쓰기용 디스크립터를 표준 출력으로 대체
        if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO) {
            perror("dup2 error to stdout");
            exit(1);
        }
        close(fd[1]); // 파이프의 쓰기용 디스크립터를 닫음

        // 프로그램 실행
        char *args[] = {"ls", "-al", NULL};
        execve("/bin/ls", args, NULL);

        perror("execve error");
        exit(1);
    } else { // 부모 프로세스
        close(fd[1]); // 파이프의 쓰기용 디스크립터를 닫음

        // 파이프의 읽기용 디스크립터로부터 결과를 읽어들임
        int nbytes = 0;
        while ((nbytes = read(fd[0], buf, MAXLINE)) > 0) {
            if (write(STDOUT_FILENO, buf, nbytes) != nbytes) {
                perror("write error");
                exit(1);
            }
        }
        if (nbytes < 0) {
            perror("read error");
            exit(1);
        }

        if (waitpid(pid, NULL, 0) < 0) { // 자식 프로세스의 종료를 기다림
            perror("waitpid error");
            exit(1);
        }

        close(fd[0]); // 파이프의 읽기용 디스크립터를 닫음
    }

    return 0;
}