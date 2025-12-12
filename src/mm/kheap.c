// kheap.c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "kheap.h"
#include "pmm.h"      // pmm_alloc_frame / pmm_free_frame
#include "bootmem.h"  // se tiver algo útil aqui
#include "../klib/memory.h"

/* Se você tiver VMM, descomente/ajuste conforme sua assinatura */
/// extern void vmm_map_page(uintptr_t phys, uintptr_t virt, uint32_t flags);

/* flags típicos: presente + escrita (ajuste para o seu VMM) */
#define KHEAP_PAGE_FLAGS   0x3u

/* ----------------------------------------------------
 * Estado global da heap
 * -------------------------------------------------- */

/* Região virtual total reservada para a heap (metadados + dados) */
static uintptr_t heap_region_start = 0;
static size_t    heap_region_size  = 0;

/* Área de dados da heap (depois dos metadados) */
static uintptr_t heap_start_addr = 0;  // início da área de dados
static uintptr_t heap_end_addr   = 0;  // fim atual (mapeado/útil)
static uintptr_t heap_max_addr   = 0;  // limite máximo de dados (start + kheap_max_size)

/* bitmap: 1 bit por unidade de HEAP_UNIT bytes dentro da área de dados */
static uint32_t *heap_bitmap      = NULL;

/* tabela de tamanhos: para cada unidade que é INÍCIO de um bloco,
 * guarda quantas unidades esse bloco ocupa (0 = não é início). */
static uint32_t *heap_alloc_units = NULL;

/* tamanhos globais em bytes/unidades apenas da ÁREA DE DADOS */
static size_t kheap_initial_size    = 0;  // quanto começa ativo
static size_t kheap_max_size        = 0;  // máximo possível de dados
static size_t kheap_max_units       = 0;  // kheap_max_size / HEAP_UNIT
static size_t kheap_bitmap_size_u32 = 0;  // tamanho do bitmap em u32

/* contador de unidades livres dentro da faixa atualmente mapeada */
static size_t heap_free_units       = 0;

/* ----------------------------------------------------
 * Helpers de bitmap
 * -------------------------------------------------- */

#define HEAP_WORD_INDEX(unit_idx)   ((unit_idx) / 32u)
#define HEAP_BIT_OFFSET(unit_idx)   ((unit_idx) % 32u)

static inline bool heap_unit_is_used(uint32_t unit_idx)
{
    uint32_t word = heap_bitmap[HEAP_WORD_INDEX(unit_idx)];
    return ((word >> HEAP_BIT_OFFSET(unit_idx)) & 1u) != 0u;
}

static inline void heap_set_unit_used(uint32_t unit_idx)
{
    heap_bitmap[HEAP_WORD_INDEX(unit_idx)] |= (1u << HEAP_BIT_OFFSET(unit_idx));
}

static inline void heap_set_unit_free(uint32_t unit_idx)
{
    heap_bitmap[HEAP_WORD_INDEX(unit_idx)] &= ~(1u << HEAP_BIT_OFFSET(unit_idx));
}

/* unidades atualmente disponíveis dentro da área de dados mapeada */
static inline uint32_t heap_current_total_units(void)
{
    //return (uint32_t)((heap_end_addr - heap_start_addr) / HEAP_UNIT);
    uint32_t units = (uint32_t)((heap_end_addr - heap_start_addr) / HEAP_UNIT);
    if (units > kheap_max_units) units = (uint32_t)kheap_max_units;
    return units;
}

/* ----------------------------------------------------
 * Expansão da heap (integração com PMM + VMM)
 * -------------------------------------------------- */

/* Garante que haja ao menos 'bytes_needed' adicionais além do tamanho atual.
 * Usa PMM para obter páginas físicas e (opcionalmente) VMM para mapear.
 *
 * Retorna true em caso de sucesso, false se não houver páginas físicas
 * ou se passar do limite kheap_max_size.
 */
