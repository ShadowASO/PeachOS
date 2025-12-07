#include "mm_setup.h"
#include "./klib/kprintf.h"
#include "./mm/bootmem.h"
#include "./mm/pmm.h"
#include "./mm/kheap.h"
#include "./page/paging.h"
#include "./../idt/idt.h"
#include "./cpu/cpu.h"

void memory_setup(e820_address_t *e820_address) {
   
    /* Identifica e mapeia a memória física disponível */
    bootmem_init(e820_address);
    
    /* Captura os limites da memória conhecida */
    uintptr_t k_phys_start = (uintptr_t)get_kernel_ini_phys();
    uintptr_t k_phys_end   = (uintptr_t)get_kernel_end_phys();
    size_t memory_size=get_memory_size();

    kprintf("\n\nkernel start=0x%x", k_phys_start);  
    kprintf("\nkernel end=0x%x", k_phys_end);  

    
    //Inicializo o boot_early
    size_t    boot_early_size = MB_SIZE*2; // 3 MiB só para estruturas
    boot_early_init(k_phys_end, boot_early_size);

    /* BITMAP: Criando o BITMAP da memória */

    //Padding para alinhar e  segurança    
    boot_early_kalloc(PAGE_SIZE, PAGE_SIZE );   
    size_t bitmap_size=pmm_calc_bitmap_size_bytes(memory_size);
    void *pmm_bitmap       = boot_early_kalloc( bitmap_size,   4096 );       
    uint16_t pagging_size=127;
    /* Inicializa */
    pmm_init(pmm_bitmap,memory_size);

    /* PAGGING: Inicia o pagging */

    paging_init(pagging_size, k_phys_start, k_phys_end, 0xC0000000);
   
    /* KHEAP: Inicializa a kheap     */

    size_t    heap_region_size  = MB_SIZE *1; // ex: alguns MiB
    size_t    heap_initial_size = MB_SIZE *1;         // ex: 1 MiB inicial
    uintptr_t heap_region_start = (uintptr_t)boot_early_kalloc( heap_initial_size,   PAGE_SIZE ); 

    kprintf("\n\nheap_initial_size=0x%x", heap_initial_size);    
    kprintf("\nheap_region_start=0x%x", heap_region_start);    
    
    kheap_init((uint32_t)heap_region_start,
           (uint32_t)heap_region_size,
           (uint32_t)heap_initial_size);

    
    uintptr_t boot_early_end   = boot_early_phys_end();
    uintptr_t region_size   = boot_early_end - k_phys_start;

    kprintf("\n\nkheap end=0x%x", boot_early_end);    
    kprintf("\nkreap_size=0x%x", region_size);   
    kprintf("\nboot_early_used =%d", boot_early_used()); 
    
    pmm_mark_region_used(k_phys_start, boot_early_end - k_phys_start);

}