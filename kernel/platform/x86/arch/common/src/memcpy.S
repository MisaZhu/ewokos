.intel_syntax noprefix
.section .text

.global __memcpy32
__memcpy32:
	mov rax, rdi
	mov rcx, rdx
	shr rcx, 3
	rep movsq
	mov rcx, rdx
	and rcx, 7
	rep movsb
	ret
