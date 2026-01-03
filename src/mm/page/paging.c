// paging.c
#include "paging.h"
#include "../pmm.h"              // pmm_alloc_frame / pmm_free_frame
#include "../../klib/memory.h"   // memset
#include "../bootmem.h"       // ajuste o path conforme sua árvore real
#include "../../klib/kprintf.h"
#include "../../cpu/cpu.h"

extern void paging_load_directory(uint32_t phys);
extern void paging_enable(void);

page_directory_t* current_directory = NULL;
static page_directory_t* kernel_directory = NULL;

/* Cria uma nova tabela de páginas para o diretório 'dir', na posição 'index'.
 * Usa o boot_early_kalloc para alocar uma página alinhada.
 * table_phys é ao mesmo tempo virtual e físico (enquanto estamos em identity map).
 */
static page_table_t* create_page_table(page_directory_t* dir, uint32_t index, uint32_t flags)
{
    // Aloca 4 KiB alinhados para a page table
    uintptr_t table_phys = (uintptr_t)boot_early_kalloc(sizeof(page_table_t), PAGE_SIZE);
    if (!table_phys) {
        // TODO: substituir por panic()
        for (;;);
    }

    // Em identity map, phys == virt
    page_table_t* table = (page_table_t*) table_phys;
    kmemset(table, 0, sizeof(page_table_t));

    // Ponteiro virtual para facilitar acesso em C
    dir->tables[index] = table;

    // PDE: endereço físico da tabela + flags
    dir->tables_phys[index] = (table_phys & 0xFFFFF000) | (flags & 0xFFF);

    return table;
}

/* Retorna ponteiro para tabela de páginas correspondente a 'virt'.
 * Se não existir e 'create' = 1, cria uma nova tabela.
 */
static page_table_t* get_page_table(page_directory_t* dir,
                                    uintptr_t virt,
                                    int create,
                                    uint32_t flags)
{
    uint32_t dir_index = PAGE_DIR_INDEX(virt);
    page_table_t* table = dir->tables[dir_index];

    if (!table && create) {
        table = create_page_table(dir, dir_index, flags);
    }

    return table;
}

/* Mapeia virt -> phys no diretório indicado (assumimos current_directory). */
void paging_map(uintptr_t virt, uintptr_t phys, uint32_t flags)
{
    page_directory_t* dir = current_directory;
    if (!dir) {
        // Paginação ainda não inicializada
        for (;;);
    }

    // Flags do PDE: herdamos bits relevantes (USER, etc.) + PRESENT | RW
    uint32_t pde_flags = (flags & 0xFFF) | PAGE_PRESENT | PAGE_RW;

    page_table_t* table =
        get_page_table(dir, virt, 1, pde_flags);

    uint32_t table_index = PAGE_TABLE_INDEX(virt);

    // Entrada da page table: base física + flags da página
    page_entry_t entry = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;
    table->entries[table_index] = entry;

    // Opcional: flush da TLB dessa página (quando já estivermos com paging ativo)
    // invlpg(virt);  // se você tiver um wrapper em assembly
}

/* Desmapeia a página virtual (não libera frame físico). */
void paging_unmap(uintptr_t virt)
{
    page_directory_t* dir = current_directory;
    if (!dir) return;

    page_table_t* table = get_page_table(dir, virt, 0, 0);
    if (!table) return;

    uint32_t table_index = PAGE_TABLE_INDEX(virt);
    table->entries[table_index] = 0;

    // Opcional: invlpg(virt);
}

/* Pega o endereço físico associado à página virtual (se mapeada). */
static uintptr_t get_physical(uintptr_t virt)
{
    page_directory_t* dir = current_directory;
    if (!dir) return 0;

    uint32_t dir_index = PAGE_DIR_INDEX(virt);
    uint32_t tbl_index = PAGE_TABLE_INDEX(virt);

    page_table_t* table = dir->tables[dir_index];
    if (!table) return 0;

    page_entry_t entry = table->entries[tbl_index];
    if (!(entry & PAGE_PRESENT)) return 0;

    return (entry & 0xFFFFF000) | (virt & 0xFFF);
}

