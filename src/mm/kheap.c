// kheap.c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "kheap.h"
#include "pmm.h"    // para pmm_alloc_frame / pmm_free_frame

/* Se você já tiver um header do VMM, inclua aqui.
 * Caso contrário, ajuste a assinatura da função abaixo para a sua. */
//extern void vmm_map_page(uintptr_t phys, uintptr_t virt, uint32_t flags);

/* flags típicos: presente + escrita (ajuste para o seu VMM) */
#define KHEAP_PAGE_FLAGS   0x3u

/* ----------------------------------------------------
 * Estado global da heap
 * -------------------------------------------------- */

static uintptr_t heap_start_addr = 0;
static uintptr_t heap_end_addr   = 0;  // fim atual (mapeado/útil)
static uintptr_t heap_max_addr   = 0;  // limite máximo (start + KHEAP_MAX_SIZE)

/* bitmap: 1 bit por unidade de 16 bytes */
// static uint32_t heap_bitmap[KHEAP_BITMAP_SIZE_U32] = {0};
static uint32_t heap_bitmap[100] = {0};

/* tabela de tamanhos: para cada unidade que é INÍCIO de um bloco,
 * guarda quantas unidades esse bloco ocupa.
 *  - 0 => não é início de bloco */
// static uint32_t heap_alloc_units[KHEAP_MAX_UNITS] = {0};
static uint32_t heap_alloc_units[100] = {0};

/* contador simples de unidades livres dentro da região atualmente mapeada */
static size_t heap_free_units = 0;

/* ----------------------------------------------------
 * Helpers de bitmap
 * -------------------------------------------------- */

#define HEAP_WORD_INDEX(unit_idx)   ((unit_idx) / 32u)
#define HEAP_BIT_OFFSET(unit_idx)   ((unit_idx) % 32u)

static inline bool heap_unit_is_used(uint32_t unit_idx)
{
    uint32_t word = heap_bitmap[HEAP_WORD_INDEX(unit_idx)];
    return ( (word >> HEAP_BIT_OFFSET(unit_idx)) & 1u );
}

static inline void heap_set_unit_used(uint32_t unit_idx)
{
    heap_bitmap[HEAP_WORD_INDEX(unit_idx)] |= (1u << HEAP_BIT_OFFSET(unit_idx));
}

static inline void heap_set_unit_free(uint32_t unit_idx)
{
    heap_bitmap[HEAP_WORD_INDEX(unit_idx)] &= ~(1u << HEAP_BIT_OFFSET(unit_idx));
}

/* unidades atualmente disponí­veis dentro da faixa mapeada */
static inline uint32_t heap_current_total_units(void)
{
    return (uint32_t)((heap_end_addr - heap_start_addr) / HEAP_UNIT);
}

/* ----------------------------------------------------
 * Expansão da heap (integração com PMM + VMM)
 * -------------------------------------------------- */

/* Garante que haja ao menos 'bytes_needed' adicionais além do tamanho atual.
 * Usa PMM para obter páginas físicas e VMM para mapear no espaço da heap.
 *
 * Retorna true em caso de sucesso, false se não houver páginas físicas
 * ou se passar do limite heap_max_addr.
 */
static bool heap_expand(uint32_t bytes_needed)
{
    if (bytes_needed == 0) {
        return true;
    }

    uintptr_t cur_size = heap_end_addr - heap_start_addr;
    uintptr_t required_size = cur_size + bytes_needed;

    if (required_size > KHEAP_MAX_SIZE) {
        /* não podemos crescer além do máximo configurado */
        return false;
    }

    /* arredonda para múltiplos de página */
    uintptr_t new_size_rounded = ALIGN_UP(required_size, KHEAP_PAGE_SIZE);
    uintptr_t delta = new_size_rounded - cur_size;

    if (delta == 0) {
        return true;
    }

    uintptr_t vaddr = heap_start_addr + cur_size;
    uintptr_t end_vaddr = vaddr + delta;

    while (vaddr < end_vaddr) {
        uintptr_t phys = pmm_alloc_frame();
        if (phys == 0) {
            /* falha ao obter página física: poderíamos tentar reverter, mas
             * por simplicidade, devolvemos false e paramos. */
            return false;
        }

        //vmm_map_page(phys, vaddr, KHEAP_PAGE_FLAGS);
        vaddr += KHEAP_PAGE_SIZE;
    }

    /* atualiza fim da heap */
    heap_end_addr = heap_start_addr + new_size_rounded;

    /* ajusta contador de unidades livres: novas unidades entraram em jogo */
    uint32_t old_units = (uint32_t)(cur_size / HEAP_UNIT);
    uint32_t new_units = (uint32_t)(new_size_rounded / HEAP_UNIT);

    heap_free_units += (new_units - old_units);

    return true;
}

