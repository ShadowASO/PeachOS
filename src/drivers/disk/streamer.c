/*
 * Copyright (C) Daniel McCarthy <daniel@dragonzap.com>
 * This file is licensed under the GPLv2 license.
 * To learn how to build this get the kernel development course at https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch
 */

#include "streamer.h"
#include "../../mm/kheap.h"
#include "config.h"
#include <stdbool.h>
#include "disk.h"
#include "../../klib/kprintf.h"

struct disk_stream* diskstreamer_new(int disk_id)
{
    struct disk_driver* disk = disk_get(disk_id);
    if (!disk)
    {
        return 0;
    }

    struct disk_stream* streamer = kzalloc(sizeof(struct disk_stream));
    streamer->pos = 0;
    streamer->disk = disk;
    return streamer;
}

int diskstreamer_seek(struct disk_stream* stream, int pos)
{
    stream->pos = pos;
    return 0;
}

int diskstreamer_read(struct disk_stream* stream, void* out, int total)
{
    if (!stream || !stream->disk || !out) return -1;
    if (total <= 0) return 0;

    int remaining = total;
    uint8_t* outp = (uint8_t*)out;

    while (remaining > 0) {
        uint32_t lba    = (uint32_t)(stream->pos / DISK_SECTOR_SIZE);
        int      offset = (int)(stream->pos % DISK_SECTOR_SIZE);

        int take = DISK_SECTOR_SIZE - offset;
        if (take > remaining) take = remaining;

        // Se quiser debug seguro:
        // kprintf("\nidisk=%p type=%u sector_size=%d lba=%u total bytes=%u offset=%d take=%d",
        //    stream->disk, (unsigned)stream->disk->type, (int)stream->disk->sector_size,
        //    (unsigned)lba, (unsigned)total, offset, take);


        // Buffer do setor (512 bytes)
        uint8_t buf[DISK_SECTOR_SIZE];

        // Lê exatamente 1 setor (512 bytes)
        int res = disk_read_block(stream->disk, lba, 1, buf);

        // Erro real
        if (res < 0) return res;

        // Na opção B esperamos bytes lidos; 1 setor => 512
        if (res != DISK_SECTOR_SIZE) {
            // Se quiser: kprintf("\nWARN: disk_read_block retornou %d (esperado %d)", res, DISK_SECTOR_SIZE);
            return -2;
        }

        // Copia apenas a fatia necessária do setor para o output
        for (int i = 0; i < take; i++) {
            outp[i] = buf[offset + i];
        }

        outp += take;
        stream->pos += take;
        remaining -= take;
    }

    // Sucesso: retornamos quantos bytes o caller pediu (e foram entregues)
    return total;
}



void diskstreamer_close(struct disk_stream* stream)
{
    kfree(stream);
}