#include <types.h>
#include <string.h>

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

static inline char char2ascii(unsigned char c){
    if(c <= 9)
        c += '0';
    else if(c >= 0xa && c <= 0xf)
        c += 'A' - 10;
    return c;
}


int to_hex(const char *str, uint8_t* hex, size_t len){
    len = min(len, strlen(str)/2);

    for(size_t i = 0; i < len; i++){
        hex[i] =ascii2char(str[i*2]) * 16 + ascii2char(str[i*2+1]);
    }
    return len;
}

int to_str(char *str, uint8_t* hex, size_t len){
    size_t i;
    for(i = 0; i < len; i++){
        str[i*2] = char2ascii(hex[i] >> 4 & 0xF);
        str[i*2+1] = char2ascii(hex[i] & 0xF);
    }
    str[i*2] = '\0';
    return i*2;
}