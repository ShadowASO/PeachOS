#include "isr.h"
#include "../terminal/kprint.h"
#include "../pic/pic.h"
#include "../klib/panic.h"
#include "./exceptions.h"   // nova função
#include "../io/io.h"
#include "irq.h"
#include "../pic/pic_consts.h"



void isr_global_handler(int_stack_t *tsk_contxt)
{
    uint32_t vector = tsk_contxt->int_no;

    
    // kprint("\nInterrupt: ");
    // kprint_hex(vector);

    // 1) EXCEÇÕES DA CPU (0–31)
    if (vector < 32) {
        handle_cpu_exception(vector, tsk_contxt);
        // NUNCA VOLTA (panic_exception é noreturn)
    }

    // 2) IRQs REMAPEADAS (32+)
    if (vector == 0x21) {       
        isr_keyboard();
    }

    // Envia EOI apenas para IRQs
    pic_send_eoi(vector);
}



void no_interrupt_handler() {
    kprint("\nInterrupt default");   
   
}

void isr_default() {
    kprint("\nInterrupt default");   
}

void isr_keyboard() {
    uint8_t sc = __read_portb(0x60);  // OBRIGATÓRIO

    kprint("Kbd IRQ \n");
    kprint_hex(sc);   
}

void isr_divide_by_zero() {
    kprint("\nDivide by zero error");
    
}