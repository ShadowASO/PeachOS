// paging.h
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

/* Tamanho de página x86 (4 KiB) */

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096u
#endif

#ifndef PAGE_TABLE_ENTRIES
#define PAGE_TABLE_ENTRIES 1024
#endif

/* Flags de página / diretório */
#define PAGE_PRESENT   0x001
#define PAGE_RW        0x002
#define PAGE_USER      0x004
#define PAGE_WRITETHRU 0x008
#define PAGE_NOCACHE   0x010
#define PAGE_ACCESSED  0x020
#define PAGE_DIRTY     0x040
#define PAGE_4MB       0x080
#define PAGE_GLOBAL    0x100

/* Helpers para extrair índices */
#define PAGE_DIR_INDEX(vaddr)   (((vaddr) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(vaddr) (((vaddr) >> 12) & 0x3FF)

/* Tipo de uma entrada de página (PTE/PDE) */
typedef uint32_t page_entry_t;

/* Estrutura da Tabela de Páginas: 1024 entradas de 32 bits (4 KiB) */
typedef struct page_table {
    page_entry_t entries[1024];
} page_table_t;

/* Diretório de páginas:
 * - 'tables' guarda ponteiros virtuais para cada tabela (para uso em C)
 * - 'tables_phys' guarda o PDE (endereço físico + flags) usado pela CPU
 * - 'phys_addr' é o endereço físico do array tables_phys[0] (valor que vai para CR3)
 */
typedef struct page_directory {
    page_table_t* tables[1024];
    uint32_t      tables_phys[1024];
    uint32_t      phys_addr;
} page_directory_t;

/* Diretório atual (em uso pela CPU) */
extern page_directory_t* current_directory;

void paging_init(uint32_t mem_size_mb,
                 uintptr_t kernel_phys_start,
                 uintptr_t kernel_phys_end,
                 uintptr_t kernel_virt_base);

void paging_switch_directory(page_directory_t* dir);

void paging_map(uintptr_t virt, uintptr_t phys, uint32_t flags);
void paging_unmap(uintptr_t virt);

/* Aloca um frame físico e o mapeia em 'virt' com 'flags'.
 * Retorna o endereço físico do frame alocado.
 */
uintptr_t vmm_alloc_page(uintptr_t virt, uint32_t flags);

/* Desaloca a página virtual: desmapeia e libera o frame físico. */
void vmm_free_page(uintptr_t virt);

static inline size_t paging_directory_index(void *virtual_address) {
    return ((uint32_t)virtual_address /(PAGE_SIZE * PAGE_TABLE_ENTRIES));
}

static inline size_t paging_table_index(void *virtual_address) {
    uint32_t index=((uint32_t)virtual_address % (PAGE_SIZE * PAGE_TABLE_ENTRIES) / PAGE_SIZE);
    return index;
}

#endif // PAGING_H
