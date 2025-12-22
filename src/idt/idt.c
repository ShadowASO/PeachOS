#include "idt.h"
#include "../klib/memory.h"
#include "../terminal/screen.h"
#include "../terminal/kprint.h"
#include "../kernel.h"
#include "isr.h"


idt_entry_t idt_entries[IDT_ENTRY_LEN];
idt_register_t idt_register;

void idt_entry_set(size_t vector, void * func_address, int type){

    idt_entry_t * entry = &idt_entries[vector];
    entry->offset_low=(uint32_t)func_address & 0x0000ffff;
    entry->selector=KERNEL_CODE_SELECTOR;
    entry->zero = 0x00;    
    entry->type_attrib=type;
    entry->offset_high=(uint32_t) func_address >> 16;

}
void load_default_isr() {
    for (size_t i=0; i < IDT_ENTRY_LEN; i++) {
         idt_entry_set(i, isr_default, TYPE_INTE);
    }

}


void idt_init() {
    kprint("\nInicializando a IDT");
   
    idt_register.limit=sizeof(idt_entries)-1;
    idt_register.base=(uint32_t) idt_entries;
   
    setup_idt();
   
   
    idt_load(&idt_register);

    kprint("\nIDT - Inicializada");

}

void setup_idt(void)
{
	//u8_t id_cpu = cpu_id();

	/* Pego a IDT do BSP no vetor idt_percpu[]*/
	//idt_table = &idt_percpu[id_cpu];
	//memset(idt_table, 0, sizeof(idt_t));
    kmemset(idt_entries,0, sizeof(idt_entries));

	idt_entry_set(0, isr_stub_0, TYPE_TRAP);
    idt_entry_set(1, isr_stub_1, TYPE_TRAP);
    idt_entry_set(2, isr_stub_2, TYPE_TRAP);
    idt_entry_set(3, isr_stub_3, TYPE_TRAP);
    idt_entry_set(4, isr_stub_4, TYPE_TRAP);
    idt_entry_set(5, isr_stub_5, TYPE_TRAP);
    idt_entry_set(6, isr_stub_6, TYPE_TRAP);
    idt_entry_set(7, isr_stub_7, TYPE_TRAP);
    idt_entry_set(8, isr_stub_8, TYPE_TRAP);
    idt_entry_set(9, isr_stub_9, TYPE_TRAP);

    idt_entry_set(10, isr_stub_10, TYPE_TRAP);
    idt_entry_set(11, isr_stub_11, TYPE_TRAP);
    idt_entry_set(12, isr_stub_12, TYPE_TRAP);
    idt_entry_set(13, isr_stub_13, TYPE_TRAP);
    idt_entry_set(14, isr_stub_14, TYPE_TRAP);
    idt_entry_set(15, isr_stub_15, TYPE_TRAP);
    idt_entry_set(16, isr_stub_16, TYPE_TRAP);
    idt_entry_set(17, isr_stub_17, TYPE_TRAP);
    idt_entry_set(18, isr_stub_18, TYPE_TRAP);
    idt_entry_set(19, isr_stub_19, TYPE_TRAP);
    
	idt_entry_set(20, isr_stub_20, TYPE_TRAP);
    idt_entry_set(21, isr_stub_21, TYPE_TRAP);
    idt_entry_set(22, isr_stub_22, TYPE_TRAP);
    idt_entry_set(23, isr_stub_23, TYPE_TRAP);
    idt_entry_set(24, isr_stub_24, TYPE_TRAP);
    idt_entry_set(25, isr_stub_25, TYPE_TRAP);
    idt_entry_set(26, isr_stub_26, TYPE_TRAP);
    idt_entry_set(27, isr_stub_27, TYPE_TRAP);
    idt_entry_set(28, isr_stub_28, TYPE_TRAP);
    idt_entry_set(29, isr_stub_29, TYPE_TRAP);

	idt_entry_set(30, isr_stub_30, TYPE_TRAP);
    idt_entry_set(31, isr_stub_31, TYPE_TRAP);
    idt_entry_set(32, isr_stub_32, TYPE_INTE);
    idt_entry_set(33, isr_stub_33, TYPE_INTE);
    idt_entry_set(34, isr_stub_34, TYPE_INTE);
    idt_entry_set(35, isr_stub_35, TYPE_INTE);
    idt_entry_set(36, isr_stub_36, TYPE_INTE);
    idt_entry_set(37, isr_stub_37, TYPE_INTE);
    idt_entry_set(38, isr_stub_38, TYPE_INTE);
    idt_entry_set(39, isr_stub_39, TYPE_INTE);

    idt_entry_set(40, isr_stub_40, TYPE_INTE);
    idt_entry_set(41, isr_stub_41, TYPE_INTE);
    idt_entry_set(42, isr_stub_42, TYPE_INTE);
    idt_entry_set(43, isr_stub_43, TYPE_INTE);
    idt_entry_set(44, isr_stub_44, TYPE_INTE);
    idt_entry_set(45, isr_stub_45, TYPE_INTE);
    idt_entry_set(46, isr_stub_46, TYPE_INTE);
    idt_entry_set(47, isr_stub_47, TYPE_INTE);
    idt_entry_set(48, isr_stub_48, TYPE_INTE);
    idt_entry_set(49, isr_stub_49, TYPE_INTE);

    idt_entry_set(50, isr_stub_50, TYPE_INTE);
    idt_entry_set(51, isr_stub_51, TYPE_INTE);
    idt_entry_set(52, isr_stub_52, TYPE_INTE);
    idt_entry_set(53, isr_stub_53, TYPE_INTE);
    idt_entry_set(54, isr_stub_54, TYPE_INTE);
    idt_entry_set(55, isr_stub_55, TYPE_INTE);
    idt_entry_set(56, isr_stub_56, TYPE_INTE);
    idt_entry_set(57, isr_stub_57, TYPE_INTE);
    idt_entry_set(58, isr_stub_58, TYPE_INTE);
    idt_entry_set(59, isr_stub_59, TYPE_INTE);
	
}