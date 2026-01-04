#include "paging.h"
#include "../pmm.h"
#include "../../klib/memory.h"
#include "../../klib/kprintf.h"
#include "../../cpu/cpu.h"
#include "paging_kmap.h"

extern void paging_load_directory(uint32_t phys);
extern void paging_enable(void);

page_directory_t* current_directory = NULL;
page_directory_t* kernel_directory  = NULL;

static inline uintptr_t va_to_pa(const paging_ctx_t* ctx, uintptr_t vaddr)
{
    if (ctx && ctx->virt_to_phys) return ctx->virt_to_phys(vaddr);
    return vaddr; // bootstrap identity
}

static inline page_directory_t * get_current_page_directory() {
    if (!current_directory) 
        return NULL;
    return current_directory;

}

uintptr_t virt_to_phys_paging(uintptr_t virt)
{
    uint32_t pd_idx = (virt >> 22) & 0x3FF;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;
    uint32_t offset = virt & 0xFFF;

    uint32_t *page_directory = (uint32_t *)get_current_page_directory();
    if (!(page_directory[pd_idx] & PAGE_PRESENT)) {
        return 0; // não mapeado
    }

    uint32_t *page_table = (uint32_t*)(page_directory[pd_idx] & 0xFFFFF000);
    if (!(page_table[pt_idx] & PAGE_PRESENT)) {
        return 0; // não mapeado
    }

    uint32_t phys_page = page_table[pt_idx] & 0xFFFFF000;
    return phys_page + offset;
}


static inline void invlpg(uintptr_t va) {
    asm volatile("invlpg (%0)" :: "r"(va) : "memory");
}

/* cria diretório (4KiB) */
page_directory_t* paging_create_directory(const paging_ctx_t* ctx)
{
    if (!ctx || !ctx->alloc_page_aligned) for(;;);

    uintptr_t dir_v = ctx->alloc_page_aligned(sizeof(page_directory_t), PAGE_SIZE);
    if (!dir_v) for(;;);

    page_directory_t* dir = (page_directory_t*)dir_v;
    kmemset(dir, 0, sizeof(page_directory_t));
    return dir;
}

/* obtém PT física a partir do PDE */
static inline uintptr_t pde_pt_phys(uint32_t pde_entry)
{
    return (uintptr_t)(pde_entry & 0xFFFFF000u);
}

/* garante que exista PT para o VA: cria sob demanda
 * - antes do paging ON: cria via ctx->alloc_page_aligned e zera direto
 * - depois do paging ON: cria via pmm_alloc_frame e zera via kmap
 */
static uintptr_t ensure_page_table(page_directory_t* dir, const paging_ctx_t* ctx,
                                   uintptr_t virt, uint32_t pde_flags,
                                   int paging_is_on)
{
    uint32_t di = PAGE_DIR_INDEX(virt);
    uint32_t pde = dir->pde[di];

    if (pde & PAGE_PRESENT) {
        return pde_pt_phys(pde);
    }

    uintptr_t pt_phys = 0;

    if (!paging_is_on) {
        // bootstrap: aloca acessível diretamente
        uintptr_t pt_v = ctx->alloc_page_aligned(sizeof(page_table_t), PAGE_SIZE);
        if (!pt_v) for(;;);
        kmemset((void*)pt_v, 0, sizeof(page_table_t));
        pt_phys = va_to_pa(ctx, pt_v);
    } else {
        // runtime: aloca frame físico e zera via kmap
        pt_phys = pmm_alloc_frame();
        if (!pt_phys) for(;;);
        page_table_t* pt = (page_table_t*)kmap(pt_phys);
        kmemset(pt, 0, sizeof(page_table_t));
        kunmap();
    }

    dir->pde[di] = (uint32_t)((pt_phys & 0xFFFFF000u) |
                             (pde_flags & 0xFFFu) |
                             PAGE_PRESENT);

    return pt_phys;
}

/* API: map */
int paging_map(page_directory_t* dir, const paging_ctx_t* ctx,
               uintptr_t virt, uintptr_t phys, uint32_t flags)
{
    if (!dir || !ctx) return -1;

    // Você pode marcar se paging já está ON por uma flag global; aqui assumo:
    // depois de paging_init_minimal, você só chama paging_map com paging ativo.
    // Se quiser suportar antes, passe isso como argumento ou faça global.
    int paging_is_on = 1;

    uint32_t pde_flags = PAGE_RW;
    if (flags & PAGE_USER) pde_flags |= PAGE_USER;

    uintptr_t pt_phys = ensure_page_table(dir, ctx, virt, pde_flags, paging_is_on);

    // acessa PT via kmap
    page_table_t* pt = (page_table_t*)kmap(pt_phys);

    uint32_t ti = PAGE_TABLE_INDEX(virt);
    pt->entries[ti] = (uint32_t)((phys & 0xFFFFF000u) |
                                (flags & 0xFFFu) |
                                PAGE_PRESENT);

    kunmap();
    invlpg(virt);

    return 0;
}

int paging_unmap(page_directory_t* dir, const paging_ctx_t* ctx, uintptr_t virt)
{
    (void)ctx;
    if (!dir) return -1;

    uint32_t di = PAGE_DIR_INDEX(virt);
    uint32_t pde = dir->pde[di];
    if (!(pde & PAGE_PRESENT)) return 0;

    uintptr_t pt_phys = pde_pt_phys(pde);
    page_table_t* pt = (page_table_t*)kmap(pt_phys);

    uint32_t ti = PAGE_TABLE_INDEX(virt);
    pt->entries[ti] = 0;

    kunmap();
    invlpg(virt);

    return 0;
}

