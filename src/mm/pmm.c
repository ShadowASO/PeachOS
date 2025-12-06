/* pmm.c - Implementação do Physical Memory Manager baseado em bitmap */

#include "pmm.h"
#include "bootmem.h"

/* Bitmap global.
 * Cada bit representa um frame de PMM_FRAME_SIZE bytes.
 */
uint32_t *g_pmm_bitmap = NULL;

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

size_t pmm_calc_bitmap_size_bytes(size_t phys_mem_size)
{
    size_t frames = phys_mem_size / PMM_FRAME_SIZE;
    if (frames > PMM_MAX_FRAMES)
        frames = PMM_MAX_FRAMES;

    size_t words = (frames + 31u) / 32u;
    return words * sizeof(uint32_t);
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

void pmm_mark_regions_status(void)
{
    /* Marca regiões NÃO USÁVEIS do E820 como usadas. */
    size_t count = e820_regions_count();
    for (size_t i = 0; i < count; i++) {
        phys_region_t *region = e820_region_by_index(i);

        if (region->type != E820_TYPE_USABLE) {
            uintptr_t base_phys = (uintptr_t)region->base;
            size_t   length    = (size_t)region->length; // cuidado: trunc em 32 bits
            pmm_mark_region_used(base_phys, length);
        }
    }

    /* Opcional: nunca use o frame 0 (para pegar ponteiros NULOS, BIOS, etc.) */
    pmm_mark_region_used(0, PMM_FRAME_SIZE);

    /* Marca a região ocupada pelo kernel como usada. */
    pmm_mark_region_used(get_kernel_ini_phys(), get_kernel_size());
    
    /* Marcar a área do bitmap */
    pmm_mark_region_used((uintptr_t)g_pmm_bitmap, g_pmm_bitmap_size);
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

void pmm_init(uint32_t *bitmap_ini, uint32_t phys_mem_size)
{
    g_pmm_bitmap = bitmap_ini;

    /* total de frames que cabem na memória física detectada */
    g_pmm_total_frames = phys_mem_size / PMM_FRAME_SIZE;
    if (g_pmm_total_frames > PMM_MAX_FRAMES) {
        g_pmm_total_frames = PMM_MAX_FRAMES;
    }

    /* quantos bits precisamos (1 por frame) -> converte para words de 32 bits */
    g_pmm_bitmap_words = (g_pmm_total_frames + 31u) / 32u;
    g_pmm_bitmap_size  = g_pmm_bitmap_words * sizeof(uint32_t);

    /* zera só a parte efetivamente usada do bitmap */
    for (size_t i = 0; i < g_pmm_bitmap_words; ++i) {
        g_pmm_bitmap[i] = 0u;
    }

    /* inicialmente consideramos todos os frames como livres */
    g_pmm_free_frames = g_pmm_total_frames;

    /* agora marcamos as regiões usadas (E820 + kernel),
       e cada marcação vai decrementando g_pmm_free_frames */
    pmm_mark_regions_status();
    //pmm_mark_control_structures(); // vou sugerir abaixo
    pmm_recalc_free_frames();
}



/* -----------------------------------------------------------
 * Marca regiões usadas / livres
 * ----------------------------------------------------------- */

void pmm_mark_region_used(uintptr_t base_phys, size_t length)
{
    if (length == 0) return;

    uintptr_t end = base_phys + length;

    size_t first_frame = pmm_addr_to_frame(base_phys);
    size_t last_frame  = pmm_addr_to_frame(end - 1);  /* inclusivo */

    if (last_frame >= g_pmm_total_frames) {
        last_frame = g_pmm_total_frames - 1;
    }

    for (size_t f = first_frame; f <= last_frame; ++f) {
        if (!PMM_TEST_BIT(g_pmm_bitmap, f)) {
            PMM_SET_BIT(g_pmm_bitmap, f);
            /* aqui decrementa SEM checar > 0,
               pois sabemos que estamos só marcando 0->1 */
            --g_pmm_free_frames;
        }
    }
}


void pmm_mark_region_free(uintptr_t base_phys, size_t length)
{
    if (length == 0) return;

    uintptr_t end = base_phys + length;

    size_t first_frame = pmm_addr_to_frame(base_phys);
    size_t last_frame  = pmm_addr_to_frame(end - 1);  /* inclusivo */

    if (first_frame >= g_pmm_total_frames) {
        return;
    }
    if (last_frame >= g_pmm_total_frames) {
        last_frame = g_pmm_total_frames - 1;
    }

    for (size_t f = first_frame; f <= last_frame; ++f) {
        if (PMM_TEST_BIT(g_pmm_bitmap, f)) {
            PMM_CLEAR_BIT(g_pmm_bitmap, f);
            ++g_pmm_free_frames;
        }
    }
}

/* -----------------------------------------------------------
 * Alocação / liberação de frames
 * ----------------------------------------------------------- */

uintptr_t pmm_alloc_frame(void)
{
    if (g_pmm_free_frames == 0) {
        return 0;  /* sem memória livre */
    }

    /* Percorre apenas os words realmente usados. */
    for (size_t word = 0; word < g_pmm_bitmap_words; ++word) {
        uint32_t value = g_pmm_bitmap[word];

        /* Se value == 0xFFFFFFFF, todos os 32 frames deste word já estão usados. */
        if (value == 0xFFFFFFFFu) {
            continue;
        }

        /* Ainda há pelo menos 1 bit zero aqui. Procura o primeiro bit 0. */
        for (uint32_t bit = 0; bit < 32u; ++bit) {
            uint32_t mask = 1u << bit;
            if (!(value & mask)) {
                size_t frame_idx = word * 32u + bit;

                if (frame_idx >= g_pmm_total_frames) {
                    /* Passou do limite real de frames disponíveis. */
                    return 0;
                }

                PMM_SET_BIT(g_pmm_bitmap, frame_idx);
                --g_pmm_free_frames;

                return pmm_frame_to_addr(frame_idx);
            }
        }
    }

    /* Se chegou aqui, não encontrou frame livre dentro dos frames válidos. */
    return 0;
}

void pmm_free_frame(uintptr_t frame_addr)
{
    size_t frame_idx = pmm_addr_to_frame(frame_addr);

    if (frame_idx >= g_pmm_total_frames) {
        /* Endereço inválido — poderia logar erro aqui. */
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
