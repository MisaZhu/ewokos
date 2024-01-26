#include <ewoksys/cdefs.h>


#define	op_t	unsigned long int
#define OPSIZ	(sizeof(op_t))
typedef unsigned char byte;
#define	reg_char	char

#define MERGE(w0, sh_1, w1, sh_2) (((w0) >> (sh_1)) | ((w1) << (sh_2)))

#define BYTE_COPY_FWD(dst_bp, src_bp, nbytes)				      \
  do									      \
    {									      \
      size_t __nbytes = (nbytes);					      \
      while (__nbytes > 0)						      \
	{								      \
	  byte __x = ((byte *) src_bp)[0];				      \
	  src_bp += 1;							      \
	  __nbytes -= 1;						      \
	  ((byte *) dst_bp)[0] = __x;					      \
	  dst_bp += 1;							      \
	}								      \
    } while (0)

#define BYTE_COPY_BWD(dst_ep, src_ep, nbytes)				      \
  do									      \
    {									      \
      size_t __nbytes = (nbytes);					      \
      while (__nbytes > 0)						      \
	{								      \
	  byte __x;							      \
	  src_ep -= 1;							      \
	  __x = ((byte *) src_ep)[0];					      \
	  dst_ep -= 1;							      \
	  __nbytes -= 1;						      \
	  ((byte *) dst_ep)[0] = __x;					      \
	}								      \
    } while (0)

#define WORD_COPY_FWD(dst_bp, src_bp, nbytes_left, nbytes)		      \
  do									      \
    {									      \
      if (src_bp % OPSIZ == 0)						      \
	_wordcopy_fwd_aligned (dst_bp, src_bp, (nbytes) / OPSIZ);	      \
      else								      \
	_wordcopy_fwd_dest_aligned (dst_bp, src_bp, (nbytes) / OPSIZ);	      \
      src_bp += (nbytes) & -OPSIZ;					      \
      dst_bp += (nbytes) & -OPSIZ;					      \
      (nbytes_left) = (nbytes) % OPSIZ;					      \
    } while (0)

#define WORD_COPY_BWD(dst_ep, src_ep, nbytes_left, nbytes)		      \
  do									      \
    {									      \
      if (src_ep % OPSIZ == 0)						      \
	_wordcopy_bwd_aligned (dst_ep, src_ep, (nbytes) / OPSIZ);	      \
      else								      \
	_wordcopy_bwd_dest_aligned (dst_ep, src_ep, (nbytes) / OPSIZ);	      \
      src_ep -= (nbytes) & -OPSIZ;					      \
      dst_ep -= (nbytes) & -OPSIZ;					      \
      (nbytes_left) = (nbytes) % OPSIZ;					      \
    } while (0)


#define	OP_T_THRES	16
