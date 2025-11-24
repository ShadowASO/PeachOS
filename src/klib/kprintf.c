// kprintf.c
#include "kprintf.h"
#include "../terminal/screen.h"


//
// -------------------------------------------------------------
// Funções auxiliares
// -------------------------------------------------------------
//

// imprime string
static void kputs(const char *s) {
    while (*s) {
        kputchar(*s++);
    }
}

// imprime inteiro não-sinalizado (32 bits), em base 10 ou 16
static void kprint_uint32(uint32_t value, uint32_t base, int uppercase) {
    char buf[32];
    int i = 0;

    if (value == 0) {
        kputchar('0');
        return;
    }

    while (value > 0) {
        uint32_t digit = value % base;
        value /= base;

        if (digit < 10) {
            buf[i++] = '0' + digit;
        } else {
            buf[i++] = (uppercase ? 'A' : 'a') + (digit - 10);
        }
    }

    // imprime invertido
    while (i--) {
        kputchar(buf[i]);
    }
}

// imprime inteiro signed (32 bits)
static void kprint_int32(int32_t value) {
    if (value < 0) {
        kputchar('-');
        value = -value;
    }
    kprint_uint32((uint32_t)value, 10, 0);
}

//
// -------------------------------------------------------------
// Implementação principal tipo printf
// -------------------------------------------------------------
//

void kvprintf(const char *fmt, va_list args) {
    char ch;

    while ((ch = *fmt++)) {

        if (ch != '%') {
            kputchar(ch);
            continue;
        }

        // não temos modificadores de largura/long, ignoramos
        char spec = *fmt++;

        switch (spec) {

        case 'c': {
            char c = (char)va_arg(args, int);
            kputchar(c);
            break;
        }

        case 's': {
            const char *s = va_arg(args, const char *);
            if (!s) s = "(null)";
            kputs(s);
            break;
        }

        case 'd':
        case 'i': {
            int32_t v = va_arg(args, int32_t);
            kprint_int32(v);
            break;
        }

        case 'u': {
            uint32_t v = va_arg(args, uint32_t);
            kprint_uint32(v, 10, 0);
            break;
        }

        case 'x': {
            uint32_t v = va_arg(args, uint32_t);
            kprint_uint32(v, 16, 0);
            break;
        }

        case 'X': {
            uint32_t v = va_arg(args, uint32_t);
            kprint_uint32(v, 16, 1);
            break;
        }

        case 'p': {
            uint32_t p = (uint32_t)va_arg(args, void *);
            kputs("0x");
            for (int i = 7; i >= 0; i--) {
                uint32_t nibble = (p >> (i*4)) & 0xF;
                kputchar(nibble < 10 ? '0'+nibble : 'a'+(nibble-10));
            }
            break;
}

        case '%':
            kputchar('%');
            break;

        default:
            // caractere desconhecido — imprime literalmente
            kputchar('%');
            kputchar(spec);
            break;
        }
    }
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);
}
