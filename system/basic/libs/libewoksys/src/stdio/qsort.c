#include <unistd.h>

typedef int (*__compar_d_fn_t) (const void *, const void *, void *);
typedef int (*__compar_fn_t) (const void *, const void *);
void qsort_r(void  *base,
           size_t nel,
           size_t width,
           __compar_d_fn_t comp,
		   void *arg)
{
	size_t wgap, i, j, k;
	char tmp;

	if ((nel > 1) && (width > 0)) {
		wgap = 0;
		do {
			wgap = 3 * wgap + 1;
		} while (wgap < (nel-1)/3);
		/* From the above, we know that either wgap == 1 < nel or */
		/* ((wgap-1)/3 < (int) ((nel-1)/3) <= (nel-1)/3 ==> wgap <  nel. */
		wgap *= width;			/* So this can not overflow if wnel doesn't. */
		nel *= width;			/* Convert nel to 'wnel' */
		do {
			i = wgap;
			do {
				j = i;
				do {
					register char *a;
					register char *b;

					j -= wgap;
					a = j + ((char *)base);
					b = a + wgap;
					if ((*comp)(a, b, arg) <= 0) {
						break;
					}
					k = width;
					do {
						tmp = *a;
						*a++ = *b;
						*b++ = tmp;
					} while (--k);
				} while (j >= wgap);
				i += width;
			} while (i < nel);
			wgap = (wgap - width)/3;
		} while (wgap);
	}
}

void qsort(void  *base,
           size_t nel,
           size_t width,
           __compar_fn_t comp)
{
	return qsort_r (base, nel, width, (__compar_d_fn_t) comp, NULL);
}
