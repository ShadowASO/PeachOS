#include "exceptions.h"
#include "../terminal/kprint.h"
#include "../klib/panic.h"

static const char *exception_names[] = {
    "#DE Divide Error",
    "#DB Debug",
    "NMI Interrupt",
    "#BP Breakpoint",
    "#OF Overflow",
    "#BR BOUND Range Exceeded",
    "#UD Invalid Opcode",
    "#NM Device Not Available",
    "#DF Double Fault",
    "Coprocessor Segment Overrun",
    "#TS Invalid TSS",
    "#NP Segment Not Present",
    "#SS Stack-Segment Fault",
    "#GP General Protection Fault",
    "#PF Page Fault",
    "(Reserved)",
    "#MF x87 Floating-Point Exception",
    "#AC Alignment Check",
    "#MC Machine Check",
    "#XF SIMD Floating-Point Exception",
    "#VE Virtualization Exception",
    "#CP Control Protection Exception",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved"
};

const char * get_exception_name(uint32_t vector) {
    return exception_names[vector];
}

void handle_cpu_exception(uint32_t vector, int_stack_t *frame)
{
    kprint("\n=== CPU EXCEPTION ===\n");

    if (vector < 32) {
        kprint(exception_names[vector]);
    } else {
        kprint("Exception desconhecida");
    }

    kprint("\nVetor: ");
    kprint_hex(vector);

    // entrega ao sistema de pânico (que dá dump de registradores)
    panic_exception(vector, frame);
}
