/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2015, Linaro Limited
 */
#if defined(CFG_UNWIND) && defined(__arm__)
#define UNWIND(...) __VA_ARGS__
#else
#define UNWIND(...)
#endif

/*
 * __value_in_regs lldiv_t __aeabi_ldivmod( long long n, long long d)
 */
.global __aeabi_ldivmod
__aeabi_ldivmod:
	push	{ip, lr}
UNWIND(	.save {ip, lr})
	push	{r0-r3}
UNWIND(	.save {r0-r3})
	mov	r0, sp
	bl	__l_divmod
	pop	{r0-r3}
	pop	{ip, pc}

/*
 * __value_in_regs ulldiv_t __aeabi_uldivmod(
 *		unsigned long long n, unsigned long long d)
 */
.global __aeabi_uldivmod
__aeabi_uldivmod:
	push	{ip, lr}
UNWIND(	.save {ip, lr})
	push	{r0-r3}
UNWIND(	.save {r0-r3})
	mov	r0, sp
	bl	__ul_divmod
	pop	{r0-r3}
	pop	{ip, pc}