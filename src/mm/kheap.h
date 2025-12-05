// kheap.h
#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ----------------------------------------------------
 * Alinhamento e limites da heap
 * -------------------------------------------------- */

// alinhamento mínimo "normal" (usado em kmalloc)
#define HEAP_ALIGNMENT      8u

// endereço virtual onde a heap começa (ajuste conforme seu linker)
#define KHEAP_START         0xC0000000u

// tamanho inicial (já mapeado) da heap ao iniciar o kernel
#define KHEAP_INITIAL_SIZE  0x00100000u  /* 1 MiB */

// tamanho máximo que a heap poderá crescer
#define KHEAP_MAX_SIZE      0x0100000u  /* 16 MiB */

/* unidade básica da heap (granularidade do bitmap) */
#define HEAP_UNIT           16u          /* 16 bytes por unidade */

/* tamanho de página (para expansão integrada ao PMM/VMM) */
#define KHEAP_PAGE_SIZE     4096u        /* 4 KiB */

/* número máximo de unidades dentro do tamanho máximo da heap */
#define KHEAP_MAX_UNITS     (KHEAP_MAX_SIZE / HEAP_UNIT)

/* bitmap: 1 bit por unidade */
#define KHEAP_BITMAP_SIZE_U32  (KHEAP_MAX_UNITS / 32u)

/* macro de alinhamento genérica */
#define ALIGN_UP(x, align)   ( ((x) + ((align) - 1u)) & ~((align) - 1u) )

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa a heap:
 *  - heap_start: endereço virtual inicial (ex.: KHEAP_START)
 *  - heap_size:  tamanho inicial já mapeado (ex.: KHEAP_INITIAL_SIZE)
 *
 * Obs.: heap_size não pode exceder KHEAP_MAX_SIZE; se exceder, será truncado.
 *       A expansão posterior usará PMM + VMM para mapear novas páginas.
 */
void kheap_init(uint32_t heap_start, uint32_t heap_size);

/* Alocação "normal" com alinhamento mínimo (HEAP_ALIGNMENT). */
void* kmalloc(size_t size);

/* Alocação alinhada: garante que o ponteiro retornado seja múltiplo de `align`.
 * Ex.: kmalloc_aligned(4096, 4096) → bloco page-aligned. */
void* kmalloc_aligned(size_t size, size_t align);

/* Alocação zerada (n * size bytes), estilo calloc. */
void* kcalloc(size_t n, size_t size);

/* Realocação:
 *  - comporta-se como malloc se ptr == NULL;
 *  - como free se new_size == 0;
 *  - tenta expandir in-place, se possível; caso contrário, aloca, copia e libera. */
void* krealloc(void* ptr, size_t new_size);

/* Libera um bloco previamente alocado. */
void  kfree(void* ptr);

/* (Opcional) estatísticas simples – quantidade de unidades livres. */
size_t kheap_get_free_units(void);
size_t kheap_get_total_units(void);

#ifdef __cplusplus
}
#endif

#endif /* KHEAP_H */
