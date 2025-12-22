#ifndef _KERNEL_CTYPE_H
#define _KERNEL_CTYPE_H

/*
 * ctype.h - Implementação mínima e segura para kernel
 * Compatível com ASCII (0x00–0x7F)
 *
 * NÃO depende de locale
 * NÃO usa tabelas globais
 * NÃO acessa memória
 *
 * Todas as funções aceitam int conforme padrão libc,
 * mas só valores unsigned char são semanticamente válidos.
 */

/* Converte para minúsculo (ASCII) */
static inline int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}

/* Converte para maiúsculo (ASCII) */
static inline int toupper(int c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

/* Testes de classe de caracteres */

static inline int isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}

static inline int islower(int c)
{
    return (c >= 'a' && c <= 'z');
}

static inline int isalpha(int c)
{
    return isupper(c) || islower(c);
}

static inline int isdigit(int c)
{
    return (c >= '0' && c <= '9');
}

static inline int isalnum(int c)
{
    return isalpha(c) || isdigit(c);
}

static inline int isxdigit(int c)
{
    return isdigit(c) ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

static inline int isspace(int c)
{
    return (c == ' ')  ||
           (c == '\t') ||
           (c == '\n') ||
           (c == '\r') ||
           (c == '\f') ||
           (c == '\v');
}

static inline int isprint(int c)
{
    return (c >= 0x20 && c <= 0x7E);
}

static inline int isgraph(int c)
{
    return (c >= 0x21 && c <= 0x7E);
}

static inline int ispunct(int c)
{
    return isgraph(c) && !isalnum(c);
}

static inline int iscntrl(int c)
{
    return (c < 0x20) || (c == 0x7F);
}

static inline int isascii(int c)
{
    return (c >= 0 && c <= 0x7F);
}

#endif /* _KERNEL_CTYPE_H */
