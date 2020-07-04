#include <stdlib.h>

float atof(const char* s) {
	(void)s;
#ifdef M_FLOAT
	return 0.0f; //TODO
#else
	return 0.0f;
#endif
}

