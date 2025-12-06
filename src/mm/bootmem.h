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

#ifndef boot_HEAP_H
#define boot_HEAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../cpu/e820.h"

// Alinhamento básico (ajuste se necessário)
#ifndef boot_HEAP_ALIGNMENT
#define boot_HEAP_ALIGNMENT 8
#endif

#define ALIGN_UP(x, align)   (((x) + ((align) - 1)) & ~((align) - 1))

#define INT32_BYTE_SIZE 4

// Inicializa o early allocator em um range [start, start+size)
void boot_heap_init(uint32_t start, uint32_t size);

// Aloca 'size' bytes (sem free real)
void* boot_kmalloc(size_t size);

// Versão zeroada
void* boot_kcalloc(size_t n, size_t size);

// "Libera" (aqui será no-op ou apenas assert de sanidade)
void  boot_kfree(void* ptr);

// Opcional: reseta toda a early heap
void  boot_heap_reset(void);

// Consulta de estado (debug/log)
uint32_t boot_heap_used(void);
uint32_t boot_heap_total(void);
bool     boot_heap_is_initialized(void);

//Memória física

void bootmem_init(e820_address_t *e820_address);
void *get_kernel_ini_vmm(void);
void *get_kernel_end_vmm(void);
uint32_t get_kernel_ini_phys(void);
uint32_t get_kernel_end_phys(void);
size_t get_memory_size(void);
size_t get_kernel_size(void) ;

#endif // boot_HEAP_H