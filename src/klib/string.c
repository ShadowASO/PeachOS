#include <stddef.h>
#include <stdint.h>

size_t kstrlen(const char *str) {
    size_t n=0;
    while (*(str+n)) {
        n++;
    }
    return n;
}

