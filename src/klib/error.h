#ifndef KERR_H
#define KERR_H

#include <stdint.h>
#include <stddef.h>

// Ajuste conforme seu layout
#include "./panic.h"   // panic(const char *msg, int_stack_t *regs) noreturn

/* =======================================================
 *  Stringify de __LINE__ (tem que vir antes de usar)
 * ======================================================= */
#define _KERR_STR2(x) #x
#define _KERR_STR(x)  _KERR_STR2(x)

/* =======================================================
 *  Branch hints (opcional)
 * ======================================================= */
#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

/* =======================================================
 *  Logging (opcional)
 *  - Por padrão, não loga (para evitar dependência).
 *  - Se você tiver kprintf, defina KERR_LOG(...) antes de incluir este header.
 * ======================================================= */
#ifndef KERR_LOG
#define KERR_LOG(...) do { } while (0)
#endif

/* =======================================================
 *  Erros mínimos (ideal: mover para errno.h)
 * ======================================================= */
#ifndef EPERM
#define EPERM   1
#endif
#ifndef ENOENT
#define ENOENT  2
#endif
#ifndef EINVAL
#define EINVAL  22
#endif
#ifndef ERANGE
#define ERANGE  34
#endif
#ifndef ENOSYS
#define ENOSYS  38
#endif
#ifndef ENOMEM
#define ENOMEM  12
#endif

/* Converte errno positivo para retorno negativo */
#define KERR(code) (-(int)(code))

/* =======================================================
 *  PANIC / BUG / WARN
 * ======================================================= */

/* BUG sem regs (uso em código comum) */
#define BUG(msg) \
    do { panic((msg), NULL); } while (0)

/* BUG com regs (uso em handlers/ISR) */
#define BUG_R(msg, regs) \
    do { panic((msg), (regs)); } while (0)

/* BUG_ON: aborta o kernel se cond for true */
#define BUG_ON(cond) \
    do { \
        if (unlikely(cond)) { \
            panic("BUG_ON: " #cond " @ " __FILE__ ":" _KERR_STR(__LINE__), NULL); \
        } \
    } while (0)

#define BUG_ON_R(cond, regs) \
    do { \
        if (unlikely(cond)) { \
            panic("BUG_ON: " #cond " @ " __FILE__ ":" _KERR_STR(__LINE__), (regs)); \
        } \
    } while (0)

/* WARN_ON: loga e retorna 1 se cond for true; não aborta */
#define WARN_ON(cond) \
    ({ \
        int __w = !!(cond); \
        if (unlikely(__w)) { \
            KERR_LOG("WARN_ON: %s @ %s:%d\n", #cond, __FILE__, __LINE__); \
        } \
        __w; \
    })

#define WARN_ON_R(cond, regs) \
    ({ \
        int __w = !!(cond); \
        (void)(regs); \
        if (unlikely(__w)) { \
            KERR_LOG("WARN_ON: %s @ %s:%d\n", #cond, __FILE__, __LINE__); \
        } \
        __w; \
    }),

/* =======================================================
 *  Guard clauses / propagação de erro (funções que retornam int)
 * ======================================================= */

#define RETURN_IF(cond, errcode) \
    do { if (unlikely(cond)) return (errcode); } while (0)

/* Comum: retorna -EINVAL, -ENOMEM, etc. */
#define RETURN_ERR_IF(cond, errno_pos) \
    do { if (unlikely(cond)) return KERR(errno_pos); } while (0)

#define RETURN_ERR_IF_NULL(ptr, errno_pos) \
    do { if (unlikely((ptr) == NULL)) return KERR(errno_pos); } while (0)

#define RETURN_IF_ERR(expr) \
    do { int __rc = (expr); if (unlikely(__rc < 0)) return __rc; } while (0)

#define TRY(expr) RETURN_IF_ERR(expr)

#define TRY_GOTO(expr, label) \
    do { int __rc = (expr); if (unlikely(__rc < 0)) goto label; } while (0)

/* =======================================================
 *  Ponteiro com erro codificado (estilo Linux)
 *  - Útil quando você quer retornar ponteiro, mas também retornar errno
 *  - NÃO confunda com ponteiros high-half; isso resolve exatamente esse caso.
 * ======================================================= */

#ifndef MAX_ERRNO
#define MAX_ERRNO 4095
#endif

/* True se x estiver no range de erros codificados (topo do espaço de endereços) */
#define IS_ERR_VALUE(x) ((uintptr_t)(x) >= (uintptr_t)-(intptr_t)(MAX_ERRNO))

#define IS_ERR(ptr)      IS_ERR_VALUE(ptr)
#define IS_ERR_OR_NULL(p) ((p) == NULL || IS_ERR(p))

/* Converte -errno (int) em ponteiro-erro */
#define ERR_PTR(err_neg) ((void*)(intptr_t)(err_neg))

/* Extrai -errno (intptr_t) do ponteiro-erro */
#define PTR_ERR(ptr)     ((intptr_t)(ptr))

/* Variante para retornar int sem ambiguidade */
#define PTR_ERR_I(ptr)   ((int)(intptr_t)(ptr))

/* Guard/return para funções que retornam ponteiro */
#define RETURN_ERR_PTR_IF(cond, errno_pos) \
    do { if (unlikely(cond)) return ERR_PTR(KERR(errno_pos)); } while (0)

#define RETURN_ERR_PTR_IF_NULL(ptr, errno_pos) \
    do { if (unlikely((ptr) == NULL)) return ERR_PTR(KERR(errno_pos)); } while (0)

/* Propaga erro de função que retorna ponteiro-erro, mas sua função retorna int */
#define PTR_TRY(ptr_expr) \
    ({ \
        void* __p = (ptr_expr); \
        if (unlikely(IS_ERR(__p))) return PTR_ERR_I(__p); \
        __p; \
    })

/* Propaga erro de função que retorna ponteiro-erro, e você também retorna ponteiro */
#define PTR_TRY_PTR(ptr_expr) \
    ({ \
        void* __p = (ptr_expr); \
        if (unlikely(IS_ERR(__p))) return __p; \
        __p; \
    })

/* =======================================================
 *  NÃO USE com ponteiro high-half:
 *    - "ISERR(value) ((int)value<0)" é incorreto para VA >= 0x80000000.
 * ======================================================= */

#endif /* KERR_H */