static bool heap_expand(uint32_t bytes_needed)
{
    if (bytes_needed == 0) {
        return true;
    }

    uintptr_t cur_size      = heap_end_addr - heap_start_addr;
    uintptr_t required_size = cur_size + bytes_needed;

    if (required_size > kheap_max_size) {
        /* não podemos crescer além do máximo configurado */
        return false;
    }

    /* arredonda para múltiplos de página */
    uintptr_t new_size_rounded = ALIGN_UP(required_size, KHEAP_PAGE_SIZE);
    uintptr_t delta            = new_size_rounded - cur_size;

    if (delta == 0) {
        return true;
    }

    uintptr_t vaddr     = heap_start_addr + cur_size;
    uintptr_t end_vaddr = vaddr + delta;

    while (vaddr < end_vaddr) {
        //uintptr_t phys = pmm_alloc_frame();
         uintptr_t phys = (uintptr_t)boot_early_kalloc(PAGE_SIZE, PAGE_SIZE);
        if (phys == 0) {
            /* falha ao obter página física: idealmente reverter o que já foi
             * mapeado nesta expansão, mas por simplicidade retornamos false. */
            return false;
        }

        // vmm_map_page(phys, vaddr, KHEAP_PAGE_FLAGS);
        vaddr += KHEAP_PAGE_SIZE;
    }

    /* atualiza fim da heap */
    heap_end_addr = heap_start_addr + new_size_rounded;

    /* ajusta contador de unidades livres: novas unidades entraram em jogo */
    uint32_t old_units = (uint32_t)(cur_size / HEAP_UNIT);
    uint32_t new_units = (uint32_t)(new_size_rounded / HEAP_UNIT);
    heap_free_units   += (new_units - old_units);

    return true;
}

/* ----------------------------------------------------
 * Inicialização
 * -------------------------------------------------- */

/*
 * region_start      : início virtual da região reservada (metadados + dados)
 * region_size       : tamanho TOTAL reservado para heap
 * initial_heap_size : quanto da ÁREA DE DADOS começa ativo (o resto é expansão)
 *
 * IMPORTANTE:
 * - A região [region_start, region_start + region_size) já deve estar
 *   mapeada pelo código inicial de paginação (ou ser mapeável via VMM).
 */
