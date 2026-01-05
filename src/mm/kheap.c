// kheap.c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "./mm.h"
#include "kheap.h"
#include "pmm.h"      // pmm_alloc_frame / pmm_free_frame
#include "bootmem.h"  // se tiver algo útil aqui
#include "../klib/memory.h"
#include "./page/paging.h"
#include "./page/paging_kmap.h"
#include "../klib/panic.h"
#include "../klib/kprintf.h"

/* Se você tiver VMM, descomente/ajuste conforme sua assinatura */
/// extern void vmm_map_page(uintptr_t phys, uintptr_t virt, uint32_t flags);

/* flags típicos: presente + escrita (ajuste para o seu VMM) */
//#define KHEAP_PAGE_FLAGS   0x3u
// -----------------------------------------------------------------------------
// Configuração
// -----------------------------------------------------------------------------

// #ifndef KHEAP_BASE
// #define KHEAP_BASE 0xD0000000u
// #endif

// #ifndef KHEAP_PAGE_FLAGS
// // flags típicos para páginas do kernel: RW + PRESENT é aplicado dentro de paging_map,
// // mas mantemos RW aqui e o map adiciona PRESENT.
// #define KHEAP_PAGE_FLAGS (PAGE_RW)
// #endif

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

// #define HEAP_WORD_INDEX(unit_idx)   ((unit_idx) / 32u)
// #define HEAP_BIT_OFFSET(unit_idx)   ((unit_idx) % 32u)

// typedef int  (*kheap_map_page_fn)(uintptr_t virt, uintptr_t phys, uint32_t flags);
// typedef void (*kheap_zero_frame_fn)(uintptr_t phys);

// static kheap_map_page_fn   g_map_page   = NULL;
// static kheap_zero_frame_fn g_zero_frame = NULL;

// void kheap_set_paging_hooks(kheap_map_page_fn map_fn, kheap_zero_frame_fn zero_fn)
// {
//     g_map_page   = map_fn;
//     g_zero_frame = zero_fn;
// }


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

static inline bool heap_is_initialized(void)
{
    return (heap_start_addr != 0 && heap_max_addr != 0 && heap_bitmap && heap_alloc_units);
}

// -----------------------------------------------------------------------------
// Mapeamento/zero de frames
// -----------------------------------------------------------------------------

static inline void zero_frame_phys(uintptr_t phys)
{
    void* p = (void*)kmap(phys);
    kmemset(p, 0, PAGE_SIZE);
    kunmap();
}

static inline void map_page_kernel(uintptr_t virt, uintptr_t phys)
{
    paging_ctx_t *ctx=get_paging_ctx();
    // paging_map já deve fazer invlpg. flags de kernel RW.
    //if (paging_map(kernel_directory, &g_paging_ctx, virt, phys, KHEAP_PAGE_FLAGS) != 0) {
    if (paging_map(kernel_directory, ctx, virt, phys, KHEAP_PAGE_FLAGS) != 0) {
        panic("kheap: paging_map failed");
    }
}

/* ----------------------------------------------------
 * Expansão da heap (integração com PMM + VMM)
 * -------------------------------------------------- */

// -----------------------------------------------------------------------------
// Expansão da heap (PMM + paging_map)
// -----------------------------------------------------------------------------

static bool heap_expand(size_t bytes_needed)
{
    if (bytes_needed == 0) return true;
    if (!heap_is_initialized()) return false;

    size_t cur_size      = (size_t)(heap_end_addr - heap_start_addr);
    size_t required_size = cur_size + bytes_needed;

    if (required_size > kheap_max_size) {
        return false;
    }

    size_t new_size_rounded = (size_t)ALIGN_UP(required_size, PAGE_SIZE);
    size_t delta            = new_size_rounded - cur_size;
    if (delta == 0) return true;

    uintptr_t vaddr     = heap_start_addr + (uintptr_t)cur_size;
    uintptr_t end_vaddr = vaddr + (uintptr_t)delta;

    while (vaddr < end_vaddr) {
        uintptr_t phys = pmm_alloc_frame();
        if (!phys) {
            // Ideal: rollback. Por ora, falha simples.
            return false;
        }

        zero_frame_phys(phys);
        map_page_kernel(vaddr, phys);

        vaddr += PAGE_SIZE;
    }

    heap_end_addr = heap_start_addr + (uintptr_t)new_size_rounded;

    uint32_t old_units = (uint32_t)(cur_size / HEAP_UNIT);
    uint32_t new_units = (uint32_t)(new_size_rounded / HEAP_UNIT);
    heap_free_units   += (size_t)(new_units - old_units);

    return true;
}


// -----------------------------------------------------------------------------
// Inicialização
// -----------------------------------------------------------------------------

