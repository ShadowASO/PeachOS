#include "panic.h"
#include "../idt/exceptions.h"
#include "../cpu/cpu.h"


static void dump_regs(int_stack_t *r)
{
    kprint("\n\n==== REGISTERS ====\n");

    kprint("EAX="); kprint_hex(r->eax); kprint("  ");
    kprint("EBX="); kprint_hex(r->ebx); kprint("\n");

    kprint("ECX="); kprint_hex(r->ecx); kprint("  ");
    kprint("EDX="); kprint_hex(r->edx); kprint("\n");

    kprint("ESI="); kprint_hex(r->esi); kprint("  ");
    kprint("EDI="); kprint_hex(r->edi); kprint("\n");

    kprint("EBP="); kprint_hex(r->ebp); kprint("  ");
    kprint("ESP="); kprint_hex(r->esp); kprint("\n");

    kprint("\n--- STACK FRAME ---\n");
    kprint("EIP="); kprint_hex(r->ip); kprint("\n");
    kprint("CS ="); kprint_hex(r->cs); kprint("\n");
    kprint("EFLAGS="); kprint_hex(r->flags); kprint("\n");
    kprint("USER ESP="); kprint_hex(r->esp); kprint("\n");
    kprint("SS="); kprint_hex(r->ss); kprint("\n");

    kprint("\n====================\n");
}


__attribute__((noreturn))
void panic(const char *msg)
{
    kprint("\n*** KERNEL PANIC ***\n");
    kprint(msg);
    kprint("\n");

    //dump_regs(regs);

    kprint("\nSistema travado.\n");

    _wait();
    __builtin_unreachable(); // opcional, ajuda o otimizador/diagnóstico

}


__attribute__((noreturn))
void panic_exception(uint32_t vector, int_stack_t *regs)
{
    kprint("\n*** CPU EXCEPTION ***\n");

    if (vector < 32)        
        kprint(get_exception_name(vector));
    else
        kprint("Excecao desconhecida");

    kprint("\nCodigo do vetor: ");
    kprint_hex(vector);
    kprint("\n");

    if (regs) {
        dump_regs(regs);
    }

   _wait();

   __builtin_unreachable();  // opcional, ajuda o otimizador/diagnóstico
}


