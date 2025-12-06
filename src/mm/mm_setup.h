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

#ifndef MM_SETUP_H
#define MM_SETUP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "bootmem.h"
#include "../cpu/e820.h"

void memory_setup(e820_address_t *e820_address);

#endif