void kheap_init(uint32_t region_start,
                uint32_t region_size,
                uint32_t initial_heap_size)
{
    /* alinha início da região à granularidade de unidade */
    uintptr_t rs        = (uintptr_t)region_start;
    uintptr_t re        = rs + (uintptr_t)region_size;
    uintptr_t rs_aligned = ALIGN_UP(rs, HEAP_UNIT);

    if (rs_aligned >= re) {
        /* região não sobra nada depois do alinhamento */
        return;
    }

    heap_region_start = rs_aligned;
    heap_region_size  = (size_t)(re - rs_aligned);

    uintptr_t region_end   = heap_region_start + heap_region_size;
    size_t    region_bytes = (size_t)(region_end - heap_region_start);

    /* chute inicial: máximo de unidades de dados se não houvesse metadados */
    size_t max_units_guess = region_bytes / HEAP_UNIT;
    if (max_units_guess == 0) {
        /* região muito pequena para qualquer heap */
        return;
    }

    size_t meta_bytes;
    size_t bitmap_bytes;
    size_t alloc_bytes;
    size_t data_bytes;

    /* Loop para ajustar kheap_max_units até caber
     * METADADOS + DADOS dentro de region_size. */
    for (;;) {
        kheap_max_units = max_units_guess;

        /* bitmap: 1 bit por unidade de dados -> ceil(units / 32) u32 */
        bitmap_bytes = sizeof(uint32_t) * ((kheap_max_units + 31u) / 32u);
        /* tabela de tamanhos: 1 uint32_t por unidade de dados */
        alloc_bytes  = sizeof(uint32_t) * kheap_max_units;

        meta_bytes   = bitmap_bytes + alloc_bytes;
        meta_bytes   = (size_t)ALIGN_UP(meta_bytes, HEAP_UNIT);

        if (meta_bytes >= region_bytes) {
            /* metadados nem cabem na região → reduzimos o chute */
            if (max_units_guess == 0) {
                return;
            }
            max_units_guess /= 2;
            continue;
        }

        data_bytes = region_bytes - meta_bytes;

        /* garante que a área de dados comporta kheap_max_units unidades */
        if (data_bytes / HEAP_UNIT < kheap_max_units) {
            if (max_units_guess == 0) {
                return;
            }
            max_units_guess /= 2;
            continue;
        }

        break; /* encontramos um kheap_max_units válido */
    }

    /* tamanho máximo de dados efetivamente utilizáveis */
    kheap_max_size = (data_bytes / HEAP_UNIT) * HEAP_UNIT;

    /* tamanho inicial ativo (pode ser menor que o máximo) */
    size_t init_bytes = ALIGN_UP(initial_heap_size, KHEAP_PAGE_SIZE);
    if (init_bytes == 0 || init_bytes > kheap_max_size) {
        init_bytes = kheap_max_size;
    }
    kheap_initial_size = init_bytes;

    /* Layout na memória:
     *
     * [ heap_region_start ]
     *   -> heap_bitmap (bitmap_bytes)
     *   -> heap_alloc_units (alloc_bytes)
     *   -> padding até HEAP_UNIT (meta_bytes total)
     *   -> heap_start_addr (área de dados) ... heap_max_addr
     */
    uint8_t *meta_base = (uint8_t*)heap_region_start;

    heap_bitmap = (uint32_t*)meta_base;
    kheap_bitmap_size_u32 = (kheap_max_units + 31u) / 32u;
    for (size_t i = 0; i < kheap_bitmap_size_u32; ++i) {
        heap_bitmap[i] = 0u;
    }

    heap_alloc_units = (uint32_t*)(meta_base + bitmap_bytes);
    for (size_t i = 0; i < kheap_max_units; ++i) {
        heap_alloc_units[i] = 0u;
    }

    heap_start_addr = heap_region_start + meta_bytes;
    heap_end_addr   = heap_start_addr + kheap_initial_size;
    heap_max_addr   = heap_start_addr + kheap_max_size;

    /* todas as unidades na faixa [heap_start_addr, heap_end_addr) começam livres */
    heap_free_units = heap_current_total_units();

    /* Marcar a área de memória física do heap_bitmap */
    pmm_mark_region_used((uintptr_t)heap_bitmap, kheap_bitmap_size_u32 * INT32_BYTE_SIZE);

    /* Marcar a área de memória física do heap_bitmap */
    pmm_mark_region_used((uintptr_t)heap_alloc_units, kheap_bitmap_size_u32 * INT32_BYTE_SIZE);

    /* Marcar a área de memória física do heap */
    pmm_mark_region_used((uintptr_t)heap_region_start, kheap_initial_size);
}

/* ----------------------------------------------------
 * Alocação interna por unidades com alinhamento
 * -------------------------------------------------- */

