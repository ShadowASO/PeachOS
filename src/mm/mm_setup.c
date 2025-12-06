#include "mm_setup.h"

#include "./mm/bootmem.h"
#include "./mm/pmm.h"
#include "./mm/kheap.h"

void memory_setup(e820_address_t *e820_address) {

    //Identifica e mapeia a memória física disponível
    bootmem_init(e820_address);

    //Cria o bitmap para controlar memória física
    pmm_init(get_kernel_end_vmm(),get_memory_size());

    //Inicializa a kheap
    uintptr_t heap_region_start = pmm_bitmap_end_addr();
    size_t    heap_region_size  = MB_SIZE; // ex: alguns MiB
    size_t    heap_initial_size = MB_SIZE;         // ex: 1 MiB inicial

    kheap_init((uint32_t)heap_region_start,
           (uint32_t)heap_region_size,
           (uint32_t)heap_initial_size);



}