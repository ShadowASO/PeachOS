#include "pic.h"
#include "../io/io.h"
#include "../terminal/kprint.h"
#include "../terminal/screen.h"
#include "pic_consts.h"
#include "../idt/idt.h"

// ---------------------------------------------------------
// Inicialização padrão (ICWs corretas)
// ---------------------------------------------------------

void setup_pic(void)
{
    disable_interrupts();

    // ICW1
    __write_portb(PIC1_COMMAND, 0x11);
    __write_portb(PIC2_COMMAND,  0x11);

    // ICW2 — remapeamento
    __write_portb(PIC1_DATA, 0x20);   // IRQ 0–7 -> IDT 32–39
    __write_portb(PIC2_DATA, 0x28);   // IRQ 8–15 -> IDT 40–47

    // ICW3 — ligação entre PIC1 e PIC2
    __write_portb(PIC1_DATA, 0x04);   // PIC2 ligado na IRQ2 (bit 2)
    __write_portb(PIC2_DATA, 0x02);   // PIC2 responde na IRQ2 do mestre

    // ICW4
    __write_portb(PIC1_DATA, 0x01);
    __write_portb(PIC2_DATA, 0x01);

    // Inicialmente deixa TUDO mascarado
    __write_portb(PIC1_DATA, 0xFF);
    __write_portb(PIC2_DATA, 0xFF);

    enable_interrupts();
}

// ---------------------------------------------------------
// Ler máscara completa dos dois PICs (IMR)
// ---------------------------------------------------------
uint16_t pic_get_mask(void)
{
    uint8_t m1 = __read_portb(PIC1_DATA);
    uint8_t m2 = __read_portb(PIC2_DATA);
    return (uint16_t)(m1 | (m2 << 8));
}

// ---------------------------------------------------------
// Habilita uma IRQ específica (0–15)
// ---------------------------------------------------------
void pic_enable_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t  bit;

    if (irq < 8) {
        port = PIC1_DATA;
        bit = irq;
    } else {
        port = PIC2_DATA;
        bit = irq - 8;
    }

    uint8_t value = __read_portb(port);
    value &= ~(1 << bit);     // 0 = habilitada
    __write_portb(port, value);
}

// ---------------------------------------------------------
// Desabilita uma IRQ específica (0–15)
// ---------------------------------------------------------
void pic_disable_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t  bit;

    if (irq < 8) {
        port = PIC1_DATA;
        bit = irq;
    } else {
        port = PIC2_DATA;
        bit = irq - 8;
    }

    uint8_t value = __read_portb(port);
    value |= (1 << bit);      // 1 = mascarada
    __write_portb(port, value);
}

// ---------------------------------------------------------
// Habilita TODAS as IRQs
// ---------------------------------------------------------
void pic_enable_all(void)
{
    __write_portb(PIC1_DATA, 0x00);
    __write_portb(PIC2_DATA, 0x00);
}

// ---------------------------------------------------------
// Desabilita TODAS as IRQs
// ---------------------------------------------------------
void pic_disable_all(void)
{
    __write_portb(PIC1_DATA, 0xFF);
    __write_portb(PIC2_DATA, 0xFF);
}

// ---------------------------------------------------------
// Debug: mostra máscara das IRQs (opcional)
// ---------------------------------------------------------
void pic_dump_mask(void)
{
    uint16_t mask = pic_get_mask();

    kprint("PIC MASK (IRQ 0–15): 0b");
    for (int i = 15; i >= 0; i--) {
        kputchar_attrib((mask & (1 << i)) ? '1' : '0',
                 get_text_video_atrib(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
    kprint("\n");
}

/**
 * Envia EOI (End Of Interrupt) ao PIC.
 *
 * O parâmetro é o número DO VETOR REMAPEADO (32–47), não o número da IRQ.
 */
void pic_send_eoi(uint32_t vector)
{
    // Exceções da CPU não devem enviar EOI
    if (vector < 32)
        return;

    // Calcula IRQ real
    uint8_t irq = vector - 32;

    // IRQ 8–15 → pertence ao PIC2 (slave)
    if (irq >= 8)
        __write_portb(PIC2_COMMAND, PIC_EOI);

    // IRQ 0–7 → sempre sinalizam ao master
    __write_portb(PIC1_COMMAND, PIC_EOI);
}

/* ============================================================================
    Leitura do IRR / ISR (para debug e logs do kernel)
   ============================================================================ */

static uint16_t pic_read_reg(uint8_t ocw3)
{
    __write_portb(PIC1_COMMAND, ocw3);
    __write_portb(PIC2_COMMAND, ocw3);
    return ((__read_portb(PIC2_COMMAND) << 8) | __read_portb(PIC1_COMMAND));
}

uint16_t pic_read_irr(void)
{
    return pic_read_reg(PIC_READ_IRR);
}

uint16_t pic_read_isr(void)
{
    return pic_read_reg(PIC_READ_ISR);
}
