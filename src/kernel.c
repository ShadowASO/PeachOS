#include "kernel.h"
#include "./terminal/screen.h"
#include "./klib/string.h"
#include "./terminal/kprint.h"
#include "./pic/pic.h"
#include "./io/io.h"
#include "./idt/irq.h"
#include "./idt/idt.h"
#include "./klib/kprintf.h"
#include "./cpu/e820.h"
#include "./mm/bootmem.h"
#include "./mm/pmm.h"

extern uint32_t _kernel_phys_base;
extern uint32_t _kernel_virt_base;



void kernel_main(void *e820_address) {
    // Agora você já está executando em 0xC0xxxxxx
    video_init();
    //Identifica e mapeia a memória física disponível
    bootmem_init(e820_address);

    pmm_init(get_kernel_end_vmm(),get_memory_size());

    //Inicializa a IDT
    idt_init();
    
    setup_pic();
    kprint("\nHello, World!");

    
    // e820_address_t *E820=e820_address;
    
    // kprintf("\ncount=%d", E820->count);

    // kprintf("\nbuf=%p", E820);
    
    kprintf("\nphys=%p virt=%p\n", &_kernel_phys_base, &_kernel_virt_base);
     kprintf("\nphys=%p virt=%p\n", get_kernel_ini_phys(), get_kernel_ini_vmm());

    //memory_init(e820_address);
    // bootmem_init(e820_address);

    pic_enable_irq(IRQ_KEYBOARD);

    // int a,b;
    // a=1;
    // b=0;        
    // a=a/b;
    
    while (1)
    {
        /* code */
    }
    

    return;
    
}