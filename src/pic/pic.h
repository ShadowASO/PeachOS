#ifndef __PIC_H__
#define __PIC_H__

#include <stdint.h>


void setup_pic(void);
uint16_t pic_get_mask(void);
void pic_enable_irq(uint8_t irq);
void pic_disable_irq(uint8_t irq);
void pic_enable_all(void);
void pic_disable_all(void);
void pic_dump_mask(void);
void pic_send_eoi(uint32_t vector);

static uint16_t pic_read_reg(uint8_t ocw3);
uint16_t pic_read_irr(void);
uint16_t pic_read_isr(void);



#endif
