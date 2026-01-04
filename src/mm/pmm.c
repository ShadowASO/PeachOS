/* pmm.c - Implementação do Physical Memory Manager baseado em bitmap */

#include "pmm.h"
#include "bootmem.h"
#include "../klib/kprintf.h"

/* Bitmap global.
 * Cada bit representa um frame de PMM_FRAME_SIZE bytes.
 */
uint32_t *g_pmm_bitmap = NULL;
static uint64_t g_pmm_bitmap_phys = 0;


/* Quantos frames existem e quantos ainda estão livres */
static size_t g_pmm_total_frames   = 0;
static size_t g_pmm_free_frames    = 0;

/* Tamanho real do bitmap em words e em bytes */
static size_t g_pmm_bitmap_words   = 0;  // quantidade de uint32_t efetivamente usados
static size_t g_pmm_bitmap_size    = 0;  // em bytes (útil p/ pmm_bitmap_end_addr)


/* -----------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------- */

static inline size_t pmm_addr_to_frame(uintptr_t addr)
{
    return (size_t)(addr / PMM_FRAME_SIZE);
}

static inline uintptr_t pmm_frame_to_addr(size_t frame_idx)
{
    return (uintptr_t)(frame_idx * (size_t)PMM_FRAME_SIZE);
}

static void pmm_mask_invalid_tail_bits(void)
{
    size_t rem = g_pmm_total_frames & 31u;
    if (rem == 0) return;

    uint32_t invalid_mask = 0xFFFFFFFFu << rem;
    g_pmm_bitmap[g_pmm_bitmap_words - 1] |= invalid_mask; // força inválidos como usados
}



size_t pmm_calc_bitmap_size_bytes(uint64_t phys_mem_size)
{
    uint64_t frames64 = 0;

    if (phys_mem_size > 0) {
        frames64 = (phys_mem_size + (uint64_t)PMM_FRAME_SIZE - 1) / (uint64_t)PMM_FRAME_SIZE;
    }

    if (frames64 > (uint64_t)PMM_MAX_FRAMES) {
        frames64 = (uint64_t)PMM_MAX_FRAMES;
    }

    size_t frames = (size_t)frames64;
    size_t words  = (frames + 31u) / 32u;
    size_t bytes  = words * sizeof(uint32_t);

    // kprintf("phys_mem_size=%x frames64=%x words=%x bytes=%x\n",
    //         (unsigned)phys_mem_size,
    //         (unsigned)frames64,
    //         (unsigned)words,
    //         (unsigned)bytes);

    // kprintf("PMM_FRAME_SIZE=%x PMM_MAX_FRAMES=%x\n",
    //         (unsigned)PMM_FRAME_SIZE,
    //         (unsigned)PMM_MAX_FRAMES);

    return bytes;
}


/* 
 * Retorna o endereço físico imediatamente após o bitmap,
 * alinhado a PMM_FRAME_SIZE. Este endereço será o
 * "heap_region_start" do seu layout da heap.
 *
 * [ heap_region_start ] ----------------------+
 *                                            |
 *                                            v
 *   +----------------------+  <- region_start (alinhado)
 *   |  bitmap (u32...)     |
 *   +----------------------+  <- pmm_bitmap_end_addr() (alinhado)
 *   |  heap_alloc_units[]  |
 *   ...
 */
uintptr_t pmm_bitmap_end_addr(void)
{
    if (!g_pmm_bitmap || g_pmm_bitmap_size == 0) {
        return 0; // ainda não inicializado
    }

    uintptr_t bitmap_end = (uintptr_t)g_pmm_bitmap + g_pmm_bitmap_size;
    return ALIGN_UP(bitmap_end, PMM_FRAME_SIZE);
}

/* Retorna quantidade de memória física livre em bytes. */
size_t pmm_get_free_memory_bytes(void)
{
    return g_pmm_free_frames * (size_t)PMM_FRAME_SIZE;
}

/* -----------------------------------------------------------
 * Marca regiões de acordo com E820 + kernel
 * ----------------------------------------------------------- */

static inline size_t pmm_addr_to_frame64(uint64_t addr) {
    return (size_t)(addr / PMM_FRAME_SIZE);
}

void pmm_mark_region_used64(uint64_t base_phys, uint64_t length)
{
    if (length == 0 || g_pmm_total_frames == 0) return;

    uint64_t limit_bytes = (uint64_t)g_pmm_total_frames * PMM_FRAME_SIZE;

    // clamp base dentro do limite
    if (base_phys >= limit_bytes) return;

    uint64_t end = base_phys + length;
    if (end > limit_bytes) end = limit_bytes;

    size_t first = pmm_addr_to_frame64(base_phys);
    size_t last  = pmm_addr_to_frame64(end - 1);

    for (size_t f = first; f <= last; ++f) {
        PMM_SET_BIT(g_pmm_bitmap, f);
    }
}

void pmm_mark_region_free64(uint64_t base_phys, uint64_t length)
{
    if (length == 0 || g_pmm_total_frames == 0) return;

    uint64_t limit_bytes = (uint64_t)g_pmm_total_frames * PMM_FRAME_SIZE;

    if (base_phys >= limit_bytes) return;

    uint64_t end = base_phys + length;
    if (end > limit_bytes) end = limit_bytes;

    size_t first = pmm_addr_to_frame64(base_phys);
    size_t last  = pmm_addr_to_frame64(end - 1);

    for (size_t f = first; f <= last; ++f) {
        PMM_CLEAR_BIT(g_pmm_bitmap, f);
    }
}


