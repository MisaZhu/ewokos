#include <sys/basic_math.h>

inline uint32_t div_u32(uint32_t v, uint32_t by) {
	if(by == 0)
		return 0;
	
	switch(by) {
	case 1<<1:
		return v >> 1;
	case 1<<2:
		return v >> 2;
	case 1<<3:
		return v >> 3;
	case 1<<4:
		return v >> 4;
	case 1<<5:
		return v >> 5;
	case 1<<6:
		return v >> 6;
	case 1<<7:
		return v >> 7;
	case 1<<8:
		return v >> 8;
	case 1<<9:
		return v >> 9;
	case 1<<10:
		return v >> 10;
	case 1<<11:
		return v >> 11;
	case 1<<12:
		return v >> 12;
	case 1<<13:
		return v >> 13;
	case 1<<14:
		return v >> 14;
	case 1<<15:
		return v >> 15;
	case 1<<16:
		return v >> 16;
	case 1<<17:
		return v >> 17;
	case 1<<18:
		return v >> 18;
	case 1<<19:
		return v >> 19;
	case 1<<20:
		return v >> 20;
	case 1<<21:
		return v >> 21;
	case 1<<22:
		return v >> 22;
	case 1<<23:
		return v >> 23;
	case 1<<24:
		return v >> 24;
	case 1<<25:
		return v >> 25;
	case 1<<26:
		return v >> 26;
	case 1<<27:
		return v >> 27;
	case 1<<28:
		return v >> 28;
	case 1<<29:
		return v >> 29;
	case 1<<30:
		return v >> 30;
	}

	uint32_t ret = 0;
	uint32_t cmp = by;
	while(cmp <= v) {
		cmp += by;
		ret++;
	}
	return ret;
}

inline uint32_t mod_u32(uint32_t v, uint32_t by) {
	uint32_t div = div_u32(v, by);
	return v - (div*by);
}

inline uint32_t abs32(int32_t v) {
	if(v < 0)
		return (-v);
	return v;
}
