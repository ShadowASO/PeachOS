#include <stddef.h>
#include <stdint.h>

void *kmemset(void *ptr, char value, size_t num) {
    unsigned char *p = ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}
void kmemcpy(void *dest, const void * src, size_t size){
    char *c_dest=(char *)dest;
    char *c_src=(char *)src;
    for (size_t i=0; i < size; i++) {       
        c_dest[i]=c_src[i];
    }
}
/*
return:[ igual=0; menor=-1; maior=+1]
*/
int kmemcmp(const void * s1, const void * s2, size_t count) {
    const char *c1=s1;
    const char *c2=s2;
    while(count-- >0) {
        if(*c1++ != *c2++) {
            return (c1[-1] < c2[-1] ? -1:1);
        }
    }
    return 0;
}