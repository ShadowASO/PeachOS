/* =========================================================================
 *  File......: bitops.h
 *  Autor.....: (seu nome)
 *  Descrição.: Macros utilitárias para manipulação de bits em uint32_t
 *              + máscaras de IRQs, portas e flags de registradores x86.
 * ========================================================================= */

#ifndef BITOPS_H
#define BITOPS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 *  Macros básicas de bit
 * -------------------------------------------------------------------------
 *
 *  Convenção:
 *    - pos  : posição do bit (0..31)
 *    - var  : variável do tipo uint32_t (ou compatível, ex: registrador)
 *    - mask : máscara de bits (um ou mais bits já deslocados)
 *
 *  Observação:
 *    - Não use expressões com efeitos colaterais em 'var' ou 'pos'
 *      (ex.: SET_BIT(*p++, i++)), pois a macro pode avaliá-las mais de uma vez.
 */

/* Cria uma máscara com apenas o bit 'pos' ligado. */
#define BIT_MASK(pos)            ((uint32_t)(1u << (pos)))

/* --- Operações sobre um único bit -------------------------------------- */

#define BIT_SET(var, pos)       ((var) |=  BIT(pos))
#define BIT_CLEAR(var, pos)     ((var) &= ~BIT(pos))
#define BIT_TOGGLE(var, pos)    ((var) ^=  BIT(pos))
#define IS_BIT_SET(var, pos)    (((var) & BIT(pos)) != 0u)

/* --- Operações sobre múltiplos bits (usando máscara) ------------------- */

#define BITS_SET(var, mask)        ((var) |=  (uint32_t)(mask))
#define BITS_CLEAR(var, mask)      ((var) &= (uint32_t)~(mask))
#define BITS_TOGGLE(var, mask)     ((var) ^=  (uint32_t)(mask))

/* Verifica se TODOS os bits de 'mask' estão setados em 'var'. */
#define ARE_BITS_SET(var, mask) \
    ((((var) & (uint32_t)(mask)) == (uint32_t)(mask)))

/* Verifica se AO MENOS UM bit de 'mask' está setado em 'var'. */
#define ANY_BIT_SET(var, mask) \
    (((var) & (uint32_t)(mask)) != 0u)


#ifdef __cplusplus
}
#endif

#endif /* BITOPS_H */
