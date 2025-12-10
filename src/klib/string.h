#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

size_t kstrlen(const char *str);
size_t kstrnlen(const char *str, size_t max);
bool isdigit(char c);
int atoi(char c);


#endif