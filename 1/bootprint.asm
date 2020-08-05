[BITS 16]
org		0x7c00              ;0x7c00번지에서 프로그램 시작

START:   
mov		ax, 0xb800          ;컬러 텍스트 메모리 접근을 위한 0xb800
mov		es, ax              ;es레지스터에 0xb800대입
mov		ax, 0x00            ;ax 초기화
mov		bx, 0               ;bx 초기화
mov		cx, 80*25*2         ;cx를 부트로더 화면 크기만큼 할당

CLS:
mov		[es:bx], ax         ;es메모리에 bx위치에 ax(null)대입
add		bx, 1               ;bx 1증가
loop 	CLS                 ;cx 크기만큼 반복

INIT:
mov 	bx, 0               ;출력을 위해 bx초기화
mov 	ecx, 28             ;ecx값을 출력 할 글자 수 만큼 할당
mov		ah, 0x0F            ;텍스트 색을 0x0F로설정

PRINT:
mov		al, byte[msg+bx]    ;al에 msg에 한 글자 대입
mov 	[es:si], ax         ;si자리에 ax(ah에서 정한 색, al에서 정한 글자) 입력
add 	si, 2               ;si 2증가 (다음위치)
add 	bx, 1               ;bx 1증가 (다음글자)
loop	PRINT               ;ecx 크기만큼 반복

msg db "Hello, Ming._.Nine's World!", 0     ;출력 할 문자열 db

times 510 - ($ - $$) db 0x00                ;출력하고 남은 공간을 전부 0(null)으로 채움
dw		0xAA55
