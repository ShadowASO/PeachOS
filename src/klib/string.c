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

// bool isdigit(char c){
//     return (c >= 48 && c <= 57);
// }

int atoi(char c) {
    return (c - 48);
}


/* Compara até n caracteres, ignorando maiúsculas/minúsculas (ASCII) */
int kstrnicmp(const char *s1, const char *s2, size_t n)
{
    unsigned char c1, c2;

    while (n-- > 0) {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;

        unsigned char lc1 = ktolower(c1);
        unsigned char lc2 = ktolower(c2);

        if (lc1 != lc2)
            return (int)lc1 - (int)lc2;

        if (c1 == '\0')  /* ambos são '\0' aqui */
            return 0;
    }

    return 0;
}

size_t kstrnlen_until(const char *str, size_t max, char terminator)
{
    const char *s = str;

    while (max--) {
        if (*s == '\0' || *s == terminator)
            break;
        s++;
    }

    return (size_t)(s - str);
}