void pmm_mark_regions_status(void)
{
    size_t count = e820_regions_count();

    // Como o bitmap começa 0xFFFFFFFF (tudo usado),
    // aqui nós LIBERAMOS somente as regiões USABLE.
    for (size_t i = 0; i < count; i++) {
        phys_region_t *r = e820_region_by_index(i);

        uint64_t base = (uint64_t)r->base;
        uint64_t len  = (uint64_t)r->length;

        if (r->type == E820_TYPE_USABLE) {
            pmm_mark_region_free64(base, len);
        }
    }

    // Nunca use frame 0
    pmm_mark_region_used64(0, PMM_FRAME_SIZE);

    // Kernel em físico
    pmm_mark_region_used64((uint64_t)get_kernel_ini_phys(),
                           (uint64_t)get_kernel_size());

    // Bitmap: AQUI você precisa do endereço físico real do bitmap!
    // Se g_pmm_bitmap for VA (high-half), isso está errado.
    // Use g_pmm_bitmap_phys.
    pmm_mark_region_used64(g_pmm_bitmap_phys,
                           (uint64_t)g_pmm_bitmap_size);
}


static void pmm_recalc_free_frames(void)
{
    size_t used = 0;

    for (size_t word = 0; word < g_pmm_bitmap_words; ++word) {
        uint32_t v = g_pmm_bitmap[word];
        if (v == 0) {
            continue; // nenhum bit usado aqui
        }

        // conta bits 1 (popcount simples)
        for (int b = 0; b < 32; ++b) {
            if (v & (1u << b)) {
                size_t frame_idx = word * 32u + b;
                if (frame_idx < g_pmm_total_frames) {
                    used++;
                }
            }
        }
    }

    g_pmm_free_frames = g_pmm_total_frames - used;
}

/* -----------------------------------------------------------
 * Inicialização
 * ----------------------------------------------------------- */

void pmm_init(uint32_t *bitmap_ini, uint64_t phys_mem_size)
{
    if (!bitmap_ini) return;

    g_pmm_bitmap = bitmap_ini;
    //Endereço físico
    //g_pmm_bitmap_phys = (uint64_t)(uintptr_t)bitmap_ini;
    g_pmm_bitmap_phys = (uint64_t)virt_to_phys_kernel((uintptr_t)bitmap_ini);

    uint64_t frames64 = 0;
    if (phys_mem_size > 0) {
        frames64 = (phys_mem_size + (uint64_t)PMM_FRAME_SIZE - 1) / (uint64_t)PMM_FRAME_SIZE;
    }
    if (frames64 > (uint64_t)PMM_MAX_FRAMES) 
        frames64 = (uint64_t)PMM_MAX_FRAMES;



    g_pmm_total_frames = (size_t)frames64;
    g_pmm_bitmap_words = (g_pmm_total_frames + 31u) / 32u;
    g_pmm_bitmap_size  = g_pmm_bitmap_words * sizeof(uint32_t);

    // Começa tudo usado
    for (size_t i = 0; i < g_pmm_bitmap_words; ++i)
        g_pmm_bitmap[i] = 0xFFFFFFFFu;

    pmm_mask_invalid_tail_bits();

    // Libera regiões USABLE e depois trava kernel/bitmap/etc
    pmm_mark_regions_status();

    // Recalcula (e NÃO mexa em g_pmm_free_frames dentro das mark_* enquanto usar recalc)
    pmm_recalc_free_frames();
}



uintptr_t pmm_alloc_frame(void)
{
    if (g_pmm_free_frames == 0 || g_pmm_total_frames == 0) {
        return 0;
    }

    for (size_t word = 0; word < g_pmm_bitmap_words; ++word) {
        uint32_t value = g_pmm_bitmap[word];

        if (value == 0xFFFFFFFFu) {
            continue;
        }

        for (uint32_t bit = 0; bit < 32u; ++bit) {
            uint32_t mask = 1u << bit;
            if (!(value & mask)) {
                size_t frame_idx = word * 32u + bit;

                // Segurança extra (idealmente não precisa se você mascarar bits inválidos na init)
                if (frame_idx >= g_pmm_total_frames) {
                    break; // não "return 0" aqui
                }

                PMM_SET_BIT(g_pmm_bitmap, frame_idx);
                if (g_pmm_free_frames) --g_pmm_free_frames;

                return (uintptr_t)frame_idx * (uintptr_t)PMM_FRAME_SIZE;
            }
        }
    }

    return 0;
}


void pmm_free_frame(uintptr_t frame_addr)
{
    if (g_pmm_total_frames == 0) return;

    // Exige alinhamento de página/frame
    if (frame_addr & (PMM_FRAME_SIZE - 1)) {
        return; // ou log/panic
    }

    // Nunca libere frame 0
    if (frame_addr == 0) {
        return;
    }

    // (Opcional mas MUITO recomendado)
    // if (frame_addr >= kernel_start && frame_addr < kernel_end) return;
    // if (frame_addr >= bitmap_start && frame_addr < bitmap_end) return;

    size_t frame_idx = (size_t)(frame_addr / PMM_FRAME_SIZE);
    if (frame_idx >= g_pmm_total_frames) {
        return;
    }

    if (PMM_TEST_BIT(g_pmm_bitmap, frame_idx)) {
        PMM_CLEAR_BIT(g_pmm_bitmap, frame_idx);
        ++g_pmm_free_frames;
    }
}


size_t pmm_get_free_frame_count(void)
{
    return g_pmm_free_frames;
}
