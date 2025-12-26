#ifndef KERNEL_H
#define KERNEL_H

#define PACKED_FIELDS __attribute__((packed)) 

/* GDT - Selector in GDT  */
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define KERNEL_TSS_SELECTOR 0x28



void kernel_main();

#endif