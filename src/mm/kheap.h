// kheap.h
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
#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Granularidade básica da heap (cada unidade) */
#ifndef HEAP_UNIT
#define HEAP_UNIT        16u
#endif

/* Alinhamento mínimo padrão para kmalloc */
#ifndef HEAP_ALIGNMENT
#define HEAP_ALIGNMENT   8u
#endif

/* Tamanho de página usado pela heap (deve casar com o VMM/PMM) */
#ifndef KHEAP_PAGE_SIZE
#define KHEAP_PAGE_SIZE  4096u
#endif

/* Helpers de alinhamento (se ainda não estiverem em outro header) */
#ifndef ALIGN_UP
//#define ALIGN_UP(x, a)   ( ((uintptr_t)(x) + ((a) - 1u)) & ~((uintptr_t)((a) - 1u)) )
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------
 * Inicialização da heap
 * -------------------------------------------------- */

/**
 * Inicializa a heap a partir de um endereço virtual `heap_start`
 * e usando inicialmente `heap_size` bytes.
 *
 * Pressupõe que [heap_start, heap_start + heap_size) já esteja mapeado
 * (por exemplo, pelo bootmem / paginador inicial).
 */
//void kheap_init(uint32_t heap_start, uint32_t heap_size);
void kheap_init(uint32_t heap_region_start,
                uint32_t heap_initial_size,
                uint32_t heap_max_size);


/* ----------------------------------------------------
 * API de alocação
 * -------------------------------------------------- */

/* Aloca `size` bytes sem alinhamento especial (usa HEAP_ALIGNMENT) */
void* kmalloc(size_t size);

/* Aloca `size` bytes com alinhamento explícito em `align` */
void* kmalloc_aligned(size_t size, size_t align);

/* Aloca `n * size` bytes e zera a memória retornada */
void* kcalloc(size_t n, size_t size);

/* Redimensiona um bloco previamente alocado, podendo mover o bloco */
void* krealloc(void* ptr, size_t new_size);

/* Libera um bloco previamente alocado por kmalloc/kcalloc/kmalloc_aligned/krealloc */
void kfree(void* ptr);

void* kpage_alloc(void);

/* se quiser um bloco de N páginas alinhado em página */
void* kpages_alloc(size_t num_pages);

/* ----------------------------------------------------
 * Estatísticas da heap
 * -------------------------------------------------- */

/* Retorna o número de unidades livres (unidades de HEAP_UNIT bytes) */
size_t kheap_get_free_units(void);

/* Retorna o número total de unidades atualmente mapeadas/úteis */
size_t kheap_get_total_units(void);

/* ----------------------------------------------------
 * Funções de debug (dump)
 * -------------------------------------------------- */
/* Habilite com -DKHEAP_DEBUG no compilador, se quiser condicionar */
#ifdef KHEAP_DEBUG

/**
 * Faz o dump do bitmap de unidades da heap:
 * - Mostra os words do bitmap e os bits (0 = livre, 1 = usado).
 */
void kheap_debug_dump_bitmap(void);

/**
 * Faz o dump dos blocos conhecidos pela tabela heap_alloc_units:
 * - Mostra endereço, número de unidades, tamanho em bytes, etc.
 */
void kheap_debug_dump_blocks(void);

#endif /* KHEAP_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* KHEAP_H */
