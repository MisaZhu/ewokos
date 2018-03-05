#include <syscall.h>

int syscall0(int code)
{
	int result;

	__asm__ volatile("ldr r0, %0" : : "m" (code));
	__asm__ volatile("swi #0");
	__asm__ volatile("str r0, %0" : "=m" (result));
	
	return result;
}

int syscall1(int code, int arg0)
{
	int result;

	__asm__ volatile("ldr r0, %0" : : "m" (code));
	__asm__ volatile("ldr r1, %0" : : "m" (arg0));
	__asm__ volatile("swi #0");
	__asm__ volatile("str r0, %0" : "=m" (result));
	
	return result;
}

int syscall2(int code, int arg0, int arg1)
{
	int result;

	__asm__ volatile("ldr r0, %0" : : "m" (code));
	__asm__ volatile("ldr r1, %0" : : "m" (arg0));
	__asm__ volatile("ldr r2, %0" : : "m" (arg1));
	__asm__ volatile("swi #0");
	__asm__ volatile("str r0, %0" : "=m" (result));
	
	return result;
}

int syscall3(int code, int arg0, int arg1, int arg2)
{
	int result;

	__asm__ volatile("ldr r0, %0" : : "m" (code));
	__asm__ volatile("ldr r1, %0" : : "m" (arg0));
	__asm__ volatile("ldr r2, %0" : : "m" (arg1));
	__asm__ volatile("ldr r3, %0" : : "m" (arg2));
	__asm__ volatile("swi #0");
	__asm__ volatile("str r0, %0" : "=m" (result));
	
	return result;
}