/*
 * region_start      : início virtual da região reservada (metadados + dados)
 * region_size       : tamanho TOTAL reservado para heap (VA)
 * initial_heap_size : quanto da ÁREA DE DADOS começa ativo (mapeado)
 *
 * IMPORTANTE:
 * - Antes de chamar kheap_init, você deve mapear páginas suficientes para cobrir:
 *   [region_start, region_start + meta_slack + initial_heap_size)
 *   onde meta_slack pode ser, por ex., 64KiB.
 *
 * Sugestão prática no memory_setup:
 *   - reserve VA fixo: KHEAP_BASE
 *   - mapear (pelo menos) (initial_heap_size + 64KiB) em páginas
 *   - chamar kheap_init(KHEAP_BASE, region_size, initial_heap_size)
 */
void kheap_init(uint32_t region_start,
                uint32_t region_size,
                uint32_t initial_heap_size)
{
    uintptr_t rs  = (uintptr_t)region_start;
    uintptr_t re  = rs + (uintptr_t)region_size;

    // alinhar início da região para HEAP_UNIT
    uintptr_t rs_aligned = ALIGN_UP(rs, HEAP_UNIT);
    if (rs_aligned >= re) {
        panic("kheap_init: region too small after align");
    }

    heap_region_start = rs_aligned;
    heap_region_size  = (size_t)(re - rs_aligned);

    uintptr_t region_end   = heap_region_start + (uintptr_t)heap_region_size;
    size_t    region_bytes = (size_t)(region_end - heap_region_start);

    // chute inicial: máximo de unidades se tudo fosse dado
    size_t max_units_guess = region_bytes / HEAP_UNIT;
    if (max_units_guess == 0) {
        panic("kheap_init: region too small");
    }

    size_t meta_bytes   = 0;
    size_t bitmap_bytes = 0;
    size_t alloc_bytes  = 0;
    size_t data_bytes   = 0;

    // Ajusta kheap_max_units até caber (metadados + dados)
    for (;;) {
        kheap_max_units = max_units_guess;

        bitmap_bytes = sizeof(uint32_t) * ((kheap_max_units + 31u) / 32u);
        alloc_bytes  = sizeof(uint32_t) * kheap_max_units;

        meta_bytes   = bitmap_bytes + alloc_bytes;
        meta_bytes   = (size_t)ALIGN_UP(meta_bytes, HEAP_UNIT);

        if (meta_bytes >= region_bytes) {
            max_units_guess /= 2;
            if (max_units_guess == 0) {
                panic("kheap_init: metadata doesn't fit");
            }
            continue;
        }

        data_bytes = region_bytes - meta_bytes;

        if ((data_bytes / HEAP_UNIT) < kheap_max_units) {
            max_units_guess /= 2;
            if (max_units_guess == 0) {
                panic("kheap_init: data area doesn't fit");
            }
            continue;
        }

        break;
    }

    kheap_max_size = (data_bytes / HEAP_UNIT) * HEAP_UNIT;

    size_t init_bytes = (size_t)ALIGN_UP((size_t)initial_heap_size, PAGE_SIZE);
    if (init_bytes == 0 || init_bytes > kheap_max_size) {
        init_bytes = kheap_max_size;
    }
    kheap_initial_size = init_bytes;

    // Layout
    uint8_t* meta_base = (uint8_t*)heap_region_start;

    heap_bitmap = (uint32_t*)meta_base;
    kheap_bitmap_size_u32 = (kheap_max_units + 31u) / 32u;
    for (size_t i = 0; i < kheap_bitmap_size_u32; ++i) {
        heap_bitmap[i] = 0u;
    }

    heap_alloc_units = (uint32_t*)(meta_base + bitmap_bytes);
    for (size_t i = 0; i < kheap_max_units; ++i) {
        heap_alloc_units[i] = 0u;
    }

    heap_start_addr = heap_region_start + (uintptr_t)meta_bytes;
    heap_end_addr   = heap_start_addr + (uintptr_t)kheap_initial_size;
    heap_max_addr   = heap_start_addr + (uintptr_t)kheap_max_size;

    heap_free_units = (size_t)heap_current_total_units();

#ifdef KHEAP_DEBUG
    kprintf("[kheap] region_start=%p size=%u\n", (void*)heap_region_start, (unsigned)heap_region_size);
    kprintf("[kheap] data_start=%p end=%p max=%p\n", (void*)heap_start_addr, (void*)heap_end_addr, (void*)heap_max_addr);
    kprintf("[kheap] units max=%u bitmap_u32=%u free_units=%u\n",
            (unsigned)kheap_max_units,
            (unsigned)kheap_bitmap_size_u32,
            (unsigned)heap_free_units);
#endif
}
// -----------------------------------------------------------------------------
// Alocação interna por unidades com alinhamento
// -----------------------------------------------------------------------------

