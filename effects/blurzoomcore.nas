;
; EffecTV - Realtime Digital Video Effector
; Copyright (C) 2001 FUKUCHI Kentarou
;
; blurzoomcore.nas : blur and zooming module
;

BITS 32

GLOBAL blurzoomcore
EXTERN blurzoombuf
EXTERN blurzoomx
EXTERN blurzoomy

SECTION .text

blurzoomcore:
	push ebp
	push esi
	push edi
	push ebx

;blur

	xor eax, eax
	mov ebp, 238
	mov esi, -320
	mov edi,  320
	mov ebx, dword [blurzoombuf]
	add ebx, 321
	mov edx, ebx
	add edx, 320*240
.byloop:
	mov cl, 53
align 4
.bxloop:
%macro PIXBLUR 0
	mov al, [ebx+esi]
	add al, [ebx-1]
	add al, [ebx+1]
	add al, [ebx+edi]
	inc ebx
	shr al, 2
	sub al, 1
	adc al, ah
	mov [edx], al
	inc edx
%endmacro

%rep 6
	PIXBLUR
%endrep
	dec cl
	jnz near .bxloop

	add ebx, 2
	add edx, 2
	dec ebp
	jnz near .byloop

; zooming

	mov edi, dword [blurzoombuf]
	mov esi, edi
	add esi, 320*240
	mov eax,  blurzoomy
	mov dword [zoomyptr], eax
	mov ch, 240
align 4
.yloop:
	mov eax, dword [zoomyptr]
	mov cl, 10
	add esi, [eax]
	add eax, 4
	xor ebx, ebx
	mov dword [zoomyptr], eax
	mov edx, blurzoomx
align 4
.xloop:
	mov ebp, [edx]
%macro MOV4PIX 0
	sal ebp ,1
	adc esi, ebx
	mov al, [esi]
	sal ebp, 1
	adc esi, ebx
	mov ah, [esi]
	rol eax, 16
	sal ebp, 1
	adc esi, ebx
	mov al, [esi]
	sal ebp, 1
	adc esi, ebx
	mov ah, [esi]
	rol eax, 16
	mov [edi], eax
	add edi, 4
%endmacro

%rep 8
	MOV4PIX
%endrep

	add edx, 4
	dec cl
	jnz near .xloop

	dec ch
	jnz near .yloop

	pop ebx
	pop edi
	pop esi
	pop ebp
	ret

SECTION .bss

zoomyptr	resd 1

