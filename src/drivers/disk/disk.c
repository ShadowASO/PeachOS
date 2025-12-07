#include "../../io/io.h"

#define LBA_SECTOR_COUNT 0x1F2
#define LBA_BLOCK_LOW 0x1F3
#define LBA_BLOCK_MID 0x1F4
#define LBA_BLOCK_HIGH 0x1F5
#define LBA_DRIVE_HEAD 0x1F6
#define LBA_STATUS_COMMAND 0x1F7

int disk_read_sector(int lba, int total, void * buf) {
    __write_portb(LBA_DRIVE_HEAD, (lba >> 24) | 0xE0);
    __write_portb(LBA_SECTOR_COUNT, total);
    __write_portb(LBA_BLOCK_LOW , (uint8_t)(lba & 0xFF));
    __write_portb(LBA_BLOCK_MID, (uint8_t)(lba >> 8));
    __write_portb(LBA_BLOCK_HIGH, (uint8_t)(lba >> 16));
    __write_portb(LBA_STATUS_COMMAND, 0x20);

    unsigned short * ptr = (unsigned short *) buf;
    for (int b=0; b < total; b++) {
        char c= __read_portb(0x1F7);

        while (!(c & 0x08))
        {
            c= __read_portb(0x1F7);
        }
        for(int i=0; i <256; i++) {
            *ptr=__read_portb(0x1F0);
            ptr++;
        }        
    }
    return 0;
}