static void* heap_alloc_units_aligned(uint32_t units_needed,
                                      uint32_t align_bytes,
                                      bool can_expand)
{
    if (!heap_is_initialized() || units_needed == 0) return NULL;

    if (align_bytes < HEAP_UNIT) align_bytes = HEAP_UNIT;

    for (;;) {
        uint32_t total_units = heap_current_total_units();

        if (units_needed > total_units || heap_free_units < units_needed) {
            if (!can_expand) return NULL;

            size_t bytes_needed = (size_t)units_needed * (size_t)HEAP_UNIT;
            if (!heap_expand(bytes_needed)) return NULL;
            continue;
        }

        for (uint32_t start_unit = 0;
             start_unit + units_needed <= total_units;
             ++start_unit)
        {
            uintptr_t addr = heap_start_addr + (uintptr_t)start_unit * HEAP_UNIT;

            if ((addr % align_bytes) != 0u) {
                continue;
            }

            bool all_free = true;
            for (uint32_t u = 0; u < units_needed; ++u) {
                if (heap_unit_is_used(start_unit + u)) {
                    all_free = false;
                    start_unit += u; // pular
                    break;
                }
            }
            if (!all_free) continue;

            for (uint32_t u = 0; u < units_needed; ++u) {
                heap_set_unit_used(start_unit + u);
            }

            heap_alloc_units[start_unit] = units_needed;
            heap_free_units             -= units_needed;

            return (void*)addr;
        }

        if (!can_expand) return NULL;

        size_t bytes_needed = (size_t)units_needed * (size_t)HEAP_UNIT;
        if (!heap_expand(bytes_needed)) return NULL;
    }
}


// -----------------------------------------------------------------------------
// API pública
// -----------------------------------------------------------------------------

void* kmalloc(size_t size)
{
    if (!heap_is_initialized() || size == 0) return NULL;

    uint32_t units_needed = (uint32_t)((size + (HEAP_UNIT - 1u)) / HEAP_UNIT);
    return heap_alloc_units_aligned(units_needed, HEAP_ALIGNMENT, true);
}

void* kzalloc(size_t size)
{
    void* ptr = kmalloc(size);
    if (!ptr) return NULL;
    kmemset(ptr, 0, size);
    return ptr;
}

void* kmalloc_aligned(size_t size, size_t align)
{
    if (!heap_is_initialized() || size == 0) return NULL;

    uint32_t units_needed = (uint32_t)((size + (HEAP_UNIT - 1u)) / HEAP_UNIT);
    return heap_alloc_units_aligned(units_needed, (uint32_t)align, true);
}

void* kcalloc(size_t n, size_t size)
{
    if (!heap_is_initialized() || n == 0 || size == 0) return NULL;

    size_t total = n * size;
    if (total / n != size) return NULL; // overflow

    void* ptr = kmalloc(total);
    if (!ptr) return NULL;

    kmemset(ptr, 0, total);
    return ptr;
}

void kfree(void* ptr)
{
    if (!heap_is_initialized() || !ptr) return;

    uintptr_t addr = (uintptr_t)ptr;

    // aceitamos liberar apenas dentro da área ativa
    if (addr < heap_start_addr || addr >= heap_end_addr) {
#ifdef KHEAP_DEBUG
        kprintf("[kfree] ptr fora da heap: %p\n", ptr);
#endif
        return;
    }

    // exige alinhamento de unidade (evita ptr no meio)
    if (((addr - heap_start_addr) % HEAP_UNIT) != 0u) {
#ifdef KHEAP_DEBUG
        kprintf("[kfree] ptr desalinhado: %p\n", ptr);
#endif
        return;
    }

    uint32_t start_unit = (uint32_t)((addr - heap_start_addr) / HEAP_UNIT);
    uint32_t units      = heap_alloc_units[start_unit];

    if (units == 0) {
#ifdef KHEAP_DEBUG
        kprintf("[kfree] ptr %p nao eh inicio de bloco (double free/corrupcao)\n", ptr);
#endif
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
    if (!heap_is_initialized()) return NULL;

    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    uintptr_t addr = (uintptr_t)ptr;
    if (addr < heap_start_addr || addr >= heap_end_addr) return NULL;

    if (((addr - heap_start_addr) % HEAP_UNIT) != 0u) return NULL;

    uint32_t start_unit = (uint32_t)((addr - heap_start_addr) / HEAP_UNIT);
    uint32_t old_units  = heap_alloc_units[start_unit];
    if (old_units == 0) return NULL;

    size_t old_size_bytes = (size_t)old_units * (size_t)HEAP_UNIT;
    uint32_t new_units    = (uint32_t)((new_size + (HEAP_UNIT - 1u)) / HEAP_UNIT);

    // 1) cabe no bloco atual
    if (new_units <= old_units) {
        if (new_units == old_units) return ptr;

        uint32_t units_to_free = old_units - new_units;
        uint32_t start_free    = start_unit + new_units;

        for (uint32_t u = 0; u < units_to_free; ++u) {
            heap_set_unit_free(start_free + u);
        }

        heap_alloc_units[start_unit] = new_units;
        heap_free_units             += units_to_free;
        return ptr;
    }

    // 2) tentar crescer in-place
    uint32_t extra_units = new_units - old_units;
    uint32_t total_units = heap_current_total_units();

    if (start_unit + new_units <= total_units) {
        bool can_grow = true;
        uint32_t first_extra = start_unit + old_units;

        for (uint32_t u = 0; u < extra_units; ++u) {
            if (heap_unit_is_used(first_extra + u)) {
                can_grow = false;
                break;
            }
        }

        if (can_grow) {
            for (uint32_t u = 0; u < extra_units; ++u) {
                heap_set_unit_used(first_extra + u);
            }
            heap_alloc_units[start_unit] = new_units;
            heap_free_units             -= extra_units;
            return ptr;
        }
    }

    // 3) alocar novo e copiar
    void* new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;

    size_t copy_bytes = (new_size < old_size_bytes) ? new_size : old_size_bytes;
    kmemcpy(new_ptr, ptr, copy_bytes);

    kfree(ptr);
    return new_ptr;
}

void* kpage_alloc(void)
{
    return kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);
}

