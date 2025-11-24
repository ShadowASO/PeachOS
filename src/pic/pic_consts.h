#ifndef __PIC_CONSTS_H__
#define __PIC_CONSTS_H__

#include <stdint.h>
#include "../idt/irq.h"

/* ============================================================================
    Portas do PIC (Intel 8259A)
   ============================================================================ */


/* -------------------------------------------------------------------------
 *  Portas padrão do PIC 8259
 * ------------------------------------------------------------------------- */


#define PIC1_PORT            0x20u
#define PIC1_COMMAND         PIC1_PORT
#define PIC1_DATA            (PIC1_PORT + 1u)

#define PIC2_PORT            0xA0u
#define PIC2_COMMAND         PIC2_PORT
#define PIC2_DATA            (PIC2_PORT + 1u)

#define PIC_EOI              0x20
#define PIC_READ_IRR         0x0A
#define PIC_READ_ISR         0x0B

/* Remapeamento padrão */
#define PIC1_REMAP_OFFSET    0x20u
#define PIC2_REMAP_OFFSET    0x28u


/* ============================================================================
    VETORES NA IDT APÓS REMAPEAMENTO
   ============================================================================ */

/* PIC1 (0x20–0x27) */
#define VEC_IRQ_TIMER         (PIC1_REMAP_OFFSET + IRQ_TIMER)
#define VEC_IRQ_KEYBOARD      (PIC1_REMAP_OFFSET + IRQ_KEYBOARD)
#define VEC_IRQ_CASCADE       (PIC1_REMAP_OFFSET + IRQ_CASCADE)
#define VEC_IRQ_COM2       (PIC1_REMAP_OFFSET + IRQ_COM2)
#define VEC_IRQ_COM1       (PIC1_REMAP_OFFSET + IRQ_COM1)
#define VEC_IRQ_LPT2     (PIC1_REMAP_OFFSET + IRQ_LPT2)
#define VEC_IRQ_FLOPPY      (PIC1_REMAP_OFFSET + IRQ_FLOPPY)
#define VEC_IRQ_LPT1     (PIC1_REMAP_OFFSET + IRQ_LPT1)

/* PIC2 (0x28–0x2F) */
#define VEC_IRQ_RTC           (PIC2_REMAP_OFFSET + (IRQ_RTC - 8))
#define VEC_IRQ_ACPI          (PIC2_REMAP_OFFSET + (IRQ_ACPI - 8))
#define VEC_IRQ_UNUSED1       (PIC2_REMAP_OFFSET + (IRQ_UNUSED1 - 8))
#define VEC_IRQ_UNUSED2       (PIC2_REMAP_OFFSET + (IRQ_UNUSED2 - 8))
#define VEC_IRQ_MOUSE         (PIC2_REMAP_OFFSET + (IRQ_MOUSE - 8))
#define VEC_IRQ_FPU           (PIC2_REMAP_OFFSET + (IRQ_FPU - 8))
#define VEC_IRQ_ATA_PRIMARY   (PIC2_REMAP_OFFSET + (IRQ_ATA_PRIMARY - 8))
#define VEC_IRQ_ATA_SECONDARY (PIC2_REMAP_OFFSET + (IRQ_ATA_SECONDARY - 8))


#endif
