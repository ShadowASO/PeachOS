// kprintf.h
#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdarg.h>
#include <stdint.h>

// Deve ser implementado pelo kernel (VGA, serial etc.)
//void kputchar(char c);

void kprintf(const char *fmt, ...);
void kvprintf(const char *fmt, va_list args);

#endif
