.intel_syntax noprefix
.section .text

.global syscall3_raw
syscall3_raw:
	int 0x80
	mov rax, rdi
	ret
