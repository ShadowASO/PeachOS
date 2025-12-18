#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdint.h>

/* ----------------------------------------------------
   Base física e base virtual
---------------------------------------------------- */
#define KERNEL_PHYS_BASE  0x00100000      /* Bootloader carrega aqui */

/* Kernel executa aqui     */
#define KERNEL_VIRT_BASE  0xC0000000   

/* Tamanho de cada bloco de memória page       */
#define PAGE_SIZE         4096

/* Offset da memória virtual em relação à memória física */
#define KERNEL_OFFSET   (KERNEL_VIRT_BASE - KERNEL_PHYS_BASE)

#define MAX_FILESYSTEMS 12
#define MAX_FILEDESCRIPTORS 512

#endif