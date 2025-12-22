#ifndef KERR_H
#define KERR_H

#include <stdint.h>
#include <stddef.h>
#include "./panic.h"   // ajuste o include conforme seu layout

// ou: #include "panic.h"

/*
 * ============================================
 *  Integração com seu kernel
 * ============================================
 * Você já tem:
 *   void panic(const char *msg, int_stack_t *regs) __attribute__((noreturn));
 *
 * Aqui assumimos:
 *   - kprint/kprint_hex existem (ou você ajusta KERR_LOG)
 *   - int_stack_t é o frame salvo de interrupção
 */

/* -------------------------------------------------------
 * Logging (opcional). Ajuste para seu logger preferido.
 * ------------------------------------------------------- */
#ifndef KERR_LOG
/* Se você tiver kprintf, pode trocar. */
//#define KERR_LOG(msg) do { kprint(msg); } while (0)
#define KERR_LOG(...)   kprintf(__VA_ARGS__)

#endif

/* -------------------------------------------------------
 * Branch hints (opcional)
 * ------------------------------------------------------- */
#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

/* -------------------------------------------------------
 * Erros (mínimo). Você pode mover para errno.h depois.
 * ------------------------------------------------------- */
#ifndef EPERM
#define EPERM   1
#endif
#ifndef ENOENT
#define ENOENT  2
#endif
// #ifndef EIO
// #define EIO     5
// #endif
// #ifndef ENOMEM
// #define ENOMEM  12
// #endif
#ifndef EINVAL
#define EINVAL  22
#endif
#ifndef ERANGE
#define ERANGE  34
#endif
#ifndef ENOSYS
#define ENOSYS  38
#endif

#define KERR(code) (-(int)(code))

/* =======================================================
 *  PANIC / BUG / WARN
 * ======================================================= */

/* BUG sem regs (uso em código comum) */
#define BUG(msg) \
    do { \
        panic((msg), NULL); \
    } while (0)

/* BUG com regs (uso em handlers) */
#define BUG_R(msg, regs) \
    do { \
        panic((msg), (regs)); \
    } while (0)

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

/*
 * WARN_ON: retorna 1 se disparou, 0 se não disparou.
 * Não trava o sistema.
 */
#define WARN_ON(cond) \
    ({ \
        int __w = !!(cond); \
        if (unlikely(__w)) { \
            kprint("WARN_ON: " #cond " @ " __FILE__ ":"); \
            kprint_dec(__LINE__); \
            kprint("\n"); \
        } \
        __w; \
    })

#define WARN_ON_R(cond, regs) \
    ({ \
        int __w = !!(cond); \
        if (unlikely(__w)) { \
            (void)(regs); \
            kprint("WARN_ON: " #cond " @ " __FILE__ ":"); \
            kprint_dec(__LINE__); \
            kprint("\n"); \
        } \
        __w; \
    })

/* =======================================================
 *  Guard clauses / propagação de erro (int)
 * ======================================================= */

#define RETURN_IF(cond, errcode) \
    do { if (unlikely(cond)) return (errcode); } while (0)

#define RETURN_IF_NULL(ptr, errcode) \
    do { if (unlikely((ptr) == NULL)) return (errcode); } while (0)

#define RETURN_IF_ERR(expr) \
    do { int __rc = (expr); if (unlikely(__rc < 0)) return __rc; } while (0)

#define TRY(expr) \
    do { int __rc = (expr); if (unlikely(__rc < 0)) return __rc; } while (0)

#define TRY_GOTO(expr, label) \
    do { int __rc = (expr); if (unlikely(__rc < 0)) goto label; } while (0)

/* =======================================================
 *  Ponteiro com erro codificado (estilo Linux)
 * ======================================================= */

#ifndef MAX_ERRNO
#define MAX_ERRNO 4095
#endif

static inline int k_is_err_value(uintptr_t v)
{
    return v >= (uintptr_t)-(intptr_t)MAX_ERRNO;
}

#define ERR_PTR(err) ((void *)(intptr_t)(err))
#define PTR_ERR(ptr) ((int)(intptr_t)(ptr))
#define IS_ERR(ptr)  k_is_err_value((uintptr_t)(ptr))
#define IS_ERR_OR_NULL(ptr) (!(ptr) || IS_ERR(ptr))

#define RETURN_ERR_PTR_IF(cond, errcode) \
    do { if (unlikely(cond)) return ERR_PTR(errcode); } while (0)

#define RETURN_ERR_PTR_IF_NULL(ptr, errcode) \
    do { if (unlikely((ptr) == NULL)) return ERR_PTR(errcode); } while (0)

/* Propaga erro de um ponteiro retornado, mas retorna int */
#define PTR_TRY(ptr_expr) \
    ({ \
        void *__p = (ptr_expr); \
        if (unlikely(IS_ERR(__p))) return PTR_ERR(__p); \
        __p; \
    })

/* =======================================================
 *  Utilitários internos: stringify de __LINE__
 *  (para evitar printf; usa concat em compile-time)
 * ======================================================= */
#define _KERR_STR2(x) #x
#define _KERR_STR(x)  _KERR_STR2(x)

#define ERROR(value) (void*)(value)
#define ERROR_I(value) (int)(value)
#define ISERR(value) ((int)value < 0)

#endif /* KERR_H */
