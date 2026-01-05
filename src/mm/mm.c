#include "mm.h"
#include "../klib/kprintf.h"
#include "../mm/bootmem.h"
#include "../mm/pmm.h"
#include "../mm/kheap.h"
#include "./page/paging.h"
#include "./../idt/idt.h"
#include "../cpu/cpu.h"

void memory_setup(e820_address_t *e820_address) {
   
    /* Identifica e mapeia a memória física disponível e extrai os limites
    físico e virtual do kernel. É o diagnóstico do sistema.
    */
    bootmem_init(e820_address);
        
    /* Captura os limites da memória conhecida */
    uintptr_t k_phys_start = (uintptr_t)get_kernel_ini_phys();
    uintptr_t k_phys_end   = (uintptr_t)get_kernel_end_phys();

    uint64_t memory_size=get_memory_size();
     kprintf("\n\n(*)memory_size = %u",memory_size );    
           
    /*
    ALOCADOR BOOT EARLY - É o alocador utilizado antes de concluirmos a
    configuração do kheap. Define o tamanho máximo de memória disponível 
    para uso/alocação pelo alocador provisório. Deve ser reservado espaço 
    suficiente para criar as estruturas iniciais. Após o kheap_init, 
    o boot_early NÃO PODE SER USADO para alocações de memória.
    */

    size_t boot_early_size = 4 * MB_SIZE;   // ou 16MiB
    boot_early_init(k_phys_end, boot_early_size);


    /* BITMAP: Criando o BITMAP para gerenciar o uso da memória física.
    */
   
    size_t bitmap_size=pmm_calc_bitmap_size_bytes(memory_size);
    kprintf("\nTamanho do bitmap = %u bytes",bitmap_size );
   
    uint32_t *pmm_bitmap = (uint32_t*)boot_early_kalloc(bitmap_size, 4096);
    kprintf("\nEndereco virtual do bitmap = %p",pmm_bitmap );      
      
    /* Inicializa o bitmap  */
    pmm_init(pmm_bitmap,memory_size);
   
    /* PAGGING: Inicia o pagging */       
    
    paging_ctx_t *ctx=get_paging_ctx();
    ctx->alloc_page_aligned = early_alloc_wrapper,  //função que será usada para alocar memória        
        ctx->virt_to_phys       = virt_to_phys_kernel, // identity no bootstrap
        ctx->bootstrap_identity_limit = 64u * 1024u * 1024u, // 64MiB
        ctx->kernel_virt_base   = 0xC0000000u,
        ctx->kernel_phys_start  = k_phys_start,
        ctx->kernel_phys_end    = k_phys_end,
        ctx->kernel_page_flags  = PAGE_RW,
     
    paging_init_minimal(ctx);
    debug_early_init();

     /**
     * Precisamos marcar a área do kernel e das estruturas criadas como utilizadas
     * no bitmap para evitar que sejam requisitadas e sobrepostas.
     */
    uintptr_t boot_early_end   = boot_early_phys_end();    
    pmm_mark_region_used64(k_phys_start, boot_early_end - k_phys_start);//Sem isso gera erro

      
    /* KHEAP: Inicializa a kheap     */

    size_t    heap_region_size  = MB_SIZE *1; // ex: alguns MiB
    size_t    heap_initial_size = MB_SIZE *1;         // ex: 1 MiB inicial
    uintptr_t heap_region_start = (uintptr_t)KHEAP_BASE; 
   
    
    map_heap_initial(ctx, heap_initial_size + 64 * 1024);

    kheap_init((uint32_t)heap_region_start,
           (uint32_t)heap_region_size,
           (uint32_t)heap_initial_size);

    
     
}