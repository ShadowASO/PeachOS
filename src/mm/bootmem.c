#include "bootmem.h"
#include "../cpu/e820.h"


static uint32_t boot_heap_start = 0;
static uint32_t boot_heap_end   = 0;
static uint32_t boot_heap_curr  = 0;
static bool     boot_heap_inited = false;

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

static size_t phys_mem_size=0;
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
size_t get_memory_size() {
    return phys_mem_size;
}
size_t get_kernel_size() {
    return kernel_size;
}
void boot_heap_init(uint32_t start, uint32_t size) {
    // Garante alinhamento do início
    uint32_t aligned_start = ALIGN_UP(start, boot_HEAP_ALIGNMENT);
    uint32_t aligned_end   = start + size;

    if (aligned_end <= aligned_start) {
        // Range inválido, em produção você pode chamar panic()
        // ou logar erro.
        boot_heap_inited = false;
        return;
    }

    boot_heap_start  = aligned_start;
    boot_heap_curr   = aligned_start;
    boot_heap_end    = aligned_end;
    boot_heap_inited = true;
}

void* boot_kmalloc(size_t size) {
    if (!boot_heap_inited || size == 0) {
        return NULL;
    }

    uint32_t aligned_size = ALIGN_UP((uint32_t)size, boot_HEAP_ALIGNMENT);

    // Verifica se cabe
    if (boot_heap_curr + aligned_size > boot_heap_end) {
        // Sem espaço na early heap
        return NULL;
    }

    void* ptr = (void*)boot_heap_curr;
    boot_heap_curr += aligned_size;
    return ptr;
}

void* boot_kcalloc(size_t n, size_t size) {
    size_t total = n * size;
    void* ptr = boot_kmalloc(total);
    if (!ptr) {
        return NULL;
    }

    // Zera a memória
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < total; i++) {
        p[i] = 0;
    }
    return ptr;
}

void boot_kfree(void* ptr) {
    // Early allocator não suporta free de blocos individuais.
    // Você pode deixar isso como no-op ou, se quiser, adicionar
    // algum tipo de assert/log de uso indevido.
    (void)ptr;
}

void boot_heap_reset(void) {
    if (!boot_heap_inited) return;
    boot_heap_curr = boot_heap_start;
}

uint32_t boot_heap_used(void) {
    if (!boot_heap_inited) return 0;
    return boot_heap_curr - boot_heap_start;
}

uint32_t boot_heap_total(void) {
    if (!boot_heap_inited) return 0;
    return boot_heap_end - boot_heap_start;
}

bool boot_heap_is_initialized(void) {
    return boot_heap_inited;
}