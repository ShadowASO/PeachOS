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
    struct filesystem** fs;
    fs = fs_get_free_filesystem();
    if (!fs)
    {
        kprintf("Problem inserting filesystem"); 
        while(1) {}
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

// static void file_free_descriptor(struct file_descriptor* desc)
// {
//     file_descriptors[desc->index-1] = 0x00;
//     kfree(desc);
// }

static int file_new_descriptor(struct file_descriptor** desc_out)
{
    int res = -ENOMEM;
    for (int i = 0; i < MAX_FILEDESCRIPTORS; i++)
    {
        if (file_descriptors[i] == 0)
        {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));
            // Descriptors start at 1
            desc->index = i + 1;
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }

    return res;
}

static struct file_descriptor* file_get_descriptor(int fd)
{
    if (fd <= 0 || fd >= MAX_FILEDESCRIPTORS)
    {
        return 0;
    }

    // Descriptors start at 1
    int index = fd - 1;
    return file_descriptors[index];
}

struct filesystem* fs_resolve(struct disk_driver* disk)
{
    struct filesystem* fs = 0;
    for (int i = 0; i < MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0)
        {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
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
    
    return -EIO;
}
