#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

size_t kstrlen(const char *str);
size_t kstrnlen(const char *str, size_t max);
char * kstrcpy(char *dest, const char * src);

bool isdigit(char c);
int atoi(char c);

/* Compara duas strings C. Retorna 0 se iguais */
static int kstrcmp(const char *a, const char *b)
{
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

/* Compara até n caracteres */
static int kstrncmp(const char *a, const char *b, size_t n)
{
    while (n-- && *a && (*a == *b)) {
        a++;
        b++;
    }
    if ((int)n < 0) {
        return 0;
    }
    return (unsigned char)*a - (unsigned char)*b;
}


#endif