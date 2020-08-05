org	0x7c00                      ;0x7c00번지에서 프로그램 시작

[BITS 16]

START:							
	mov 	ax, 0xb800			;컬러 텍스트 메모리 접근을 위한 0xb800
	mov 	es, ax				;메모리 지정
	mov 	ax, 0x00			;ax초기화
	mov 	bx, 0				;bx초기화
	mov 	cx, 80*25*2			;cx를 부트로더 화면 크기만큼
	mov 	dx, 1				;dx초기화 (select위치)

	CLS:
		mov 	[es:bx], ax		;es로부터 bx이동한칸을 0으로 초기화
		add 	bx, 1			;bx를 1증가
		loop	CLS				;cx만큼 반복

	;jmp		BOOT1_LOAD ;BOOT1_LOAD로 점프

PRINT:
	mov 	bx, 0				;bx초기화
	mov 	ah, 0x0F			;글자색을 0x0F(밝은 흰색)
	
	mov 	si, 0				;si를 초기화(index)
	PRINT_1:
		mov 	al, byte[ssuos_1 + si]	;ssuos_1문자열의 si번째 대입
		mov 	[es:bx], ax				;es로부터 bx이동한 칸에 ax대입
		add 	bx, 2					;bx를 2씩증가(출력 화면에서 한칸)
		add 	si, 1					;si를 1증가(문자열 다음글자)
		cmp		al, 0x00				;al값이랑 0을 비교(문자열 마지막 null)
		ja		PRINT_1					;al값이 0보다 크다면 PRINT_1으로 jmp

	add 	bx, 2						;한칸 공백
	mov 	si, 0
	PRINT_2:							;ssuos_2문자열 출력
		mov 	al, byte[ssuos_2 + si]
		mov 	[es:bx], ax
		add 	bx, 2
		add 	si, 1
		cmp 	al, 0x00
		ja 		PRINT_2

	mov 	bx, 80 * 2					;bx의 위치를 다음줄로
	mov 	si, 0
	PRINT_3:							;ssuos_3문자열 출력
		mov 	al, byte[ssuos_3 + si]
		mov 	[es:bx], ax
		add 	bx, 2
		add 	si, 1
		cmp 	al, 0x00
		ja 		PRINT_3

	SEL_POS:							;dx의 값에 따라 select가 출력될 위치 지정
		cmp 	dx, 1
		je 		SEL_1
		cmp		dx, 2
		je		SEL_2
		cmp		dx, 3
		je		SEL_3

		SEL_1:
			mov 	bx, 0
			jmp 	SEL_END

		SEL_2:
			mov 	bx, 26
			jmp 	SEL_END

		SEL_3:
			mov		bx, 80 * 2
			jmp 	SEL_END

		SEL_END:

	mov 	si, 0
	PRINT_SEL:						;select 출력
		mov 	al, byte[select + si]
		cmp 	al, 0x00
		je 		INPUT
		mov 	[es:bx], ax
		add 	bx, 2
		add 	si, 1
		jmp 	PRINT_SEL


INPUT:							;키보드 입력을 받을 함수
	mov 	ah, 0x10			;ah에 키보드 인터럽트를 저장하기 위해 ah설정
	int 	0x16				;키보드 인터럽트

	cmp 	ah, $48				;키보드 위쪽 방향키의 스캔코드 
	je 		INPUT_U
	cmp 	ah, $50				;키보드 아래쪽 방향키의 스캔코드 
	je 		INPUT_D
	cmp 	ah, $4d				;키보드 오른쪽 방향키의 스캔코드 
	je 		INPUT_R
	cmp 	ah, $4b				;키보드 왼쪽 방향키의 스캔코드 
	je 		INPUT_L
	cmp 	ah, $1C				;키보드 엔터키의 스캔코드 
	je 		BOOT1_LOAD

	jmp		INPUT

	INPUT_U:					;방향 처리 3에서 위가 입려되면 1로
		cmp 	dx, 3
		je		GO_1
		jmp		INPUT
	INPUT_D:					;방향 처리 1에서 아래가 입려되면 3으로
		cmp 	dx, 1
		je		GO_3
		jmp		INPUT
	INPUT_R:					;방향 처리 1에서 오른쪽이 입려되면 2으로
		cmp 	dx, 1
		je		GO_2
		jmp		INPUT
	INPUT_L:					;방향 처리 2에서 왼쪽이 입려되면 1로
		cmp 	dx, 2
		je		GO_1
		jmp		INPUT

	GO_1:						;현재 가르키고있는 값을 dx에 저장
		mov		dx, 1
		jmp PRINT
	GO_2:
		mov		dx, 2
		jmp PRINT
	GO_3:
		mov		dx, 3
		jmp PRINT

BOOT1_LOAD:
	mov     ax, 0x0900 
	mov     es, ax
	mov     bx, 0x0
	mov 	si, dx				;현재 dx값을 si에 저장

	mov     ah, 2	
	mov     al, 0x4		
	mov     ch, 0	
	mov     cl, 2	
	mov     dh, 0		
	mov     dl, 0x80

	int     0x13	
	jc      BOOT1_LOAD

	cmp		si, 1				;si값에 따라 해당하는 커널로드 실행
	je		KERNEL_LOAD1
	cmp		si, 2
	je		KERNEL_LOAD2
	cmp		si, 3
	je		KERNEL_LOAD3
	jmp 	END	


KERNEL_LOAD1:
	mov     ax, 0x1000	
	mov     es, ax		
	mov     bx, 0x0		

	mov     ah, 2		
	mov     al, 0x3f	
	mov     ch, 0				;6에 해당하는 CHS값 할당
	mov     cl, 0x6	
	mov     dh, 0     
	mov     dl, 0x80  

	int     0x13
	jc      KERNEL_LOAD1
	jmp		END

KERNEL_LOAD2:
	mov     ax, 0x1000	
	mov     es, ax		
	mov     bx, 0x0		

	mov     ah, 2		
	mov     al, 0x3f	
	mov     ch, 0x09			;10000에 해당하는 CHS값 할당
	mov     cl, 0x2F
	mov     dh, 0x0E     
	mov     dl, 0x80  

	int     0x13
	jc      KERNEL_LOAD2
	jmp		END

KERNEL_LOAD3:
	mov     ax, 0x1000	
	mov     es, ax		
	mov     bx, 0x0		

	mov     ah, 2		
	mov     al, 0x3f	
	mov     ch, 0x0E			;15000에 해당하는 CHS값 할당
	mov     cl, 0x07
	mov     dh, 0x0E  
	mov     dl, 0x80  

	int     0x13
	jc      KERNEL_LOAD3
	jmp		END

END:							;끝을 알리는 함수
	jmp		0x0900:0x0000

select db "[O]",0
ssuos_1 db "[ ] SSUOS_1",0
ssuos_2 db "[ ] SSUOS_2",0
ssuos_3 db "[ ] SSUOS_3",0
ssuos_4 db "[ ] SSUOS_4",0
partition_num : resw 1

times   446-($-$$) db 0x00		

PTE:
partition1 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition2 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition3 db 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x98, 0x3a, 0x00, 0x00, 0x3f, 0x0, 0x00, 0x00
partition4 db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
times 	510-($-$$) db 0x00	;남는 모든 바이트를 0으로 설정
dw	0xaa55					;맨마지막 2byte설정
