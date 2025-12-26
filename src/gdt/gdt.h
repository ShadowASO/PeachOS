#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#include "../kernel.h"

#define GDT_ENTRY_NULL 0x00
#define GDT_KERNEL_CODE 0x9A
#define GDT_KERNEL_DATA 0x92
#define GDT_USER_CODE 0xF8
#define GDT_USER_DATA 0xF2
#define GDT_TSS 0xE9

typedef struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_high_half1;
    uint8_t access;
    uint8_t limit_flags;
    uint8_t base_high_half2;
} PACKED_FIELDS gdt_entry_t;

//Este formato é mais amigável para o ser humano do que
//o formato efetivo de cada entry
typedef struct gdt_segmento
{
   uint32_t base;
   uint32_t limit;
   uint8_t type;
} PACKED_FIELDS gdt_segmento_t;

void gdt_load(gdt_entry_t *gdt, int size);
void gdt_add_entries(gdt_entry_t *gdt, gdt_segmento_t * new_seg, int total);


#endif