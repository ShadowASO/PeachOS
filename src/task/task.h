#ifndef TASK_H
#define TASK_H

#include "../kernel.h"
#include "../config.h"
#include "../mm/page/paging.h"

typedef struct registers
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} PACKED_FIELDS registers_t;

typedef struct task
{
    page_directory_t *pdir;
    registers_t regs;
    task_t *next;
    task_t *prev;

} PACKED_FIELDS task_t;

task_t * task_new();
task_t * task_current();
task_t * task_get_next();
int task_free(task_t * task);

#endif
