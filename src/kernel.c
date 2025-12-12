#include "kernel.h"
#include "./terminal/screen.h"
#include "./klib/string.h"
#include "./klib/memory.h"
//#include "./terminal/kprint.h"
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
#include "./mm/page/paging.h"
#include "./drivers/disk/disk.h"
#include "./drivers/disk/streamer.h"
#include "./fs/path.h"

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
void debug_kernel_main(void) {
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

    kprintf("\nDirectory index=%d", paging_directory_index((void *)pag1));
    kprintf("\nTable index=%d", paging_table_index((void *)pag1));

}
char buf[512];

void kernel_main(void *e820_address) {
    // Agora você já está executando em 0xC0xxxxxx
    video_init();

    //Setup da memória
    memory_setup(e820_address);

    disk_search_and_init();
    
    //Inicializa a IDT
    idt_init();
    
    //Inicializa o PIC
    setup_pic();

    kprintf("\nHello, World!");
            

    //Habilita o teclado
    pic_enable_irq(IRQ_KEYBOARD);

    //for (int i = 0; i < 512; i++) buf[i] = 0xAA;
    kmemset(buf, 0xAA, 512);
    //disk_read_sector(0,1,buf);

    //****************** */
    struct disk_stream * stream = diskstreamer_new(0);
    kprintf("\nstream->disk->type=%d, stream->pos=%d", stream->disk->type, stream->pos);

    int len=disk_read_block(stream->disk,0,1,buf);
    
   kprintf("\nValor de len=%d",len);

    diskstreamer_seek(stream, 0);
    kprintf("\nstream->disk->type=%d, stream->pos=%d", stream->disk->type, stream->pos);
    kmemset(buf, 0xAA, 512);

    
    len=diskstreamer_read(stream, buf,24);
    //disk_read_sector(0,1,&c);
    //disk_read_block(stream->disk,0,1,buf);

    kprintf("\nValor de len=%d",len);
    
    _wait();
    
    
}