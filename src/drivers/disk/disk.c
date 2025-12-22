#include <stdint.h>
#include "../../io/io.h"
#include "disk.h"

#include "../../klib/string.h"
#include "../../klib/memory.h"
#include "../../klib/kprintf.h"
#include "disk.h"
#include "../../fs/file.h"

/* Portas padrão do canal primário */
#define ATA_REG_DATA        0x1F0
#define ATA_REG_ERROR       0x1F1
#define ATA_REG_SECCOUNT0   0x1F2
#define ATA_REG_LBA0        0x1F3
#define ATA_REG_LBA1        0x1F4
#define ATA_REG_LBA2        0x1F5
#define ATA_REG_HDDEVSEL    0x1F6
#define ATA_REG_STATUS      0x1F7
#define ATA_REG_COMMAND     0x1F7

/* Bits de status */
#define ATA_SR_ERR  0x01
#define ATA_SR_DRQ  0x08
#define ATA_SR_SRV  0x10
#define ATA_SR_DF   0x20
#define ATA_SR_RDY  0x40
#define ATA_SR_BSY  0x80

/* Comandos */
#define ATA_CMD_READ_PIO        0x20

/* Se quiser evitar loop infinito, use um timeout simples */
#define ATA_MAX_POLL  1000000

static inline uint8_t disk_status(void) {
    return __read_portb(ATA_REG_STATUS);
}

static inline void ata_400ns_delay(void) {
    // 400ns = 4 leituras do status no canal
    (void)disk_status();
    (void)disk_status();
    (void)disk_status();
    (void)disk_status();
}

static int disk_wait_bsy_clear(void) {
    uint32_t count = 0;
    while (disk_status() & ATA_SR_BSY) {
        if (++count > ATA_MAX_POLL) return -1;
    }
    return 0;
}

static int disk_wait_drq_set(void) {
    uint32_t count = 0;
    while (1) {
        uint8_t st = disk_status();

        if (st & ATA_SR_ERR) return -2;
        if (st & ATA_SR_DF)  return -3;

        // DRQ deve vir com BSY=0
        if ((st & ATA_SR_DRQ) && !(st & ATA_SR_BSY)) return 0;

        if (++count > ATA_MAX_POLL) return -1;
    }
}

struct disk_driver disk;

/**
 * Faz a inicialização do disk.
 */
void disk_search_and_init(void) {
    kmemset(&disk, 0, sizeof(disk));
    disk.type = DISK_TYPE_REAL;
    disk.sector_size = DISK_SECTOR_SIZE;
    disk.id = 0;
    disk.filesystem = fs_resolve(&disk);

    //kprintf("\ndisk.filesystem=%p", disk.filesystem);
    //kprintf("\ndisk.sector_size=%d", disk.sector_size);
}

struct disk_driver* disk_get(int index)
{
    if (index != 0) return 0;
    return &disk;
}

int disk_read_block(struct disk_driver *idisk, uint32_t lba, size_t total, void *buf)
{
    if (!idisk) {
        kprintf("\nERROR - disk driver nulo");
        return -1;
    }

    // Se quiser debug seguro:
    // kprintf("\nidisk=%p type=%u sector_size=%d lba=%u total=%u",
    //        idisk, (unsigned)idisk->type, (int)idisk->sector_size,
    //        (unsigned)lba, (unsigned)total);


    uint8_t* p = (uint8_t*)buf;
    uint32_t cur_lba = lba;
    size_t remaining = total;

    while (remaining > 0) {
        size_t chunk = remaining > 256 ? 256 : remaining;
        int r = disk_read_sector28(cur_lba, (uint16_t)chunk, p);
        if (r != 0) return r; // mantém códigos negativos/erros

        cur_lba += (uint32_t)chunk;
        p += (chunk * DISK_SECTOR_SIZE);
        remaining -= chunk;
    }

    // ✅ bytes lidos
    return (int)(total * DISK_SECTOR_SIZE);
}

/* ---------------------------------------------------------
 * Helpers de status
 * --------------------------------------------------------- */

/* ---------------------------------------------------------
 * Leitura LBA28, múltiplos setores (até 256)
 * --------------------------------------------------------- */

int disk_read_sector28(uint32_t lba, uint16_t sectors, void *buf)
{
    if (sectors == 0) return 0;

    // LBA28 suporta até 0x0FFFFFFF
    if (lba > 0x0FFFFFFF) return -4;

    if (disk_wait_bsy_clear() != 0) return -1;

    __write_portb(ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    ata_400ns_delay();

    // Em ATA: 0 => 256 setores. Vamos programar assim:
    uint8_t sc = (sectors == 256) ? 0 : (uint8_t)sectors;

    __write_portb(ATA_REG_SECCOUNT0, sc);
    __write_portb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    __write_portb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    __write_portb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    __write_portb(ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    uint16_t *ptr = (uint16_t*)buf;
    uint16_t count = (sectors == 256) ? 256 : sectors;

    for (uint16_t s = 0; s < count; s++) {
        int r = disk_wait_drq_set();
        if (r != 0) return r;

        for (int i = 0; i < 256; i++) {
            *ptr++ = __read_portw(ATA_REG_DATA);
        }
    }

    return 0;
}

