#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define CMP_IGUAL   0
#define CMP_MENOR   -1
#define CMP_MAIOR   1

void * kmemset(void *ptr, char c, size_t size);
void kmemcpy(void *dest, const void * src, size_t size);
int kmemcmp(const void * s1, const void * s2, size_t count);



#endif