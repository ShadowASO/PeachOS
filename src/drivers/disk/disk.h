// disk.h
#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stddef.h>

#define DISK_SECTOR_SIZE 512
#define DISK_TYPE_REAL 0

typedef uint32_t DISK_TYPE;

struct disk_driver {
    DISK_TYPE type;
    int32_t sector_size;
};

//int disk_read_sector28(uint32_t lba, uint8_t sectors, void *buf);
int disk_read_sector28(uint32_t lba, uint16_t sectors, void *buf);

/* Wrapper compatível com sua função antiga */

struct disk_driver* disk_get(int index);

int disk_read_block(struct disk_driver *idisk, uint32_t lba, size_t total, void *buf);

static inline int disk_read_sector(int lba, int total, void *buf) {
    if (total <= 0) return 0;
    return disk_read_block(disk_get(0), (uint32_t)lba, (size_t)total, buf);
}
void disk_search_and_init(void);


#endif