static void* heap_alloc_units_aligned(uint32_t units_needed,
                                      uint32_t align_bytes,
                                      bool can_expand)
{
    if (units_needed == 0) {
        return NULL;
    }

    /* 0 ou 1 → sem requisito especial */
    if (align_bytes < HEAP_UNIT) {
        align_bytes = HEAP_UNIT;
    }

    for (;;) {
        uint32_t total_units = heap_current_total_units();

        if (units_needed > total_units) {
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

        for (uint32_t start_unit = 0;
             start_unit + units_needed <= total_units;
             ++start_unit)
        {
            uintptr_t addr =
                heap_start_addr + (uintptr_t)start_unit * HEAP_UNIT;

            /* verifica alinhamento real em bytes */
            if (align_bytes && (addr % align_bytes) != 0u) {
                continue;
            }

            bool all_free = true;
            for (uint32_t u = 0; u < units_needed; ++u) {
                if (heap_unit_is_used(start_unit + u)) {
                    all_free = false;
                    start_unit += u;  /* pular para depois do ocupado */
                    break;
                }
            }

            if (!all_free) {
                continue;
            }

            /* marca como usado */
            for (uint32_t u = 0; u < units_needed; ++u) {
                heap_set_unit_used(start_unit + u);
            }
            heap_alloc_units[start_unit] = units_needed;
            heap_free_units             -= units_needed;

            return (void*)addr;
        }

        if (can_expand) {
            uint32_t bytes_needed = units_needed * HEAP_UNIT;
            if (!heap_expand(bytes_needed)) {
                return NULL;
            }
            continue;
        }

        return NULL;
    }
}
static inline bool heap_is_initialized(void) {
    return (heap_start_addr != 0 && heap_max_addr != 0);
}

/* ----------------------------------------------------
 * API pública: kmalloc, kmalloc_aligned, kcalloc, krealloc, kfree
 * -------------------------------------------------- */

void* kmalloc(size_t size)
{
    if (!heap_is_initialized() || size == 0) return NULL;

    if (size == 0) {
        return NULL;
    }

    /* arredonda para múltiplos de HEAP_UNIT */
    uint32_t units_needed = (uint32_t)((size + (HEAP_UNIT - 1u)) / HEAP_UNIT);

    /* alinhamento mínimo "normal": HEAP_ALIGNMENT (ex.: 8) */
    return heap_alloc_units_aligned(units_needed, HEAP_ALIGNMENT, true);
}

void* kzalloc(size_t size) {
    void *ptr=kmalloc(size);
    kmemset(ptr,0, size);
    return ptr;

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
    if (n == 0 || size == 0) {
        return NULL;
    }

    /* proteção simples contra overflow de n * size */
    size_t total = n * size;
    if (total / n != size) {
        return NULL;
    }

    void* ptr = kmalloc(total);
    if (!ptr) {
        return NULL;
    }
    kmemset(ptr, 0, total);
      

    return ptr;
}

void kfree(void* ptr)
{
    if (!ptr) {
        return;
    }

    uintptr_t addr = (uintptr_t)ptr;

    if (addr < heap_start_addr || addr >= heap_end_addr) {
        /* ponteiro fora da área de dados da heap */
        return;
    }

    uint32_t start_unit = (uint32_t)((addr - heap_start_addr) / HEAP_UNIT);
    uint32_t units      = heap_alloc_units[start_unit];

    if (units == 0) {
        #ifdef KHEAP_DEBUG
             kprintf("[kfree] ponteiro %p não é início de bloco; possivel double free ou corrupção\n", ptr);
        #endif
        /* não é início de bloco (talvez double free ou ptr inválido) */
        return;
    }

    for (uint32_t u = 0; u < units; ++u) {
        heap_set_unit_free(start_unit + u);
    }

    heap_alloc_units[start_unit] = 0;
    heap_free_units             += units;
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
        /* ponteiro fora da heap ⇒ trata como erro */
        return NULL;
    }

    uint32_t start_unit = (uint32_t)((addr - heap_start_addr) / HEAP_UNIT);
    uint32_t old_units  = heap_alloc_units[start_unit];

    if (old_units == 0) {
        /* não é início de bloco conhecido ⇒ erro */
        return NULL;
    }

    uint32_t old_size_bytes = old_units * HEAP_UNIT;
    uint32_t new_units      = (uint32_t)((new_size + (HEAP_UNIT - 1u)) / HEAP_UNIT);

    /* caso 1: novo tamanho cabe dentro do bloco atual */
    if (new_units <= old_units) {
        if (new_units == old_units) {
            return ptr;
        }

        uint32_t units_to_free = old_units - new_units;
        uint32_t start_free    = start_unit + new_units;

        for (uint32_t u = 0; u < units_to_free; ++u) {
            heap_set_unit_free(start_free + u);
        }
        heap_alloc_units[start_unit] = new_units;
        heap_free_units             += units_to_free;

        return ptr;
    }

    /* caso 2: tentar expandir "para frente" se as unidades seguintes estiverem livres */
    uint32_t extra_units = new_units - old_units;
    uint32_t total_units = heap_current_total_units();

    if (start_unit + new_units <= total_units) {
        bool can_grow_in_place = true;
        uint32_t first_extra   = start_unit + old_units;

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
            heap_free_units             -= extra_units;

            return ptr;
        }
    }

    /* caso 3: não dá para crescer in-place ⇒ aloca novo bloco, copia, libera o antigo */
    void* new_ptr = kmalloc(new_size);
    if (!new_ptr) {
        return NULL;
    }

    uint32_t copy_bytes = (new_size < old_size_bytes)
                            ? (uint32_t)new_size
                            : old_size_bytes;

    uint8_t* dst = (uint8_t*)new_ptr;
    uint8_t* src = (uint8_t*)ptr;

    for (uint32_t i = 0; i < copy_bytes; ++i) {
        dst[i] = src[i];
    }

    kfree(ptr);
    return new_ptr;
}