uintptr_t paging_get_physical(page_directory_t* dir, const paging_ctx_t* ctx, uintptr_t virt)
{
    (void)ctx;
    if (!dir) return 0;

    uint32_t di = PAGE_DIR_INDEX(virt);
    uint32_t pde = dir->pde[di];
    if (!(pde & PAGE_PRESENT)) return 0;

    uintptr_t pt_phys = pde_pt_phys(pde);
    page_table_t* pt = (page_table_t*)kmap(pt_phys);

    uint32_t ti = PAGE_TABLE_INDEX(virt);
    uint32_t e = pt->entries[ti];

    kunmap();

    if (!(e & PAGE_PRESENT)) return 0;
    return (uintptr_t)((e & 0xFFFFF000u) | (virt & 0xFFFu));
}

/* init minimal:
 * - cria kernel_directory
 * - inicializa KMAP (PT reservada) ainda no bootstrap
 * - identity-map só até bootstrap_identity_limit (sem 4GiB!)
 * - mapeia kernel high-half
 * - liga paging
 */
void paging_init_minimal(const paging_ctx_t* ctx)
{
    if (!ctx || !ctx->alloc_page_aligned) for(;;);

    kernel_directory = paging_create_directory(ctx);
    current_directory = kernel_directory;

    // prepara KMAP (precisa do kernel_directory existente)
    paging_kmap_init(kernel_directory, ctx);

    uint32_t kflags = (ctx->kernel_page_flags & 0xFFFu) | PAGE_RW;

    // identity limit
    uintptr_t id_limit = ctx->bootstrap_identity_limit;
    if (id_limit == 0) id_limit = 64u * 1024u * 1024u;
    id_limit = (id_limit / PAGE_SIZE) * PAGE_SIZE;

    // Durante init (paging OFF), criamos PTs via alloc_page_aligned (sem kmap)
    for (uintptr_t phys = 0; phys < id_limit; phys += PAGE_SIZE) {
        // aqui paging_is_on=0 implicitamente, então usamos ensure_page_table bootstrap:
        uint32_t pde_flags = PAGE_RW;
        uintptr_t pt_phys = ensure_page_table(kernel_directory, ctx, phys, pde_flags, 0);

        // pt é acessível diretamente porque alocado via alloc_page_aligned (bootstrap identity)
        page_table_t* pt = (page_table_t*)pt_phys; // válido no bootstrap identity
        uint32_t ti = PAGE_TABLE_INDEX(phys);
        pt->entries[ti] = (uint32_t)((phys & 0xFFFFF000u) | (kflags & 0xFFFu) | PAGE_PRESENT);
    }

    // map kernel high-half
    for (uintptr_t phys = ctx->kernel_phys_start; phys < ctx->kernel_phys_end; phys += PAGE_SIZE) {
        uintptr_t virt = ctx->kernel_virt_base + (phys - ctx->kernel_phys_start);

        uint32_t pde_flags = PAGE_RW;
        uintptr_t pt_phys = ensure_page_table(kernel_directory, ctx, virt, pde_flags, 0);

        // bootstrap identity: pt_phys acessível direto
        page_table_t* pt = (page_table_t*)pt_phys;
        uint32_t ti = PAGE_TABLE_INDEX(virt);
        pt->entries[ti] = (uint32_t)((phys & 0xFFFFF000u) | (kflags & 0xFFFu) | PAGE_PRESENT);
    }

    // carrega CR3 com físico do diretório
    uint32_t cr3_phys = (uint32_t)va_to_pa(ctx, (uintptr_t)kernel_directory);
    paging_load_directory(cr3_phys);
    paging_enable();

    // daqui em diante: use paging_map/unmap/get_physical (elas usam kmap)
}

/* troca diretório */
void paging_switch_directory(page_directory_t* dir, const paging_ctx_t* ctx)
{
    if (!dir) return;
    current_directory = dir;
    uint32_t cr3_phys = (uint32_t)va_to_pa(ctx, (uintptr_t)dir);
    paging_load_directory(cr3_phys);
}

/* cria diretório user copiando o high-half do kernel */
page_directory_t* paging_create_user_directory_from_kernel(const paging_ctx_t* ctx,
                                                           const page_directory_t* kdir)
{
    page_directory_t* udir = paging_create_directory(ctx);

    // copia apenas high-half: assume kernel_virt_base alinhado 4MiB (ex.: 0xC0000000)
    uint32_t start = PAGE_DIR_INDEX(ctx->kernel_virt_base);
    for (uint32_t i = start; i < 1024u; ++i) {
        udir->pde[i] = kdir->pde[i];
    }

    return udir;
}

int user_map_pages(page_directory_t* udir, const paging_ctx_t* ctx,
                   uintptr_t uva_start, size_t size, uint32_t flags)
{
    uintptr_t start = uva_start & ~(PAGE_SIZE - 1);
    uintptr_t end   = (uva_start + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    for (uintptr_t va = start; va < end; va += PAGE_SIZE) {
        uintptr_t pa = pmm_alloc_frame();
        if (!pa) return -1;

        // zera frame via kmap (boa prática)
        void* p = (page_table_t*)kmap(pa);
        kmemset(p, 0, PAGE_SIZE);
        kunmap();

        if (paging_map(udir, ctx, va, pa, flags | PAGE_USER | PAGE_PRESENT) != 0) {
            // (ideal: liberar pa e desfazer mappings anteriores)
            return -1;
        }
    }
    return 0;
}
