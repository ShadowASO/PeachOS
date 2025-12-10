#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define CMP_IGUAL   0
#define CMP_MENOR   -1
#define CMP_MAIOR   1

void * memset(void *ptr, int c, size_t size);
void memcpy(void *dest, void * src, size_t size);
int memcmp(void * s1, void * s2, size_t count);



#endif