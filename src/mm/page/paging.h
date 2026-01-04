#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096u
#endif

#ifndef PAGE_TABLE_ENTRIES
#define PAGE_TABLE_ENTRIES 1024u
#endif

/* Flags */
#define PAGE_PRESENT   0x001
#define PAGE_RW        0x002
#define PAGE_USER      0x004
#define PAGE_WRITETHRU 0x008
#define PAGE_NOCACHE   0x010
#define PAGE_ACCESSED  0x020
#define PAGE_DIRTY     0x040
#define PAGE_4MB       0x080
#define PAGE_GLOBAL    0x100

#define PAGE_DIR_INDEX(vaddr)   ((((uint32_t)(vaddr)) >> 22) & 0x3FFu)
#define PAGE_TABLE_INDEX(vaddr) ((((uint32_t)(vaddr)) >> 12) & 0x3FFu)

typedef uint32_t page_entry_t;

typedef struct page_table {
    page_entry_t entries[PAGE_TABLE_ENTRIES];
} page_table_t;

/* Page Directory deve caber em 4KiB (1 frame) */
typedef struct page_directory {
    uint32_t pde[PAGE_TABLE_ENTRIES]; /* PDEs (PT phys + flags) */
} page_directory_t;

typedef struct paging_ctx {
    // alocador para estruturas no bootstrap (normalmente boot_early_kalloc)
    uintptr_t (*alloc_page_aligned)(size_t bytes, size_t align);

    // converter VA->PA do kernel (se você estiver high-half)
    // no bootstrap identity, pode ser NULL (assumimos VA==PA)
    uintptr_t (*virt_to_phys)(uintptr_t vaddr);

    // limite de identity-map durante init (ex.: 64MiB)
    uintptr_t bootstrap_identity_limit;

    // base virtual do kernel high-half (ex.: 0xC0000000)
    uintptr_t kernel_virt_base;

    // range físico do kernel
    uintptr_t kernel_phys_start;
    uintptr_t kernel_phys_end;

    // flags padrão para kernel pages (ex.: PAGE_RW)
    uint32_t kernel_page_flags;
} paging_ctx_t;

/* diretórios globais */
extern page_directory_t* current_directory;
extern page_directory_t* kernel_directory;

uintptr_t virt_to_phys_paging(uintptr_t virt);

/* init minimal + kmap pronto */
void paging_init_minimal(const paging_ctx_t* ctx);

/* troca CR3 */
void paging_switch_directory(page_directory_t* dir, const paging_ctx_t* ctx);

/* map/unmap genérico (cria PT sob demanda, usando kmap quando paging já estiver ON) */
int  paging_map(page_directory_t* dir, const paging_ctx_t* ctx,
                uintptr_t virt, uintptr_t phys, uint32_t flags);

int  paging_unmap(page_directory_t* dir, const paging_ctx_t* ctx,
                  uintptr_t virt);

uintptr_t paging_get_physical(page_directory_t* dir, const paging_ctx_t* ctx,
                              uintptr_t virt);

/* criação de diretório (para userland também) */
page_directory_t* paging_create_directory(const paging_ctx_t* ctx);

/* cria diretório user copiando o “high-half” do kernel */
page_directory_t* paging_create_user_directory_from_kernel(const paging_ctx_t* ctx,
                                                           const page_directory_t* kdir);

int user_map_pages(page_directory_t* udir, 
                    const paging_ctx_t* ctx,
                   uintptr_t uva_start, 
                   size_t size, uint32_t flags);

#endif
