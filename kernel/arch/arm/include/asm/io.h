/* Author: Lv Zheng <zhenglv@hotmail.com>
 */
#ifndef __IO_ARM_H_INCLUDE__
#define __IO_ARM_H_INCLUDE__

#define __raw_writeb(v,a)	(*(volatile unsigned char  *)(a) = (v))
#define __raw_writew(v,a)	(*(volatile unsigned short *)(a) = (v))
#define __raw_writel(v,a)	(*(volatile unsigned int   *)(a) = (v))

#define __raw_readb(a)		(*(volatile unsigned char  *)(a))
#define __raw_readw(a)		(*(volatile unsigned short *)(a))
#define __raw_readl(a)		(*(volatile unsigned int   *)(a))

/* XXX: Atomic Register Access
 *
 * Be aware that no atomic assurance is made for the following macros.
 * Please use __raw_xxxbwl_atomic versions for atomic register access.
 */
#define __raw_setb(v,a)					\
	do {						\
		unsigned char __v = __raw_readb(a);	\
		__v |= (v);				\
		__raw_writeb(__v, (a));			\
	} while (0)
#define __raw_setw(v, a)				\
	do {						\
		unsigned short __v = __raw_readw(a);	\
		__v |= (v);				\
		__raw_writew(__v, (a));			\
	} while (0)
#define __raw_setl(v,a)					\
	do {						\
		unsigned long __v = __raw_readl(a);	\
		__v |= (v);				\
		__raw_writel(__v, (a));			\
	} while (0)
#define __raw_clearb(v,a)				\
	do {						\
		unsigned char __v = __raw_readb(a);	\
		__v &= ~(v);				\
		__raw_writeb(__v, (a));			\
	} while(0)
#define __raw_clearw(v,a)				\
	do {						\
		unsigned short __v = __raw_readw(a);	\
		__v &= ~(v);				\
		__raw_writew(__v, (a));			\
	} while(0)
#define __raw_clearl(v,a)				\
	do {						\
		unsigned long __v = __raw_readl(a);	\
		__v &= ~(v);				\
		__raw_writel(__v, (a));			\
	} while (0)
#define __raw_writeb_mask(v,m,a)			\
	do {						\
		unsigned char __v = __raw_readb(a);	\
		__v &= ~(m);				\
		__v |= (v);				\
		__raw_writeb(__v, (a));			\
	} while (0)
#define __raw_writew_mask(v,m,a)			\
	do {						\
		unsigned short __v = __raw_readw(a);	\
		__v &= ~(m);				\
		__v |= (v);				\
		__raw_writew(__v, (a));			\
	} while (0)
#define __raw_writel_mask(v,m,a)			\
	do {						\
		unsigned long __v = __raw_readl(a);	\
		__v &= ~(m);				\
		__v |= (v);				\
		__raw_writel(__v, (a));			\
	} while (0)
#define __raw_testb(v, a)				\
	(__raw_readb(a) & (v))
#define __raw_testw(v, a)				\
	(__raw_readw(a) & (v))
#define __raw_testl(v, a)				\
	(__raw_readl(a) & (v))

#ifndef ARCH_HAVE_IO_ATOMIC
#define __raw_setb_atomic(b, a)				\
	do {						\
		unsigned char __v = __raw_readb(a);	\
		__v |= _BV(b);				\
		__raw_writeb(__v, (a));			\
	} while (0)
#define __raw_setw_atomic(b, a)				\
	do {						\
		unsigned short __v = __raw_readw(a);	\
		__v |= _BV(b);				\
		__raw_writew(__v, (a));			\
	} while (0)
#define __raw_setl_atomic(b, a)				\
	do {						\
		unsigned long __v = __raw_readl(a);	\
		__v |= _BV(b);				\
		__raw_writel(__v, (a));			\
	} while (0)
#define __raw_clearb_atomic(b, a)			\
	do {						\
		unsigned char __v = __raw_readb(a);	\
		__v &= ~_BV(b);				\
		__raw_writeb(__v, (a));			\
	} while(0)
#define __raw_clearw_atomic(b, a)			\
	do {						\
		unsigned short __v = __raw_readw(a);	\
		__v &= ~_BV(b);				\
		__raw_writew(__v, (a));			\
	} while(0)
#define __raw_clearl_atomic(b, a)			\
	do {						\
		unsigned long __v = __raw_readl(a);	\
		__v &= ~_BV(b);				\
		__raw_writel(__v, (a));			\
	} while(0)
#define __raw_testb_atomic(b, a)			\
	((__raw_readb(a) >> b) & 0x01)
#define __raw_testw_atomic(b, a)			\
	((__raw_readw(a) >> b) & 0x01)
#define __raw_testl_atomic(b, a)			\
	((__raw_readl(a) >> b) & 0x01)
#endif

#endif /* __IO_ARM_H_INCLUDE__*/
