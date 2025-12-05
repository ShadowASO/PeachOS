/* pmm.c - Implementação do Physical Memory Manager baseado em bitmap */

#include "pmm.h"
#include "bootmem.h"


/* Bitmap global. 
 * Cada bit representa um frame de PMM_FRAME_SIZE bytes. */
// uint32_t g_pmm_bitmap[PMM_BITMAP_SIZE_U32] = {0};
uint32_t *g_pmm_bitmap=NULL;

/* Quantidade de frames livres. */
size_t g_pmm_free_frames = 0;

/* Quantidade total de frames realmente utilizáveis
 * (derivada do total_phys_mem_bytes passado em pmm_init). */
static size_t g_pmm_total_frames = 0;

/* Converte endereço físico -> índice de frame */
static inline size_t pmm_addr_to_frame(uintptr_t addr)
{
    return (size_t)(addr / PMM_FRAME_SIZE);
}

/* Converte índice de frame -> endereço físico base */
static inline uintptr_t pmm_frame_to_addr(size_t frame_idx)
{
    return (uintptr_t)(frame_idx * (size_t)PMM_FRAME_SIZE);
}

void pmm_mark_regions_status() {
    //Marca regiões identificadas pelo e820
    size_t count=e820_regions_count();
    for(size_t i=0; i < count; i++) {
        phys_region_t *region=e820_region_by_index(i);
        if (region->type != E820_TYPE_USABLE){
            uintptr_t base_phys=(uintptr_t)region->base;
            size_t end_phys=region->length;
            pmm_mark_region_used(base_phys,end_phys);
        }            
    }
    //Marcar a região utilizada pelo kernel
    pmm_mark_region_used(get_kernel_ini_phys(), get_kernel_size());

}

void pmm_init(uint32_t *bitmap_ini, uint32_t phys_mem_size)
{
    //size_t phys_mem_size=get_memory_size();
    // g_pmm_bitmap=get_kernel_end_vmm();
    g_pmm_bitmap=bitmap_ini;

    /* Calcula quantos frames realmente cabem na memória física detectada. */
    //g_pmm_total_frames = total_phys_mem_bytes / PMM_FRAME_SIZE;
    g_pmm_total_frames = phys_mem_size / PMM_FRAME_SIZE;
    if (g_pmm_total_frames > PMM_MAX_FRAMES) {
        g_pmm_total_frames = PMM_MAX_FRAMES;
    }

    /* Zera completamente o bitmap (todos bits = 0 => todos frames LIVRES). */
    for (size_t i = 0; i < PMM_BITMAP_SIZE_U32; ++i) {
        g_pmm_bitmap[i] = 0u;
    }
    //Faz a marcação das área utilizadas
    pmm_mark_regions_status();

    /* Inicialmente, consideramos todos os frames possíveis como livres,
     * mas só fazemos contagem até g_pmm_total_frames. */
    g_pmm_free_frames = g_pmm_total_frames;
}


/* Marca frames como usados em [base_phys, base_phys + length) */
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
            if (g_pmm_free_frames > 0) {
                --g_pmm_free_frames;
            }
        }
    }
}

/* Marca frames como livres em [base_phys, base_phys + length) */
void pmm_mark_region_free(uintptr_t base_phys, size_t length)
{
    if (length == 0) return;

    uintptr_t end = base_phys + length;

    size_t first_frame = pmm_addr_to_frame(base_phys);
    size_t last_frame  = pmm_addr_to_frame(end - 1);  /* inclusivo */

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

/* Aloca o primeiro frame livre (primeiro bit 0 no bitmap) */
uintptr_t pmm_alloc_frame(void)
{
    if (g_pmm_free_frames == 0) {
        return 0;  /* sem memória livre */
    }

    for (size_t word = 0; word < PMM_BITMAP_SIZE_U32; ++word) {
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
