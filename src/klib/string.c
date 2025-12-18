#include "string.h"

size_t kstrlen(const char *str) {
    size_t n=0;
    while (*(str+n)) {
        n++;
    }
    return n;
}

size_t kstrnlen(const char *str, size_t max){
    size_t i=0;
    for(;i<max; i++) {
        if(str[i]==0)
            break;
    }
    return i;
}

char * kstrcpy(char *dest, const char * src) {
    char *rsp=dest;
    while(*src != 0) {
        *dest = *src;
        src += 1;
        dest += 1;
    }
    *dest = 0x00;
    return rsp;
}

bool isdigit(char c){
    return (c >= 48 && c <= 57);
}
int atoi(char c) {
    return (c - 48);
}


