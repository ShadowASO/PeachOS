#include "bootmem.h"
#include "../cpu/e820.h"
#include "../klib/kprintf.h"

//Limites para alocar memória, antes do setup da 
//heap
static uint32_t boot_early_start = 0;
static uint32_t boot_early_max   = 0;
static uint32_t boot_early_curr  = 0;
static bool     boot_early_inited = false;

//Início do kernel
extern uint32_t _kernel_ini_vmm;
extern uint32_t _kernel_ini_phys;
//Final do kernel
extern uint32_t _kernel_end_vmm;
extern uint32_t _kernel_end_phys;

//Variáveis globais de memória
static uint32_t *kernel_ini_vmm = NULL;
static uint32_t *kernel_end_vmm = NULL;

static uint32_t kernel_ini_phys = 0;
static uint32_t kernel_end_phys = 0;

static uint64_t phys_mem_size=0;
static size_t kernel_size=0;

void make_memory_map(e820_address_t *e820_address) {
     e820_memory_init(e820_address);
}

void bootmem_init(e820_address_t *e820_address) {
    //Identifica a memória disponível no sistema    
    make_memory_map(e820_address);
    //Virtual
    kernel_ini_vmm=(void *)&_kernel_ini_vmm;
    kernel_end_vmm=(void *)&_kernel_end_vmm;
    //Físico
    kernel_ini_phys=(uint32_t)&_kernel_ini_phys;
    kernel_end_phys=(uint32_t)&_kernel_end_phys;

    //Tamanho do kernel
    kernel_size=kernel_end_phys-kernel_ini_phys;

    //Tamanho total da memória
    phys_mem_size=e820_memory_size();

   
}
void *get_kernel_ini_vmm(){
    return kernel_ini_vmm;
}
void *get_kernel_end_vmm(){
    return kernel_end_vmm;
}
uint32_t get_kernel_ini_phys(){
    return kernel_ini_phys;
}
uint32_t get_kernel_end_phys(){
    return kernel_end_phys;
}
uint64_t get_memory_size() {
    return phys_mem_size;
}
size_t get_kernel_size() {
    return kernel_size;
}
/**
 * Calcula e fixa os limites da memória para permitir a posterior alocação, 
 * que inicial em aligned_start e termina em aligned_end
 */
void boot_early_init(uint32_t start, uint32_t size_bytes) {
    // Garante alinhamento do início
    uint32_t aligned_start = ALIGN_UP(start, boot_HEAP_ALIGNMENT);  

    uint32_t end = start + size_bytes;            // <- isso é o certo
    end = ALIGN_DOWN(end, boot_HEAP_ALIGNMENT);   // alinhe o END

    if (end <= aligned_start) {
        boot_early_inited = false;
        return;
    }

    boot_early_start  = aligned_start;
    boot_early_curr   = aligned_start;
    boot_early_max    = end;
    boot_early_inited = true;

    debug_early_init() ;

}

void debug_early_init() {
    kprintf("\nboot_early_start=0x%x", boot_early_start);   
    kprintf("\nboot_early_curr=0x%x", boot_early_curr);   
    kprintf("\nboot_early_max=0x%x", boot_early_max);   
   
}

void* boot_early_kalloc(size_t size, size_t align) {

    if ((align & (align - 1)) != 0) return NULL;

    if (align == 0) align = boot_HEAP_ALIGNMENT;


    if (!boot_early_inited || size == 0 ) {
        return NULL;
    }
    

    //uint32_t aligned_size = ALIGN_UP((uint32_t)size, boot_HEAP_ALIGNMENT);
    uintptr_t curr = boot_early_curr;
    // alinhamento
    uintptr_t aligned = (curr + (align - 1)) & ~(align - 1);
    
    // Verifica se cabe
    if (aligned + size > boot_early_max) {
        // Sem espaço na early heap
        
        kprintf("\nsize=0x%x", size); 
        kprintf("\nboot_early_curr=0x%x", boot_early_curr); 
        kprintf("\nboot_early_curr aligned=0x%x", aligned ); 
        kprintf("\nboot_early_curr aligned + size=0x%x", aligned + size); 
        kprintf("\nboot_early_end=0x%x", boot_early_max); 

        return NULL;
    }

    void* ptr = (void*)aligned;
    boot_early_curr = aligned + size;
    return ptr;
}

void* boot_early_kcalloc(size_t size, size_t align) {
    //size_t total = n * size;
    void* ptr = boot_early_kalloc(size, align);
    if (!ptr) {
        return NULL;
    }

    // Zera a memória
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = 0;
    }
    return ptr;
}

void boot_early_kfree(void* ptr) {
    // Early allocator não suporta free de blocos individuais.
    // Você pode deixar isso como no-op ou, se quiser, adicionar
    // algum tipo de assert/log de uso indevido.
    (void)ptr;
}

void boot_early_reset(void) {
    if (!boot_early_inited) return;
    boot_early_curr = boot_early_start;
}

uint32_t boot_early_used(void) {
    if (!boot_early_inited) return 0;
    return boot_early_curr - boot_early_start;
}

uint32_t boot_early_total(void) {
    if (!boot_early_inited) return 0;
    return boot_early_max - boot_early_start;
}

bool boot_early_is_initialized(void) {
    return boot_early_inited;
}

uint32_t boot_early_phys_end(void) {
    if (!boot_early_inited) return 0;
    return boot_early_curr;
}