#ifndef IRQ_CONSTANTS_H
#define IRQ_CONSTANTS_H

//
// IRQs do PIC (8259A) — IRQ 0 a 15
//

#define IRQ_TIMER        0    // Timer do sistema (PIT)
#define IRQ_KEYBOARD     1    // Teclado PS/2
#define IRQ_CASCADE      2    // Linha usada para encadear PIC2 → PIC1
#define IRQ_COM2         3    // Serial COM2 / COM4
#define IRQ_COM1         4    // Serial COM1 / COM3
#define IRQ_LPT2         5    // Porta paralela 2 (ou som em algumas placas)
#define IRQ_FLOPPY       6    // Controlador de disquete
#define IRQ_LPT1         7    // Porta paralela 1 (pode ser "spurious IRQ" também)

#define IRQ_RTC          8    // Relógio em tempo real (RTC)
#define IRQ_ACPI         9
#define IRQ_UNUSED1      10
#define IRQ_UNUSED2      11
#define IRQ_MOUSE       12    // Mouse PS/2
#define IRQ_FPU         13    // Coprocessador matemático (x87, FPU)
#define IRQ_ATA_PRIMARY 14    // IDE/ATA primário (ou SATA legacy)
#define IRQ_ATA_SECOND  15    // IDE/ATA secundário (ou SATA legacy)

/* -------------------------------------------------------------------------
 *  Máscaras pré-definidas para IRQs (8259 PIC)
 *
 *  Uso típico: PICx_DATA = máscara de interrupções (1 = desabilita IRQ).
 *  Ex.: para desabilitar apenas o teclado:
 *        uint8_t mask = IRQ1_KEYBOARD_MASK;
 *        outb(PIC1_DATA_PORT, mask);
 * ------------------------------------------------------------------------- */

/* Bits da máscara do PIC (IMR) correspondentes aos IRQs 0..15 */
#define IRQ0_TIMER_MASK            BIT(0)   /* Timer (PIT)                */
#define IRQ1_KEYBOARD_MASK         BIT(1)   /* Teclado PS/2               */
#define IRQ2_CASCADE_MASK          BIT(2)   /* Cascade para PIC escravo   */
#define IRQ3_COM2_MASK             BIT(3)   /* Porta Serial COM2          */
#define IRQ4_COM1_MASK             BIT(4)   /* Porta Serial COM1          */
#define IRQ5_LPT2_MASK             BIT(5)   /* LPT2 / som / uso geral     */
#define IRQ6_FLOPPY_MASK           BIT(6)   /* Controlador de disquete    */
#define IRQ7_LPT1_MASK             BIT(7)   /* LPT1 / spurious            */

#define IRQ8_RTC_MASK              BIT(8)   /* Relógio de tempo real      */
#define IRQ9_ACPI_MASK             BIT(9)   /* IRQ redirecionada / ACPI   */
#define IRQ10_RESERVED_MASK        BIT(10)  /* Reservado / uso geral      */
#define IRQ11_RESERVED_MASK        BIT(11)  /* Reservado / uso geral      */
#define IRQ12_MOUSE_MASK           BIT(12)  /* Mouse PS/2                 */
#define IRQ13_FPU_MASK             BIT(13)  /* Coprocessador/FPU          */
#define IRQ14_PRIMARY_ATA_MASK     BIT(14)  /* Controlador ATA primário   */
#define IRQ15_SECONDARY_ATA_MASK   BIT(15)  /* Controlador ATA secundário */

/* Máscaras compostas úteis */
#define IRQ_MASK_NONE              0x00u      /* Nenhuma IRQ mascarada    */
#define IRQ_MASK_ALL               0xFFu      /* Todas mascaradas         */

/* Ex.: desmascarar teclado mantendo o resto mascarado */
#define IRQ_MASK_ONLY_KEYBOARD     (~IRQ1_KEYBOARD_MASK)


#define IRQ_MASK(n)      (1U << (n))


#endif
