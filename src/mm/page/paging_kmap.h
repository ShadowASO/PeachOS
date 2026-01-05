#ifndef PAGING_KMAP_H
#define PAGING_KMAP_H

#include <stdint.h>

// #define KERNEL_VIRT_BASE 0xC0000000u
// #define KERNEL_PHYS_BASE 0x00100000u

//---------------------------------------------------- */
#define KERNEL_PHYS_BASE  0x00100000      /* Bootloader carrega aqui */

/* Kernel executa aqui     */
#define KERNEL_VIRT_BASE  0xC0000000   

#define KERNEL_OFFSET    (KERNEL_VIRT_BASE - KERNEL_PHYS_BASE)

#define KMAP_VA 0xFFC00000u

void     paging_kmap_init(page_directory_t* kdir, const paging_ctx_t* ctx);
uintptr_t kmap(uintptr_t phys);
void     kunmap(void);

static inline uintptr_t virt_to_phys_highhalf(uintptr_t virt)
{
    if (virt < KERNEL_VIRT_BASE) {
        // endereço identity-mapped (early / low memory)
        return virt;
    }

    return virt - KERNEL_VIRT_BASE + KERNEL_PHYS_BASE;
}




#endif
