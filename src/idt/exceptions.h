#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdint.h>
#include "../idt/isr.h"

/**
 * @brief Trata uma exceção da CPU (0–31).
 *
 * Imprime informação sobre a exceção, o nome (#GP, #DE, #PF),
 * e então delega ao sistema de panic() para dump dos registradores.
 */
void handle_cpu_exception(uint32_t vector, int_stack_t *frame);
const char * get_exception_name(uint32_t vector);

#endif