void* kpage_alloc(void)
{
    /* página virtual 4KiB, alinhada em 4KiB */
    return kmalloc_aligned(KHEAP_PAGE_SIZE, KHEAP_PAGE_SIZE);
}

/* se quiser um bloco de N páginas alinhado em página */
void* kpages_alloc(size_t num_pages)
{
    size_t bytes = num_pages * KHEAP_PAGE_SIZE;
    return kmalloc_aligned(bytes, KHEAP_PAGE_SIZE);
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

#ifdef KHEAP_DEBUG

void kheap_debug_dump_bitmap(void)
{
    uint32_t total_units = heap_current_total_units();
    uint32_t total_words = (total_units + 31u) / 32u;

    kprintf("=== kheap bitmap dump ===\n");
    kprintf("heap_start_addr = %p\n", (void*)heap_start_addr);
    kprintf("heap_end_addr   = %p\n", (void*)heap_end_addr);
    kprintf("heap_max_addr   = %p\n", (void*)heap_max_addr);
    kprintf("total_units     = %u\n", total_units);
    kprintf("heap_free_units = %u\n", (uint32_t)heap_free_units);
    kprintf("bitmap_words    = %u\n", total_words);

    for (uint32_t w = 0; w < total_words; ++w) {
        uint32_t word = heap_bitmap[w];

        kprintf("  [%4u] 0x%08x | ", w, word);

        /* Imprime os bits daquele word, apenas até o total_units */
        for (uint32_t bit = 0; bit < 32u; ++bit) {
            uint32_t unit = w * 32u + bit;
            if (unit >= total_units) {
                break;
            }
            kprintf("%c", (word & (1u << bit)) ? '1' : '0');
        }
        kprintf("\n");
    }

    kprintf("=== fim do bitmap ===\n");
}

#endif /* KHEAP_DEBUG */

#ifdef KHEAP_DEBUG

void kheap_debug_dump_blocks(void)
{
    uint32_t total_units = heap_current_total_units();

    kprintf("=== kheap blocks dump ===\n");
    kprintf("heap_start_addr = %p\n", (void*)heap_start_addr);
    kprintf("heap_end_addr   = %p\n", (void*)heap_end_addr);
    kprintf("total_units     = %u\n", total_units);
    kprintf("heap_free_units = %u\n", (uint32_t)heap_free_units);

    for (uint32_t u = 0; u < total_units; ) {

        uint32_t units = heap_alloc_units[u];

        if (units == 0) {
            /* não é início de bloco, só anda 1 unidade */
            ++u;
            continue;
        }

        uintptr_t addr = heap_start_addr + (uintptr_t)u * HEAP_UNIT;
        uint32_t bytes = units * HEAP_UNIT;

        /* Verifica se o primeiro unit está marcado como usado (sanidade) */
        bool used = heap_unit_is_used(u);

        kprintf("  bloco @ %p: start_unit=%u, units=%u, bytes=%u, used=%s\n",
                (void*)addr,
                u,
                units,
                bytes,
                used ? "yes" : "no");

        /* pula direto para depois do bloco atual */
        u += units;
    }

    kprintf("=== fim dos blocos ===\n");
}

#endif /* KHEAP_DEBUG */
