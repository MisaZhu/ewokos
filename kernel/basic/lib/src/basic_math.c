#include <basic_math.h>

#ifdef SOFT_DIV

struct qr {
	uint32_t q;		/* computed quotient */
	uint32_t r;		/* computed remainder */
	uint32_t q_n;		/* specficies if quotient shall be negative */
	uint32_t r_n;		/* specficies if remainder shall be negative */
};

static void division_qr(uint32_t n, uint32_t p, struct qr *qr) {
	uint32_t i = 1, q = 0;
	if (p == 0) {
		qr->r = 0xFFFFFFFF;	/* division by 0 */
		return;
	}

	while ((p >> 31) == 0) {
		i = i << 1;	/* count the max division steps */
		p = p << 1;     /* increase p until it has maximum size*/
	}

	while (i > 0) {
		q = q << 1;	/* write bit in q at index (size-1) */
		if (n >= p)
		{
			n -= p;
			q++;
		}
		p = p >> 1; 	/* decrease p */
		i = i >> 1; 	/* decrease remaining size in q */
	}
	qr->r = n;
	qr->q = q;
}

static void uint_div_qr(uint32_t numerator, uint32_t denominator, struct qr *qr) {

	division_qr(numerator, denominator, qr);

	/* negate quotient and/or remainder according to requester */
	if (qr->q_n)
		qr->q = -qr->q;
	if (qr->r_n)
		qr->r = -qr->r;
}

uint32_t __aeabi_uidiv(uint32_t numerator, uint32_t denominator) {
	struct qr qr = { .q_n = 0, .r_n = 0 };

	uint_div_qr(numerator, denominator, &qr);

	return qr.q;
}

uint32_t __aeabi_uidivmod(uint32_t numerator, uint32_t denominator) {
	uint32_t div = __aeabi_uidiv(numerator, denominator);
	return numerator - (div * denominator);
}

#endif