#ifndef PANIC_H
#define PANIC_H

#include <stdint.h>
#include "../terminal/kprint.h"
#include "../idt/isr.h"   // para int_stack_t

/**
 * @brief Entra em estado de pânico e trava o kernel.
 *
 * Mostra mensagem, dump de registradores e trava o sistema.
 */
__attribute__((noreturn))
// void panic(const char *msg, int_stack_t *regs);
void panic(const char *msg);

/**
 * @brief Panic especializado para exceções da CPU.
 *
 * Recebe número da exceção e ponteiro para o contexto salvo.
 */
__attribute__((noreturn))
void panic_exception(uint32_t exception_vector, int_stack_t *regs);

#endif
