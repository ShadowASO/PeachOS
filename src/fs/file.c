/*
 * Copyright (C) Daniel McCarthy <daniel@dragonzap.com>
 * This file is licensed under the GPLv2 license.
 * To learn how to build this get the kernel development course at https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch
 */

#include "file.h"
#include "../config.h"
#include "../klib/memory.h"
#include "../mm/kheap.h"
#include "../klib/string.h"
#include "../klib/kprintf.h"
#include "../drivers/disk/disk.h"
#include "../status.h"
#include "../kernel.h"
#include "./fat/fat16.h"
#include "../klib/error.h"


struct filesystem* filesystems[MAX_FILESYSTEMS];
struct file_descriptor* file_descriptors[MAX_FILEDESCRIPTORS];

static struct filesystem** fs_get_free_filesystem()
{
    int i = 0;
    for (i = 0; i < MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }

    return 0;
}

void fs_insert_filesystem(struct filesystem* filesystem)
{
    struct filesystem** fs = fs_get_free_filesystem();
    if (!fs)
    {
        kprintf("Problem inserting filesystem");
        while (1) {}
    }
    *fs = filesystem;
}

static void fs_static_load()
{
    fs_insert_filesystem(fat16_init());
}

void fs_load()
{
    kmemset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

void fs_init()
{
    kmemset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

static void file_free_descriptor(struct file_descriptor* desc)
{
    file_descriptors[desc->index-1] = 0x00;
    kfree(desc);
}



static int file_new_descriptor(struct file_descriptor** desc_out)
{
    if (!desc_out) return -EINVARG;

    for (int i = 0; i < MAX_FILEDESCRIPTORS; i++)
    {
        if (file_descriptors[i] == 0)
        {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));
            if (!desc)
                return -ENOMEM;

            desc->index = i + 1; // descritores começam em 1
            file_descriptors[i] = desc;
            *desc_out = desc;
            return 0;
        }
    }

    return -ENOMEM; // ou -ENFILE se você tiver
}

static struct file_descriptor* file_get_descriptor(int fd)
{
    // Válidos: 1..MAX_FILEDESCRIPTORS
    if (fd <= 0 || fd > MAX_FILEDESCRIPTORS)
        return 0;

    return file_descriptors[fd - 1];
}

struct filesystem* fs_resolve(struct disk_driver* disk)
{
    for (int i = 0; i < MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] && filesystems[i]->resolve(disk) == 0)
            return filesystems[i];
    }
    return 0;
}

FILE_MODE file_get_mode_by_string(const char* str)
{
    FILE_MODE mode = FILE_MODE_INVALID;
    if (kstrncmp(str, "r", 1) == 0)
    {
        mode = FILE_MODE_READ;
    }
    else if(kstrncmp(str, "w", 1) == 0)
    {
        mode = FILE_MODE_WRITE;
    }
    else if(kstrncmp(str, "a", 1) == 0)
    {
        mode = FILE_MODE_APPEND;
    }
    return mode;
}

int fopen(const char* filename, const char* mode_str)
{
    int res = 0;
    struct disk_driver* disk = 0;
    FILE_MODE mode = FILE_MODE_INVALID;
    void* descriptor_private_data = 0;
    struct file_descriptor* desc = 0;
    struct path_root* root_path = 0;

    if (!filename || !mode_str)
        return 0;

    root_path = pathparser_parse(filename, 0);
    if (!root_path)
    {
        kprintf("\n[fopen] pathparser_parse falhou");
        res = -EINVARG;
        goto out;
    }

    if (!root_path->first)
    {
        kprintf("\n[fopen] caminho sem arquivo/parte: %s", filename);
        res = -EINVARG;
        goto out;
    }

    disk = disk_get(root_path->drive_no);
    if (!disk)
    {
        kprintf("\n[fopen] disk_get(%d) falhou", root_path->drive_no);
        res = -EIO;
        goto out;
    }

    // Se filesystem ainda não foi resolvido, resolva agora
    if (!disk->filesystem)
    {
        disk->filesystem = fs_resolve(disk);
        if (!disk->filesystem)
        {
            kprintf("\n[fopen] nenhum filesystem suportado no disco %d", root_path->drive_no);
            res = -EFSNOTUS;
            goto out;
        }
    }

    mode = file_get_mode_by_string(mode_str);
    if (mode == FILE_MODE_INVALID)
    {
        kprintf("\n[fopen] modo invalido: %s", mode_str);
        res = -EINVARG;
        goto out;
    }

    descriptor_private_data = disk->filesystem->open(disk, root_path->first, mode);
    if (ISERR(descriptor_private_data))
    {
        res = ERROR_I(descriptor_private_data);
        kprintf("\n[fopen] open() falhou: %d", res);
        goto out;
    }

    res = file_new_descriptor(&desc);
    if (res < 0)
    {
        kprintf("\n[fopen] sem descritores: %d", res);
        goto out;
    }

    desc->filesystem = disk->filesystem;
    desc->private_data = descriptor_private_data;
    desc->disk = disk;

    res = desc->index;

out:
    // Sempre liberar o path parseado
    if (root_path)
    {
        pathparser_free(root_path);
        root_path = 0;
    }

    // Em erro, limpar o que foi alocado/aberto
    if (res < 0)
    {
        // Se você tiver close no FS, o correto é fechar aqui.
        // Ex: disk->filesystem->close(descriptor_private_data);
        descriptor_private_data = 0;

        if (desc)
        {
            // Se você quiser liberar desc, reative file_free_descriptor.
            // Por ora, evita “slot perdido” caso tenha sido reservado.
            file_descriptors[desc->index - 1] = 0;
            kfree(desc);
            desc = 0;
        }

        // mantendo sua convenção: fopen não retorna negativo
        return 0;
    }

    return res;
}

int fclose(int fd)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->close(desc->private_data);
    if (res == PEACHOS_ALL_OK)
    {
        file_free_descriptor(desc);
    }
out:
    return res;
}

int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd)
{
    int res = 0;
    if (size == 0 || nmemb == 0 || fd < 1)
    {
        res = -EINVARG;
        goto out;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        res = -EINVARG;
        goto out;
    }

    res = desc->filesystem->read(desc->disk, desc->private_data, size, nmemb, (char*) ptr);
out:
    return res;
}

int fseek(int fd, int offset, FILE_SEEK_MODE whence)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->seek(desc->private_data, offset, whence);
out:
    return res;
}

int fstat(int fd, struct file_stat* stat)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->stat(desc->disk, desc->private_data, stat);
out:
    return res;
}