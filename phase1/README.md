# Phase 1
## 구현 내용
### eval()
builtin_command가 아닐 경우, Fork 후 자식 프로세스에서 execve()로 작업 수행.
ls, mkdir, touch 등의 동작은 리눅스 시스템 내 /bin 디렉토리에 저장되어 있으므로, /bin 경로를 생략한 입력 시 추가해 작업 수행.

이후 부모 프로세스에서 waitpid()로 완료된 자식 프로세스에 대한 reaping 진행.

### builtin_command()
exit, cd, history(!!, !#)에 대한 분기 및 해당하는 로직 수행.

cd 입력 시 chdir()로 경로 이동.
"cd", "cd ~", "cd ~/" 입력 시 홈 디렉토리로 이동.
환경 변수 ("$HOME") 경로 입력 시, getenv()로 환경 변수 확인 및 해당 경로로 이동.

history 입력 시, .myshell_history 파일에 저장된 히스토리 출력.
!! 입력 시 마지막으로 수행된 cmdline 다시 수행(eval()).
!# 입력 시 히스토리의 해당 line에 해당하는 cmdline 다시 수행(eval()).

한 cmdline에서 !!/!# 뒤 문자열(옵션 등)입력 시, !!/!#을 해당하는 히스토리 cmdline으로 치환 후 문자열도 그대로 pass해 new cmdline 수행되도록 구현.


### save_history()
eval() 내부에서 호출, 입력되는 cmdline history를 경우에 따라 저장함.

입력된 cmdline이 builtin_command에 해당하는 경우, "!!" / "!#"을 제외하고 항상 호출해 히스토리에 저장.
("!#"의 경우 내부에서 과거 history를 읽어와 해당하는 명령어로 치환한 후, eval()을 재귀 호출.
 이 때 history 갱신)

입력된 cmdline이 builtin_command에 해당하지 않는 경우 항상 호출, 히스토리 저장.

마지막으로 저장된 history cmdline과 동일할 경우, 중복 저장하지 않음.

## Flowchart
![fc1](https://github.com/SEEWON/Myshell/assets/50395394/8133d6be-ecea-43eb-a890-90d9eee6a8f7)