/* pmm.h - Physical Memory Manager baseado em bitmap
 *
 * Controla frames de memória física (usada/livre) por meio de um mapa de bits.
 * Cada bit representa um frame de tamanho fixo (PMM_FRAME_SIZE).
 */
/*
[ heap_region_start ] ----------------------+
                                           |
                                           v
    +----------------------+  <- region_start (alinhado)
    |  bitmap (u32...)     |
    +----------------------+
    |  heap_alloc_units[]  |
    +----------------------+
    |  padding p/ alinhar  |
    +----------------------+  <- heap_start_addr (início efetivo da heap)
    |  heap útil inicial   |  tamanho = heap_initial_size
    |        ...           |
    +----------------------+  <- heap_end_addr
    |  espaço p/ expandir  |  até heap_max_size
    |        ...           |
    +----------------------+  <- heap_max_addr

*/

#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "bootmem.h"

/* Medidas de Memória*/

#define KB_SIZE 1024ULL
#define MB_SIZE (KB_SIZE * KB_SIZE)
#define GB_SIZE (KB_SIZE * KB_SIZE * KB_SIZE)
#define TB_SIZE (KB_SIZE * KB_SIZE * KB_SIZE * KB_SIZE)


//-----------------------------------------------------

/* Tamanho de cada frame de memória física.
 * 4 KiB é o padrão clássico para x86. */
#define PMM_FRAME_SIZE        4096ull

/* Tamanho máximo de memória física suportada pelo bitmap.
 * Ajuste conforme sua máquina/uso (ex.: 256 MiB, 512 MiB, 1 GiB...). */

#define PMM_MAX_PHYS_MEM   (4ULL * 1024ULL * 1024ULL * 1024ULL) // 4 GiB

/* Número total de frames (cada um de 4 KiB) que cabem em PMM_MAX_PHYS_MEM. */
#define PMM_MAX_FRAMES        (PMM_MAX_PHYS_MEM / PMM_FRAME_SIZE)


/* Cada uint32_t tem 32 bits, então:
 * total de entradas necessárias no bitmap. */

#define PMM_BITMAP_SIZE_U32 ((PMM_MAX_FRAMES + 31ULL) / 32ULL)


#if PMM_MAX_PHYS_MEM == 0 || PMM_MAX_FRAMES == 0
#error "Overflow nas macros do PMM (use ULL)."
#endif

/* --------------------------------------------------------------------
 * Macros de manipulação de bits no bitmap
 * ------------------------------------------------------------------ */

/* Calcula índice do uint32_t dentro do bitmap para um dado frame. */
#define PMM_WORD_INDEX(frame_idx)   ((frame_idx) / 32u)
/* Calcula posição do bit dentro do uint32_t. */
#define PMM_BIT_OFFSET(frame_idx)   ((frame_idx) % 32u)

/* Marca frame como USADO (bit = 1) */
#define PMM_SET_BIT(bitmap, frame_idx) \
    do { \
        (bitmap)[PMM_WORD_INDEX(frame_idx)] |= (1u << PMM_BIT_OFFSET(frame_idx)); \
    } while (0)

/* Marca frame como LIVRE (bit = 0) */
#define PMM_CLEAR_BIT(bitmap, frame_idx) \
    do { \
        (bitmap)[PMM_WORD_INDEX(frame_idx)] &= ~(1u << PMM_BIT_OFFSET(frame_idx)); \
    } while (0)

/* Testa se frame está usado (bit == 1). Retorna 0 ou 1. */
#define PMM_TEST_BIT(bitmap, frame_idx) \
    ( ((bitmap)[PMM_WORD_INDEX(frame_idx)] >> PMM_BIT_OFFSET(frame_idx)) & 0x1u )

/* --------------------------------------------------------------------
 * API do gerenciador de memória física
 * ------------------------------------------------------------------ */

/* Inicializa o PMM.
 *
 *  - total_phys_mem_bytes: tamanho total de memória física detectada.
 *    (por exemplo, vindo da BIOS/multiboot).
 *
 *  O PMM inicialmente vai marcar TODOS os frames possíveis como LIVRES.
 *  Depois, você deve chamar pmm_mark_region_used() para marcar:
 *    - região do kernel
 *    - região reservada (BIOS, MMIO, etc.)
 */


void pmm_init(uint32_t *bitmap_ini, uint64_t phys_mem_size);

/* Marca uma região de memória física como USADA.
 *  base_phys: endereço físico inicial
 *  length: tamanho em bytes
 */

void pmm_mark_region_used64(uint64_t base_phys, uint64_t length);

/* Marca uma região de memória física como LIVRE.
 * Útil se você inicialmente marcou tudo como usado, por exemplo.
 */
void pmm_mark_region_free(uintptr_t base_phys, size_t length);

/* Aloca um frame físico livre e o marca como usado.
 * Retorna o endereço físico base do frame (múltiplo de PMM_FRAME_SIZE),
 * ou 0 em caso de falha (nenhum frame livre).
 *
 * Obs.: se você quiser tratar 0 como endereço válido, troque o valor
 * de erro para (uintptr_t) -1, por exemplo.
 */
uintptr_t pmm_alloc_frame(void);

/* Libera um frame físico (marca como livre) dado o endereço físico base. */
void pmm_free_frame(uintptr_t frame_addr);

/* Retorna quantidade de frames livres. */
size_t pmm_get_free_frame_count(void);

/* Retorna quantidade de memória física livre em bytes. */
size_t pmm_get_free_memory_bytes(void);

uintptr_t pmm_bitmap_end_addr(void);

size_t pmm_calc_bitmap_size_bytes(uint64_t phys_mem_size);

static inline uintptr_t virt_to_phys_kernel(uintptr_t virt)
{
    uintptr_t _kernel_virt_base=(uintptr_t)get_kernel_ini_vmm();
    uintptr_t _kernel_virt_end=(uintptr_t)get_kernel_end_vmm();
    uintptr_t _kernel_phys_base=(uintptr_t)get_kernel_ini_phys();

    if (virt < (uintptr_t)&_kernel_virt_base ||
        virt >= (uintptr_t)&_kernel_virt_end) {
        // não é endereço do kernel
        return virt; // assume identity
    }

    return virt - (uintptr_t)&_kernel_virt_base
                + (uintptr_t)&_kernel_phys_base;
}


#endif /* PMM_H */
