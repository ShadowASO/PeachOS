#include "../../io/io.h"
#include "disk.h"
#include <stdint.h>
#include "../../klib/string.h"
#include "../../klib/memory.h"
#include "disk.h"

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

struct disk_driver disk;


void disk_search_and_init(void) {
    memset(&disk,0, sizeof(disk));
    disk.type=DISK_TYPE_REAL;
    disk.sector_size=DISK_SECTOR_SIZE;

}

struct disk_driver * get_driver(int index) {
    if(index !=0) {
        return 0;
    }
    return &disk;
}

int disk_read_block(struct disk_driver *idisk, uint32_t lba, size_t total, void *buf) {
    if(idisk !=0) {
        return -1;
    }
    return disk_read_sector(lba, total, buf);
}
/* ---------------------------------------------------------
 * Helpers de status
 * --------------------------------------------------------- */

static inline uint8_t disk_status(void) {
    return __read_portb(ATA_REG_STATUS);
}

/* Espera BSY limpar (controladora pronta para aceitar comando/dados) */
static int disk_wait_bsy_clear(void) {
    uint32_t count = 0;
    while (disk_status() & ATA_SR_BSY) {
        if (count++ > ATA_MAX_POLL) {
            return -1;  // timeout
        }
    }
    return 0;
}

/* Espera DRQ set (dados prontos) e checa erro/DF */
static int disk_wait_drq_set(void) {
    uint32_t count = 0;
    while (1) {
        uint8_t st = disk_status();

        if (st & ATA_SR_ERR) {
            return -2;  // erro
        }
        if (st & ATA_SR_DF) {
            return -3;  // device fault
        }
        if (st & ATA_SR_DRQ) {
            return 0;   // pronto para transferir
        }

        if (count++ > ATA_MAX_POLL) {
            return -1;  // timeout
        }
    }
}

/* ---------------------------------------------------------
 * Leitura LBA28, múltiplos setores (até 256)
 * --------------------------------------------------------- */

int disk_read_sector28(uint32_t lba, uint8_t sectors, void *buf) {
    if (sectors == 0) {
        sectors = 1;   // 0 = 256 setores no protocolo; aqui vamos forçar 1
    }

    /* Espera o disco ficar livre antes de mandar comando novo */
    if (disk_wait_bsy_clear() != 0) {
        return -1;
    }

    /* Seleciona drive + bits altos do LBA (LBA28) */
    __write_portb(ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));

    /* Conta de setores */
    __write_portb(ATA_REG_SECCOUNT0, sectors);

    /* LBA baixo/médio/alto (24 bits baixos) */
    __write_portb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    __write_portb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    __write_portb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));

    /* Comando de leitura PIO */
    __write_portb(ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    uint16_t *ptr = (uint16_t*)buf;

    /* Para cada setor pedido... */
    for (uint8_t s = 0; s < sectors; s++) {
        int r = disk_wait_drq_set();
        if (r != 0) {
            return r;  // erro/timeout
        }

        /* Lê 256 words = 512 bytes do DATA */
        for (int i = 0; i < 256; i++) {
            *ptr++ = __read_portw(ATA_REG_DATA);
        }
    }

    return 0;
}
