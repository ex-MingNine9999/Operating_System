# Operating System Project

<u>이 코드들은 SSUOS를 기반으로 하여 짜여져 있으며, 해당 OS코드의 저작권은 숭실대학교 OSlab에 있습니다.</u>



## 1번 과제 

- 부트로더에 출력을 하는 기본적인 어셈블리 과제



## 2번 과제

- 부트로더에서 키보드 인터럽트를 처리하고 원하는 커널에 접속할 수 있도록 하는 어셈블리



## 3번 과제

- system call 구현
- 시스템 콜을 이해하고 시스템 콜 함수 보완 및 추가 구현
- syscall_handler라는 인터럽트를 이용하여 시스템 콜 함수를 실행 하도록 설계
- 이미 구현되어있는 open()을 보완하여 옵션에 O_APPEND, O_TRUNC 플래그를 추가
- fcntl()을 새로 구현하여 시스템 콜 함수로 구현
- fcntl()은 F_DUPFD, F_GETFL, F_SETFL 플래그를 구현



## 4번 과제 

- SSUOS의 스케줄러 구현
- preemptive기반 FIFO 스케줄러를 ULE(non-interactive) 스케줄러로 바꾸는 설계
- 0번 deamon프로세스가 schedule()을 통하여 스케줄링 할 프로세스를 선택하여 context switching하도록 설계
- 각각의 run queue는 우선순위 4개를 포함
- run queue에서 우선순위가 높으며 비어있지 않은 리스트 탐색
- 위에서 찾은 리스트에서 실행가능하면서 가장 앞에있는 프로세스를 선택
- 실행 중이던 프로세스가 I/O 요청시, 프로세스의 상태를 block하고 schedule() 호출
- I/O 대기 중인 프로세스는 스케줄링 되지않음



## 5번 과제

- Inverted Page Table 구현
- Level Hash를 구현
- Level Hash를 사용한 역 페이지 테이블 구현
- Level Hash의 resizing은 고려하지 않고 삽입, 삭제만 구현
- Level Hash
  - Top-level 버킷에 삽입
  - Top-level 버킷이 가득 차있다면, 대응하는 Bottom-level 버킷에 삽입
  - 모두 가득 차있다면, Top-level 버킷의 아이템 하나를 이동가능한 Top-level 버킷으로 이동 후 삽입
  - Top-level 버킷의 아이템중 이동가능한 아이템이 없다면, Bottom-level 버킷에서 아이템 이동 후 삽입



## 6번 과제

- Virtual Memory - Page Allocator 구현
- palloc()의 내부를 구현
  - page frame중 하나를 user pool이나 kernel pool에서 할당
  - 성공하면 해당 메모리 블록(page frame)의 실제 메모리 주소 리턴
  - 사용가능한 page frame을 관리하기 위하여 bitmap구조 사용
- free_page()를 구현