void* kpages_alloc(size_t num_pages)
{
    if (num_pages == 0) return NULL;
    return kmalloc_aligned(num_pages * (size_t)PAGE_SIZE, PAGE_SIZE);
}

// -----------------------------------------------------------------------------
// Estatísticas
// -----------------------------------------------------------------------------

size_t kheap_get_free_units(void)
{
    return heap_free_units;
}

size_t kheap_get_total_units(void)
{
    return (size_t)heap_current_total_units();
}
/**
 * Faz o mapeamento inicial da área de memória definida para a heap.
 */
void map_heap_initial(const paging_ctx_t* ctx, size_t bytes)
{
    uintptr_t va = KHEAP_BASE;
    uintptr_t end = KHEAP_BASE + ALIGN_UP(bytes, PAGE_SIZE);

    for (; va < end; va += PAGE_SIZE) {
        uintptr_t pa = pmm_alloc_frame();
        if (!pa) panic("OOM: heap init frames");

        // zera frame via kmap (recomendado)
        void* p = (void*)kmap(pa);
        kmemset(p, 0, PAGE_SIZE);
        kunmap();
        //invalid_tlb((uintptr_t)p);
        //kprintf("\nva=%p -> phys=%x",va,pa);


        if (paging_map(kernel_directory, ctx, va, pa, PAGE_RW) != 0) {
            panic("paging_map failed in heap init");
        }
    }
}


#ifdef KHEAP_DEBUG

void kheap_debug_dump_bitmap(void)
{
    if (!heap_is_initialized()) {
        kprintf("kheap not initialized\n");
        return;
    }

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

        for (uint32_t bit = 0; bit < 32u; ++bit) {
            uint32_t unit = w * 32u + bit;
            if (unit >= total_units) break;
            kprintf("%c", (word & (1u << bit)) ? '1' : '0');
        }
        kprintf("\n");
    }

    kprintf("=== fim do bitmap ===\n");
}

void kheap_debug_dump_blocks(void)
{
    if (!heap_is_initialized()) {
        kprintf("kheap not initialized\n");
        return;
    }

    uint32_t total_units = heap_current_total_units();

    kprintf("=== kheap blocks dump ===\n");
    kprintf("heap_start_addr = %p\n", (void*)heap_start_addr);
    kprintf("heap_end_addr   = %p\n", (void*)heap_end_addr);
    kprintf("total_units     = %u\n", total_units);
    kprintf("heap_free_units = %u\n", (uint32_t)heap_free_units);

    for (uint32_t u = 0; u < total_units; ) {
        uint32_t units = heap_alloc_units[u];
        if (units == 0) {
            ++u;
            continue;
        }

        uintptr_t addr = heap_start_addr + (uintptr_t)u * HEAP_UNIT;
        uint32_t bytes = units * HEAP_UNIT;
        bool used = heap_unit_is_used(u);

        kprintf("  bloco @ %p: start_unit=%u, units=%u, bytes=%u, used=%s\n",
                (void*)addr, u, units, bytes, used ? "yes" : "no");

        u += units;
    }

    kprintf("=== fim dos blocos ===\n");
}



#endif /* KHEAP_DEBUG */
