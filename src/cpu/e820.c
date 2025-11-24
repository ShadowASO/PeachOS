/*--------------------------------------------------------------------------
*  File name:  e820.c
*  Author:  Aldenor Sombra de Oliveira
*  Data de criação: 24-11-2025
*--------------------------------------------------------------------------
Este source code cria as estruturas e guarda o mapa de memória física obtido
por meio da INT e820 durante o boot.
--------------------------------------------------------------------------*/
 #include "e820.h"
#include "../klib/kprintf.h"
#include "../klib/memory.h"

#define MAX_E820_ENTRIES 128

//Vetor para receber as entradas do mapa de memória
static phys_region_t phys_entries[MAX_E820_ENTRIES];
//Cria a estrutura que irá guardar em memória o mapa da memória
static phys_map_t physmap;


void e820_collect_regions(e820_address_t *e820_address)
{    
   
    uint8_t *raw = e820_address->entries;

    for (uint16_t i = 0; i < e820_address->count; i++) {

        if (i >= MAX_E820_ENTRIES)
            break;

        uint8_t *ent = raw + i * e820_address->entry_size;

        // Interpretar como 20 ou 24 bytes
        uint64_t base   = *(uint64_t *)(ent + 0);
        uint64_t length = *(uint64_t *)(ent + 8);
        uint32_t type   = *(uint32_t *)(ent + 16);

        uint32_t acpi = 0;
        if (e820_address->entry_size >= 24)
            acpi = *(uint32_t *)(ent + 20);

        phys_region_t r = {
            .base   = base,
            .length = length,
            .type   = type,
            .acpi   = acpi,
        };
        
        physmap.mem_map[i]=r;
    }
}


uint64_t get_total_usable_ram(void)
{
    uint64_t sum = 0;

    for (size_t i = 0; i < physmap.count; i++) {
        if (physmap.mem_map[i].type == E820_TYPE_USABLE)
            sum += physmap.mem_map[i].length;
    }

    return sum;
}


void e820_debug_print()
{
    kprintf("\nEntradas E820 detectadas: %d\n", physmap.count);

    for (size_t i = 0; i < physmap.count; i++) {
        uint32_t base32   = (uint32_t)physmap.mem_map[i].base;
        uint32_t length32 = (uint32_t)physmap.mem_map[i].length;
        uint32_t type     = physmap.mem_map[i].type;

        kprintf(
            "\nE820[%d]: base=0x%X length=0x%X tipo=%d ",
            (int)i,
            base32,
            length32,
            type
        );
    }
}


void memory_init(e820_address_t *e820_address)
{
    kprintf("\nDetectando memory via E820...\n");

    //limpo o vetor de registros do mapa de memória
    memset(phys_entries,0, sizeof(phys_entries));

    //Atribui o vetor de entradas ao campo da estrutura de memória
    physmap.mem_map=phys_entries;

    //Guarda a quantidade de registros
    physmap.count=e820_address->count;

    //O tamanho de cada registro
    physmap.entry_size=e820_address->entry_size;
   
    //Preenche a estrutura com o mapa da memória devolvido por E820
    e820_collect_regions(e820_address);

    //Exibe na tela o mapa da memória    
    e820_debug_print();

    //Extrai o total de memória livre
    uint64_t total = get_total_usable_ram();

    kprintf("\nTotal de RAM livre: %u MB\n", (uint32_t)(total / (1024 * 1024)));
}
