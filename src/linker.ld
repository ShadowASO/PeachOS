/* ============================================================
   Linker script para Kernel High-Half
   VMA = 0xC0000000 +
   PMA = 0x00100000 +
   Binário carregado pelo bootloader em 1 MiB
   ============================================================ */

/* OUTPUT_FORMAT(binary)
ENTRY(_kernel_start) */
OUTPUT_FORMAT(elf32-i386)
ENTRY(_kernel_start)


/* ----------------------------------------------------
   Base física e base virtual
   ---------------------------------------------------- */
KERNEL_PHYS_BASE = 0x00100000;      /* Bootloader carrega aqui */
/* Kernel executa aqui     */
KERNEL_VIRT_BASE = 0xC0000000;    
/* KERNEL_VIRT_BASE = 0x00000000;       */
PAGE_SIZE        = 4096;

KERNEL_OFFSET =  KERNEL_VIRT_BASE - KERNEL_PHYS_BASE;


/* ----------------------------------------------------
   Exportações básicas
   ---------------------------------------------------- */
PROVIDE(_kernel_phys_base  = KERNEL_PHYS_BASE);
PROVIDE(_kernel_virt_base  = KERNEL_VIRT_BASE);
PROVIDE(_kernel_high_offset = KERNEL_OFFSET);


SECTIONS
{
    /* --------------------------------------------------------
       Coloca o kernel em VMA alto
       -------------------------------------------------------- */
    /* . = KERNEL_VMM; */
    . = KERNEL_VIRT_BASE;
    . = ALIGN(PAGE_SIZE);




    _kernel_ini_vmm = .;
    _kernel_ini_phys = . - KERNEL_OFFSET;

    /* --------------------------------------------------------
       .text
       Código do kernel
       -------------------------------------------------------- */
    .text ALIGN(PAGE_SIZE) : AT(ADDR(.text) - KERNEL_OFFSET)
    {
        *(.text)
        *(.text.*)
        
    }
    /* --------------------------------------------------------
       .rodata
       -------------------------------------------------------- */
    .rodata ALIGN(PAGE_SIZE) : AT(ADDR(.rodata) - KERNEL_OFFSET)
    {
        *(.rodata)
        *(.rodata.*)
    }

   
    /* --------------------------------------------------------
       .data
       -------------------------------------------------------- */
    .data ALIGN(PAGE_SIZE) : AT(ADDR(.data) - KERNEL_OFFSET)
    {
        *(.data)
        *(.data.*)
    }
    /* --------------------------------------------------------
       .bss
       -------------------------------------------------------- */
    .bss ALIGN(PAGE_SIZE) : AT(ADDR(.bss) - KERNEL_OFFSET)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
    
    /* --------------------------------------------------------
       Page Tables (com físico real!)
       -------------------------------------------------------- */
    .pagetables ALIGN(PAGE_SIZE) : AT(ADDR(.pagetables) - KERNEL_OFFSET)
    {
        /* Símbolos exportados */
        page_directory = .;
        . += PAGE_SIZE;

        page_table_identity = .;
        . += PAGE_SIZE;

        page_table_high = .;
        . += PAGE_SIZE;
    }
    /* --------------------------------------------------------
       Stack (com PMA real, não NOLOAD!)
       -------------------------------------------------------- */
    .stack ALIGN(PAGE_SIZE) : AT(ADDR(.stack) - KERNEL_OFFSET)
    {
        _kernel_stack_bottom = .;
        . += (PAGE_SIZE * 4);
        _kernel_stack_top = .;
    }

    /DISCARD/ :
   {
    *(.eh_frame .eh_frame.*)
    *(.eh_frame_hdr)
    *(.gcc_except_table .gcc_except_table.*)
    *(.comment)
    *(.note .note.*)
   }

   
    . = ALIGN(PAGE_SIZE);
    . += PAGE_SIZE;
    _kernel_end_vmm = .;
}

/* ------------------------------------------------------------
   Exportações finais
   ------------------------------------------------------------ */
/* Exportações finais */
PROVIDE(_kernel_stack_bottom = _kernel_stack_bottom);
PROVIDE(_kernel_stack_top    = _kernel_stack_top);
PROVIDE(_kernel_stack_top_phys = _kernel_stack_top - KERNEL_OFFSET);

PROVIDE(page_directory_phys      = page_directory   - KERNEL_OFFSET);
PROVIDE(page_table_identity_phys = page_table_identity - KERNEL_OFFSET);
PROVIDE(page_table_high_phys     = page_table_high     - KERNEL_OFFSET);

/* Limites do Kernel */
PROVIDE(_kernel_ini_phys     = _kernel_ini_vmm  - KERNEL_OFFSET);
PROVIDE(_kernel_ini_vmm     = _kernel_ini_vmm );


PROVIDE(_kernel_end_phys     = _kernel_end_vmm  - KERNEL_OFFSET);
PROVIDE(_kernel_end_vmm     = _kernel_end_vmm );