/* Aloca um frame físico e mapeia em 'virt'. */
uintptr_t vmm_alloc_page(uintptr_t virt, uint32_t flags)
{
    uintptr_t frame = pmm_alloc_frame();
    if (!frame) {
        // out-of-memory
        for (;;);
    }

    paging_map(virt, frame, flags);
    return frame;
}

/* Desaloca a página virtual: desmapeia e libera frame físico. */
void vmm_free_page(uintptr_t virt)
{
    uintptr_t phys = get_physical(virt);
    if (phys) {
        paging_unmap(virt);
        pmm_free_frame(phys & 0xFFFFF000);
    }
}

/* Cria um diretório de páginas vazio.
 * Supõe identity mapping por enquanto (phys == virt).
 *
 * IMPORTANTE:
 *  - page_directory_t tem ~8 KiB → precisamos alocar sizeof(page_directory_t),
 *    alinhado em PAGE_SIZE.
 *  - CR3 deve apontar para o array de PDEs (tables_phys), não para o início do struct.
 */
static page_directory_t* create_page_directory(void)
{
    //kprintf("\nentrei direc1");

    uintptr_t dir_virt = (uintptr_t)boot_early_kalloc(sizeof(page_directory_t), PAGE_SIZE);
    //kprintf("\nentrei direc2 %x", dir_virt);
    if (!dir_virt) {
        for (;;);
    }

    page_directory_t* dir = (page_directory_t*) dir_virt;
    kmemset(dir, 0, sizeof(page_directory_t));

    // Enquanto estamos em identity map, virtual == físico
    // CR3 deve receber o endereço físico do array de PDEs (tables_phys)
    dir->phys_addr = (uintptr_t)&dir->tables_phys[0];

    return dir;
}

/* Inicializa a paginação:
 * - identity map de toda a RAM (0 .. mem_size_mb)
 * - mapeia o kernel em high-half (kernel_virt_base)
 */
void paging_init(uint32_t mem_size_bytes,
                 uintptr_t kernel_phys_start,
                 uintptr_t kernel_phys_end,
                 uintptr_t kernel_virt_base)
{
    
    // Cria diretório do kernel
    kernel_directory = create_page_directory();
    current_directory = kernel_directory;

    uint32_t flags = PAGE_PRESENT | PAGE_RW;

    /* 1) Identity map da RAM física 0 .. mem_size_mb*1MiB */
    
    uintptr_t mem_end = (uintptr_t) ALIGN_DOWN(mem_size_bytes, PAGE_SIZE);
    
    kprintf("\n**mem_end %u",mem_end );    

    for (uintptr_t phys = 0; phys < mem_end; phys += PAGE_SIZE) {
        paging_map(phys, phys, flags);
    }
   
    /* 2) Mapeia a região do kernel em high-half */
    for (uintptr_t phys = kernel_phys_start; phys < kernel_phys_end; phys += PAGE_SIZE) {
        uintptr_t offset = phys - kernel_phys_start;
        uintptr_t virt   = kernel_virt_base + offset;
        paging_map(virt, phys, flags);
    }
    //kprintf("\n**kernel_phys_end %u",kernel_phys_end );
   
    /* Carrega CR3 com o endereço físico do array de PDEs */
    paging_load_directory(kernel_directory->phys_addr);

     /* Liga o bit PG no CR0 */
    paging_enable();

    // A partir daqui, você pode executar no endereço virtual do kernel.
}

void paging_switch_directory(page_directory_t* dir)
{
    if (!dir) return;

    current_directory = dir;
    paging_load_directory(dir->phys_addr);
}
int paging_table_entry_set(uint32_t * directory, void * virtual_address, uint32_t frame_addr) {

    size_t directory_index=paging_directory_index(virtual_address);
    size_t table_index=paging_table_index(virtual_address);

    uint32_t directory_entry =directory[directory_index];
    uint32_t *table = (uint32_t*)(directory_entry & 0xFFFFF000);
    table[table_index]=frame_addr;

    return 0;

}
