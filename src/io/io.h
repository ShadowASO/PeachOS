
#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <stdint.h>

// Escrevendo e lendo bytes
void __write_portb(uint16_t port, uint8_t data);
uint8_t __read_portb(uint16_t port);

// Escrevendo e lendo words
void __write_portw(uint16_t port, uint16_t data);
uint16_t __read_portw(uint16_t port);

// Escrevendo e lendo double words
void __write_portd(uint16_t port, uint32_t data);
uint32_t __read_portd(uint16_t port);


#endif