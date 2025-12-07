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
#include "./mm/kheap.h"
#include "./mm/mm_setup.h"
#include "./cpu/cpu.h"

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


void kernel_main(void *e820_address) {
    // Agora você já está executando em 0xC0xxxxxx
    video_init();

    //Setup da memória
    memory_setup(e820_address);
    
    //Inicializa a IDT
    idt_init();
    
    //Inicializa o PIC
    setup_pic();

    kprint("\nHello, World!");

    kprintf("\nkernel size=0x%x", get_kernel_size());
    kprintf("\nkernel begin=0x%x", get_kernel_ini_vmm());      
    kprintf("\nkernel end=0x%x", get_kernel_end_vmm());


    uintptr_t end1=(uintptr_t)kmalloc(7000);
    kprintf("\nkamlloc end1=%p", end1);

    uintptr_t end2=(uintptr_t)kmalloc(4096);
    kprintf("\nkamlloc end2=%p", end2);

    uintptr_t pag1=(uintptr_t)kpage_alloc();
    kprintf("\nkpage_alloc=%p", pag1);

    pag1=(uintptr_t)kpages_alloc(3);
    kprintf("\nkpages_alloc=%p", pag1);

    kfree((void *)pag1);

    pag1=(uintptr_t)kpage_alloc();
    kprintf("\nkpage_alloc=%p", pag1);
        

    //Habilita o teclado
    pic_enable_irq(IRQ_KEYBOARD);

    
    _wait();
    

    return;
    
}