/* ----------------------------------------------------
 * Inicialização
 * -------------------------------------------------- */

void kheap_init(uint32_t heap_start, uint32_t heap_size)
{
    /* alinha início da heap à granularidade de unidade */
    uintptr_t aligned_start = ALIGN_UP((uintptr_t)heap_start, HEAP_UNIT);

    heap_start_addr = aligned_start;

    /* não deixar passar do tamanho máximo da heap */
    if (heap_size > KHEAP_MAX_SIZE) {
        heap_size = KHEAP_MAX_SIZE;
    }

    /* alinhamos heap_size para múltiplo de página para simplificar mapeamento */
    heap_size = (uint32_t)ALIGN_UP(heap_size, KHEAP_PAGE_SIZE);

    heap_end_addr = heap_start_addr + heap_size;
    heap_max_addr = heap_start_addr + KHEAP_MAX_SIZE;

    /* zera bitmap e tabela de blocos */
    for (uint32_t i = 0; i < KHEAP_BITMAP_SIZE_U32; ++i) {
        heap_bitmap[i] = 0u;
    }
    for (uint32_t i = 0; i < KHEAP_MAX_UNITS; ++i) {
        heap_alloc_units[i] = 0u;
    }

    /* assume-se que a faixa [heap_start_addr, heap_end_addr) já está mapeada
     * pelo código de paginação inicial do kernel.
     *
     * Todas as unidades dentro desse intervalo começam como livres. */
    heap_free_units = heap_current_total_units();
}

/* ----------------------------------------------------
 * Alocação interna por unidades com alinhamento
 * -------------------------------------------------- */

/* Aloca 'units_needed' unidades de HEAP_UNIT bytes,
 * com alinhamento mínimo em bytes 'align_bytes'.
 *
 * Se can_expand == true, tenta expandir a heap se não encontrar espaço
 * na região mapeada atual.
 */
static void* heap_alloc_units_aligned(uint32_t units_needed,
                                      uint32_t align_bytes,
                                      bool can_expand)
{
    if (units_needed == 0) {
        return NULL;
    }

    /* converte alinhamento em unidades.
     * Ex.: align_bytes = 4096 => align_units = 4096/16 = 256 */
    uint32_t align_units = 1u;
    if (align_bytes > HEAP_UNIT) {
        align_units = align_bytes / HEAP_UNIT;
    }

    for (;;) {
        uint32_t total_units = heap_current_total_units();

        if (units_needed > total_units) {
            /* mesmo toda a heap atual não comporta; se pudermos expandir, tentamos */
            if (can_expand) {
                uint32_t bytes_needed = units_needed * HEAP_UNIT;
                if (!heap_expand(bytes_needed)) {
                    return NULL;
                }
                /* volta ao for(;;) para escanear com a heap maior */
                continue;
            } else {
                return NULL;
            }
        }

        /* se não há unidades livres suficientes nem em contagem, atalho rápido */
        if (heap_free_units < units_needed) {
            if (can_expand) {
                uint32_t bytes_needed = units_needed * HEAP_UNIT;
                if (!heap_expand(bytes_needed)) {
                    return NULL;
                }
                continue;
            } else {
                return NULL;
            }
        }

        /* procura um run de 'units_needed' unidades livres com alinhamento */
        for (uint32_t start_unit = 0; start_unit + units_needed <= total_units; ++start_unit) {

            /* respeita o alinhamento em unidades */
            if (align_units > 1u && (start_unit % align_units) != 0u) {
                continue;
            }

            bool all_free = true;
            for (uint32_t u = 0; u < units_needed; ++u) {
                if (heap_unit_is_used(start_unit + u)) {
                    all_free = false;
                    /* pular direto para depois do primeiro unit ocupado melhora um pouco */
                    start_unit += u;
                    break;
                }
            }

            if (!all_free) {
                continue;
            }

            /* achamos um intervalo de units_needed unidades livres a partir de start_unit */
            for (uint32_t u = 0; u < units_needed; ++u) {
                heap_set_unit_used(start_unit + u);
            }
            heap_alloc_units[start_unit] = units_needed;
            heap_free_units -= units_needed;

            uintptr_t addr = heap_start_addr + (uintptr_t)start_unit * HEAP_UNIT;
            return (void*)addr;
        }

        /* se chegou aqui, não achou espaço na heap atual */
        if (can_expand) {
            uint32_t bytes_needed = units_needed * HEAP_UNIT;
            if (!heap_expand(bytes_needed)) {
                return NULL;
            }
            /* tenta de novo com a heap expandida */
            continue;
        }

        return NULL;
    }
}

/* ----------------------------------------------------
 * API pública: kmalloc, kmalloc_aligned, kcalloc, krealloc, kfree
 * -------------------------------------------------- */

void* kmalloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    /* arredonda para múltiplos de HEAP_UNIT */
    uint32_t units_needed = (uint32_t)((size + (HEAP_UNIT - 1u)) / HEAP_UNIT);

    /* alinhamento mínimo "normal": HEAP_ALIGNMENT (ex.: 8) */
    return heap_alloc_units_aligned(units_needed, HEAP_ALIGNMENT, true);
}

void* kmalloc_aligned(size_t size, size_t align)
{
    if (size == 0) {
        return NULL;
    }

    uint32_t units_needed = (uint32_t)((size + (HEAP_UNIT - 1u)) / HEAP_UNIT);

    return heap_alloc_units_aligned(units_needed, (uint32_t)align, true);
}

void* kcalloc(size_t n, size_t size)
{
    size_t total = n * size;
    void* ptr = kmalloc(total);
    if (!ptr) {
        return NULL;
    }

    /* zera a memória manualmente */
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < total; ++i) {
        p[i] = 0;
    }

    return ptr;
}

void* krealloc(void* ptr, size_t new_size)
{
    if (!ptr) {
        return kmalloc(new_size);
    }

    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    uintptr_t addr = (uintptr_t)ptr;

    if (addr < heap_start_addr || addr >= heap_end_addr) {
        /* ponteiro fora da heap ⇒ trata como erro (aqui retornamos NULL) */
        return NULL;
    }

    uint32_t start_unit = (uint32_t)((addr - heap_start_addr) / HEAP_UNIT);
    uint32_t old_units = heap_alloc_units[start_unit];

    if (old_units == 0) {
        /* não é início de bloco conhecido ⇒ erro */
        return NULL;
    }

    uint32_t old_size_bytes = old_units * HEAP_UNIT;
    uint32_t new_units = (uint32_t)((new_size + (HEAP_UNIT - 1u)) / HEAP_UNIT);

    /* caso 1: novo tamanho cabe dentro do bloco atual */
    if (new_units <= old_units) {
        /* se quiser, pode liberar as unidades excedentes para trás */
        if (new_units == old_units) {
            return ptr;
        }

        uint32_t units_to_free = old_units - new_units;
        uint32_t start_free = start_unit + new_units;

        for (uint32_t u = 0; u < units_to_free; ++u) {
            heap_set_unit_free(start_free + u);
        }
        heap_alloc_units[start_unit] = new_units;
        heap_free_units += units_to_free;

        return ptr;
    }

    /* caso 2: tentar expandir "para frente" se as unidades seguintes estiverem livres */
    uint32_t extra_units = new_units - old_units;
    uint32_t total_units = heap_current_total_units();

    if (start_unit + new_units <= total_units) {
        bool can_grow_in_place = true;
        uint32_t first_extra = start_unit + old_units;

        for (uint32_t u = 0; u < extra_units; ++u) {
            if (heap_unit_is_used(first_extra + u)) {
                can_grow_in_place = false;
                break;
            }
        }

        if (can_grow_in_place) {
            for (uint32_t u = 0; u < extra_units; ++u) {
                heap_set_unit_used(first_extra + u);
            }
            heap_alloc_units[start_unit] = new_units;
            heap_free_units -= extra_units;

            return ptr;
        }
    }

    /* caso 3: não dá para crescer in-place ⇒ aloca novo bloco, copia, libera o antigo */
    void* new_ptr = kmalloc(new_size);
    if (!new_ptr) {
        return NULL;
    }

    /* copia o mínimo entre velho e novo tamanho */
    uint32_t copy_bytes = (new_size < old_size_bytes) ? (uint32_t)new_size : old_size_bytes;

    uint8_t* dst = (uint8_t*)new_ptr;
    uint8_t* src = (uint8_t*)ptr;

    for (uint32_t i = 0; i < copy_bytes; ++i) {
        dst[i] = src[i];
    }

    kfree(ptr);
    return new_ptr;
}

void kfree(void* ptr)
{
    if (!ptr) {
        return;
    }

    uintptr_t addr = (uintptr_t)ptr;

    if (addr < heap_start_addr || addr >= heap_end_addr) {
        /* ponteiro fora da heap */
        return;
    }

    uint32_t start_unit = (uint32_t)((addr - heap_start_addr) / HEAP_UNIT);
    uint32_t units = heap_alloc_units[start_unit];

    if (units == 0) {
        /* não é início de bloco (talvez double free ou ptr inválido) */
        return;
    }

    for (uint32_t u = 0; u < units; ++u) {
        heap_set_unit_free(start_unit + u);
    }

    heap_alloc_units[start_unit] = 0;
    heap_free_units += units;
}

/* ----------------------------------------------------
 * Estatísticas simples
 * -------------------------------------------------- */

size_t kheap_get_free_units(void)
{
    return heap_free_units;
}

size_t kheap_get_total_units(void)
{
    return (size_t)heap_current_total_units();
}
