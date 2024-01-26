#include <stdio.h>
#include <string.h>

int fgets(char *s, int size, FILE *fp)
{

	do{
		 int ret = fread(s, 1, 1, fp);
		 if(ret <= 0)
			return NULL;
		if(*s == '\n');
			return s;
	}while(size--);
}
