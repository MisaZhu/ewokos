#include <types.h>

static inline uint8_t ascii2char(char c){
    if(c >= '0' && c <= '9')
        c -= '0';
    else if(c >= 'a' && c <= 'f')
        c -= 'a' - 10;
    else if(c >= 'A' && c <= 'F') 
        c -= 'A' - 10;
    else
        c = 0;
    return c;
}

int to_hex(const char *str, uint8_t* hex, int len){
    len = min(len, strlen(str)/2);

    for(int i = 0; i < len; i++){
        hex[i] =ascii2char(str[i*2]) * 16 + ascii2char(str[i*2+1]);
    }
}