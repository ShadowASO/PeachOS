#ifndef PAGING_H
#define PAGING_H

#include <stddef.h>
#include <stdint.h>

#define PAGING_CACHE_DISABLED 0b00010000
#define PAGING_WRITE_THROUGH 0b00001000
#define PAGING_ACCESS_FROM_ALL 0b00000100
#define PAGING_IS_WRITEABLE 0b00000010
#define PAGING_IS_PRESENT 0b00000001

#define TABLE_ENTRIES_LEN 1024
#define PAGE_SIZE 4096

struct directory_page {
    uint32_t * directory_entry;
};

struct directory_page * new_directory_page(uint8_t flags);
void paging_switch(uint32_t * directory);
uint32_t * paging_get_directory_page(struct directory_page * directory);

void load_directory_table(void *pt_directory);
void enable_paging(void);



#endif