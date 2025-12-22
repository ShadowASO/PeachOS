#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

size_t kstrlen(const char *str);
size_t kstrnlen(const char *str, size_t max);
char * kstrcpy(char *dest, const char * src);

//bool isdigit(char c);
int atoi(char c);

static inline unsigned char ktolower(unsigned char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}


/* Compara duas strings C. Retorna 0 se iguais */
static int kstrcmp(const char *str1, const char *str2)
{
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (unsigned char)*str1 - (unsigned char)*str2;
}

/* Compara até n caracteres */
static int kstrncmp(const char *str1, const char *str2, size_t n)
{
    while (n-- && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    if ((int)n < 0) {
        return 0;
    }
    return (unsigned char)*str1 - (unsigned char)*str2;
}
/* Compara até n caracteres, ignorando maiúsculas/minúsculas (ASCII) */

int kstrnicmp(const char *s1, const char *s2, size_t n);

size_t kstrnlen_until(const char *str, size_t max, char terminator);



#endif