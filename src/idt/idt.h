#ifndef IDT_H
#define IDT_H

#include <stddef.h>
#include <stdint.h>

typedef struct 
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attrib;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct 
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_register_t;


#define IDT_ENTRY_LEN 256   // 256 entradas
#define IDT_ENTRY_SIZE 8    // 8 bytes


/* Descriptor type*/
#define GATE_PDPL 0x80 /*Present | System - kernel mode */

//Type gates
#define TYPE_INTE (0xE | GATE_PDPL)
#define TYPE_TRAP (0xF | GATE_PDPL)



extern void idt_load(idt_register_t * ptr);
void idt_entry_set(size_t vector, void * func_address, int type);
void setup_idt(void);
void idt_init();

// Habilitar / desabilitar interrupções da CPU
static inline void enable_interrupts(void) {
    __asm__ __volatile__("sti");
}

static inline void disable_interrupts(void) {
    __asm__ __volatile__("cli");
}



#endif
