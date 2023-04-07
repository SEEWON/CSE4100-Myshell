# Phase 2
## 구현 내용
### 파이프 구현 로직
main -> eval_pipeline -> while(eval())

1. eval_pipeline에서는 파이프로 구분된 command의 개수를 센다.
2. 가장 먼저 실행되는 command와, 가장 마지막에 실행되는 command를 제외한 command들은 while 문 안에서 반복해 수행된다.
3. command와 command 간에는 pipeline을 통해 출력을 입력으로 보내 준다.
4. file descripter는 fd1, fd2 두 개를 사용한다. 파이프는 한번 close()된 후에는 다시 사용하지 않으므로, fd1과 fd2는 동적으로 할당하고 해제하며 관리한다.

5. 가장 먼저 실행되는 command
실행하고, 그 output을 fd1[1]에 쓴다.

6. while() 문 안에서 순차적으로 처리되는 command(들)
앞서 출력되는 fd1[1]을 읽어와 fd2[0] 입력으로 실행하고, 그 output을 fd2[1] 출력으로 내보낸다.
fd1을 사용하는 파이프는 이미 한번 사용했으므로(close했음), free하고 새로운 fd1을 할당한다.
현재 command가 내보낸 출력이 존재하는 fd2를 fd1으로 복사하고, fd2도 free 후 새로 할당한다.
이제 while문이 시작될 때와 같은 상태가 되었다. 앞선 명령의 출력이 fd1에 존재하고, fd2는 비어 있는 상태이다.
수행되어야 할 command의 개수만큼 반복한다.

7. 가장 마지막으로 실행되는 command
fd1[0]에 있는 앞선 command의 출력을 읽어 와 stdout으로 출력한다.

8. 위 5, 6, 7번 항목은 모두 eval() 함수를 호출해 수행되나, 각 경우의 파라미터를 다르게 넘겨 구분한다. 5번 항목은 fd1만, 6번 항목은 fd1/fd2를, 7번 항목은 fd1만 넘긴다. 5번 항목은 7번 항목과 구분하기 위해 fd1를 코드 상 fd2자리에서 넘긴다. 7번 항목, 가장 마지막으로 실행되는 command에 대해 eval() 내에서 처리되는 fd는 코드 상 fd2 자리에서 사용되나 실제 eval_pipeline에서는 fd1이다.