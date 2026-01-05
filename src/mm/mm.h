/*
[ heap_region_start ] ----------------------+
                                           |
                                           v
    +----------------------+  <- region_start (alinhado)
    |  bitmap (u32...)     |
    +----------------------+
    |  heap_alloc_units[]  |
    +----------------------+
    |  padding p/ alinhar  |
    +----------------------+  <- heap_start_addr (início efetivo da heap)
    |  heap útil inicial   |  tamanho = heap_initial_size
    |        ...           |
    +----------------------+  <- heap_end_addr
    |  espaço p/ expandir  |  até heap_max_size
    |        ...           |
    +----------------------+  <- heap_max_addr

*/

#ifndef MM_H
#define MM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "bootmem.h"
#include "../cpu/e820.h"
#include "../klib/panic.h"


/* Medidas de Memória*/

#define BYTE_SIZE 4
#define KB_SIZE 1024ULL
#define MB_SIZE (KB_SIZE * KB_SIZE)
#define GB_SIZE (KB_SIZE * KB_SIZE * KB_SIZE)
#define TB_SIZE (KB_SIZE * KB_SIZE * KB_SIZE * KB_SIZE)


#ifndef PAGE_SIZE
#define PAGE_SIZE 4096u
#endif


static inline bool is_power_of_two(uintptr_t x)
{
    return x && !(x & (x - 1));
}

static inline uintptr_t align_up(uintptr_t value, uintptr_t align)
{
    
    if (!is_power_of_two(align)) {   
        panic("\nErro: alinhamento deve ser potencia de dois");
    }

    return (value + (align - 1)) & ~(align - 1);
}

static inline uintptr_t align_down(uintptr_t value, uintptr_t align)
{
    
    if (!is_power_of_two(align)) {   
        panic("\nErro: alinhamento deve ser potencia de dois");
    }

    return value & ~(align - 1);
}

/* Alinhamento de memória */
#define ALIGN_UP(x, align)   align_up((x), align)
#define ALIGN_DOWN(x, align) align_down((x), align)



void memory_setup(e820_address_t *e820_address);

#endif