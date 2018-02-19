#include <proc.h>

ProcessT *_currentPocess = 0;

int *_getCurrentContext() {
	return _currentPocess->context;
}
