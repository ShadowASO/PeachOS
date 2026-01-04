#include "paging.h"
#include "paging_kmap.h"
#include "../../klib/memory.h"
#include "../../klib/kprintf.h"

/* Essa PT é acessível por ponteiro normal (alocada no bootstrap identity) */
static page_table_t* g_kmap_pt = NULL;

static inline void invlpg(uintptr_t va) {
    asm volatile("invlpg (%0)" :: "r"(va) : "memory");
}

/* Inicializa a page table do KMAP:
 * - aloca uma PT no bootstrap (identity)
 * - seta PDE do KMAP no diretório do kernel
 * - NÃO usa kmap internamente (evita recursão)
 */
void paging_kmap_init(page_directory_t* kdir, const paging_ctx_t* ctx)
{
    if (!kdir || !ctx || !ctx->alloc_page_aligned) for(;;);

    // aloca PT acessível no bootstrap (VA==PA ou ctx->virt_to_phys)
    uintptr_t pt_v = ctx->alloc_page_aligned(sizeof(page_table_t), PAGE_SIZE);
    if (!pt_v) for(;;);

    g_kmap_pt = (page_table_t*)pt_v;
    kmemset(g_kmap_pt, 0, sizeof(page_table_t));

    uintptr_t pt_phys;
    if (ctx->virt_to_phys) pt_phys = ctx->virt_to_phys(pt_v);
    else pt_phys = pt_v; // identity

    uint32_t di = PAGE_DIR_INDEX(KMAP_VA);

    // PDE aponta para PT do KMAP
    kdir->pde[di] = (uint32_t)((pt_phys & 0xFFFFF000u) | PAGE_PRESENT | PAGE_RW);
}

/* Mapeia qualquer PA em KMAP_VA (1 página) */
uintptr_t kmap(uintptr_t phys)
{
    if (!g_kmap_pt) for(;;);

    phys &= 0xFFFFF000u;

    uint32_t ti = PAGE_TABLE_INDEX(KMAP_VA);
    g_kmap_pt->entries[ti] = (uint32_t)(phys | PAGE_PRESENT | PAGE_RW);

    invlpg(KMAP_VA);
    return KMAP_VA;
}

void kunmap(void)
{
    if (!g_kmap_pt) return;

    uint32_t ti = PAGE_TABLE_INDEX(KMAP_VA);
    g_kmap_pt->entries[ti] = 0;

    invlpg(KMAP_VA);
}
