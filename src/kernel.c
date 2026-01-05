#include "kernel.h"
#include "./terminal/screen.h"
#include "./klib/string.h"
#include "./klib/memory.h"
#include "./pic/pic.h"
#include "./io/io.h"
#include "./idt/irq.h"
#include "./idt/idt.h"
#include "./klib/kprintf.h"
#include "./cpu/e820.h"
#include "./mm/bootmem.h"
#include "./mm/pmm.h"
#include "./mm/kheap.h"
#include "./mm/mm.h"
#include "./cpu/cpu.h"
#include "./mm/page/paging.h"
#include "./drivers/disk/disk.h"
#include "./drivers/disk/streamer.h"
#include "./fs/path.h"
#include "./fs/file.h"
#include "./gdt/gdt.h"
#include "./config.h"
#include "./task/tss.h"

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

    Para a execução, é necessário que o kvm esteja carregado. Existe um pequeno script para
    carregar e descarregar o kvm. O kvm precisa ser descarregado para que o VirtualBox fun-
    cione. Script: kvm-toggle.sh
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

    //kprintf("\nDirectory index=%d", paging_directory_index((void *)pag1));
    //kprintf("\nTable index=%d", paging_table_index((void *)pag1));

}

tss_t tss_real;
gdt_entry_t gdt_real[GDT_SEGMENTS_LEN];
gdt_segmento_t gdt_segs[GDT_SEGMENTS_LEN]= {
    {.base = 0x00, .limit = 0x00, .type = GDT_ENTRY_NULL},                      // NULL Segment
    {.base = 0x00, .limit = 0xffffffff, .type = GDT_KERNEL_CODE},               // Kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = GDT_KERNEL_DATA},               // kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = GDT_USER_CODE},                 // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = GDT_USER_DATA},                 // User data segmente
    {.base = (uint32_t)&tss_real, .limit = sizeof(tss_real), .type = GDT_TSS}   //TSS
};           

char buf[512];

extern uint32_t _kernel_stack_top;

void kernel_main(void *e820_address) {
    // Agora você já está executando em 0xC0xxxxxx
    video_init();

    //Setup da memória
    memory_setup(e820_address);

    //Inicializa a API de acesso ao disco
    
    fs_init();

    // Search and initialize the disks
    disk_search_and_init();

    //Setup de GDT
    kmemset(gdt_real,0, sizeof(gdt_real));
    gdt_add_entries(gdt_real, gdt_segs, GDT_SEGMENTS_LEN);    
    gdt_load(gdt_real, sizeof(gdt_real));
    //******************** */

    //Inicializa a IDT
    idt_init();

    // Setup the TSS
    kmemset(&tss_real, 0x00, sizeof(tss_real));
    tss_real.esp0 = _kernel_stack_top;
    tss_real.ss0 = KERNEL_DATA_SELECTOR;
    tss_load(KERNEL_TSS_SELECTOR);
    //--------------------------------
    
    //Inicializa o PIC
    setup_pic();

    kprintf("\nHello, World!");

    void *p=kmalloc(400);
    kprintf("\np=%p",p);

   

    int fd = fopen("0:/hello.txt","r");
    kprintf("\n\nfd - phys address=%p", virt_to_phys_paging((uintptr_t)&fd));
    if(fd) {
        kprintf("\nO arquivo hello.txt aberto");
        char buf[14];
        fseek(fd,2,SEEK_SET);
        fread(buf,13,1,fd);
        buf[13]=0x00;
        kprintf("\nConteudo: %s", buf);
        struct file_stat s;
        fstat(fd,&s);
        kprintf("\nSize=%d",s.filesize);
        fclose(fd);
        
    }
    int fd2 = fopen("0:/hello2.txt","r");
    if(fd2) {
        // kprintf("\nO arquivo hello2.txt aberto");
        // char buf2[64];
        // //fseek(fd2,2,SEEK_SET);
        // int i=fread(buf2,30,1,fd2);
        // buf2[i]=0x00;
        // kprintf("\nLidos: %d", i);
        // kprintf("\nConteudo: %s", buf2);
        // struct file_stat s2;
        // fstat(fd2,&s2);
        // kprintf("\nSize=%d",s2.filesize);
        fclose(fd2);
        
    }
            

    //Habilita o teclado
    pic_enable_irq(IRQ_KEYBOARD);

    
    kmemset(buf, 0xAA, 512);

         
    _wait();
    
    
}