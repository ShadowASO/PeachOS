#include <stddef.h>
#include <stdint.h>

void *memset(void *ptr, int value, size_t num) {
    unsigned char *p = ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}
void memcpy(void *dest, void * src, size_t size){
    char *c_dest=(char *)dest;
    char *c_src=(char *)src;
    for (size_t i=0; i < size; i++) {       
        c_dest[i]=c_src[i];
    }
}