/*--------------------------------------------------------------------------
*  File name:  e820.h
*  Author:  Aldenor Sombra de Oliveira
*  Data de criação: 24-11-2025
*--------------------------------------------------------------------------
Este header possui as estruturas para a guarda do mapa de memória física obtido
por meio da INT e820 durante o boot.
--------------------------------------------------------------------------*/
#ifndef E820_H
#define E820_H


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    E820_TYPE_USABLE        = 1,
    E820_TYPE_RESERVED      = 2,
    E820_TYPE_ACPI_RECLAIM  = 3,
    E820_TYPE_ACPI_NVS      = 4,
    E820_TYPE_BAD_MEMORY    = 5
} e820_type_t;

typedef struct {
    uint64_t base; 
    uint64_t length;     
    uint32_t type;   
} __attribute__((packed))  e820_entry20_t;

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) e820_entry24_t;

typedef struct {
    uint16_t count;
    uint16_t entry_size;
    uint8_t alinha[12];       
    uint8_t entries[];  //Tem que ser um vetor de uint8_t. Ponteiro desalinha
} __attribute__((packed)) e820_address_t;

//----------------------------------------------------------
typedef struct {
    uint64_t base;
    uint64_t length;  
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) phys_region_t;

typedef struct {
    uint16_t count;
    uint16_t entry_size;
    phys_region_t *mem_map;
} __attribute__((packed)) phys_map_t;
//----------------------------------------------------------

static inline e820_type_t e820_to_phys(uint32_t t) {
    switch (t) {
        case 1: return E820_TYPE_USABLE ;
        case 2: return E820_TYPE_RESERVED;
        case 3: return E820_TYPE_ACPI_RECLAIM;
        case 4: return E820_TYPE_ACPI_NVS;
        case 5: return E820_TYPE_BAD_MEMORY;
        default: return E820_TYPE_RESERVED;
    }
}


void e820_memory_init(e820_address_t *e820_address);

uint64_t e820_memory_size(void);
size_t e820_memory_free(void);
size_t e820_regions_count();

phys_region_t * e820_region_by_index(size_t index);

void e820_debug_print();

#endif
