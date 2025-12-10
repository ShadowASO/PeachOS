// disk.h
#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#define DISK_SECTOR_SIZE 512
#define DISK_TYPE_REAL 0

typedef uint32_t DISK_TYPE;

struct disk_driver {
    DISK_TYPE type;
    int32_t sector_size;
};

int disk_read_sector28(uint32_t lba, uint8_t sectors, void *buf);

/* Wrapper compatível com sua função antiga */
static inline int disk_read_sector(int lba, int total, void *buf) {
    return disk_read_sector28((uint32_t)lba, (uint8_t)total, buf);
}

#endif
