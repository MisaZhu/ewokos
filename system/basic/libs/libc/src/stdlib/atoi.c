#include <stdlib.h>
#include <string.h>

int atoi_base(const char *s, int b) {
	int i, result, x, error;
	for (i = result = error = x = 0; s[i]!='\0'; i++, result += x) {
		if (b==2) {
			if (!(s[i]>47&&s[i]<50)){			//rango
				error = 1;
			}
			else {
				x = s[i] - '0';
				result *= b;
			}
		}
		else if (b==8) {
			if (i==0 && s[i]=='0'){
				x=0;
			}
			else if (!(s[i]>47&&s[i]<56)) {	//rango
				error = 1;
			}
			else {
				x = s[i] - '0';
				result *= b;
			}
		}
		else if (b==10) {
			if (!(s[i]>47&&s[i]<58)) {			//rango
				error = 1;
			}
			else {
				x = s[i] - '0';
				result *= b;
			}
		}
		else if (b==16) {
			if((i==0 && s[i]=='0')||(i==1 && (s[i]=='X'||s[i]=='x'))) {
				x = 0;
			}
			else if (!((s[i]>47 && s[i]<58) || ((s[i]>64 && s[i]<71) || (s[i]>96 && s[i]<103)) )) {		//rango
				error = 1;
			}
			else {
				x = (s[i]>64 && s[i]<71)? s[i]-'7': s[i] - '0';
				x = (s[i]>96 && s[i]<103)? s[i]-'W': x;
				result *= b;
			}
		}
	}

	if (error)
		return 0;
	else
		return result;
}

int atoi(const char *str) {
	return atoi_base(str, 10);
}

