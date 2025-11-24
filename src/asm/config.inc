; ----------------------------------------------------
;   Base física e base virtual
; ----------------------------------------------------

%define KERNEL_PHYS_BASE   0x00100000
%define KERNEL_VIRT_BASE   0xC0000000
%define PAGE_SIZE          4096

%define KERNEL_OFFSET      (KERNEL_VIRT_BASE - KERNEL_PHYS_BASE)
