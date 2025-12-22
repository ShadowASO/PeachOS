#/*
 * Copyright (C) Daniel McCarthy <daniel@dragonzap.com>
 * This file is licensed under the GPLv2 license.
 * To learn how to build this get the kernel development course at https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch
 */

#include "path_parser.h"
#include "../kernel.h"
#include "../klib/ctype.h"
#include "../klib/string.h"
#include "../mm/kheap.h"
#include "../klib/memory.h"
#include "../status.h"
#include "path.h"


static int pathparser_path_valid_format(const char* filename)
{
    int len = kstrnlen(filename, PATH_MAX);
    return (len >= 3 && isdigit(filename[0]) && kmemcmp((void*)&filename[1], ":/", 2) == 0);
}

static int pathparser_get_drive_by_path(const char** path)
{
    if(!pathparser_path_valid_format(*path))
    {
        return -EBADPATH;
    }

    //int drive_no = tonumericdigit(*path[0]);
    int drive_no = atoi(*path[0]);

    // Add 3 bytes to skip drive number 0:/ 1:/ 2:/
    *path += 3;
    return drive_no;
}

static struct path_root* pathparser_create_root(int drive_number)
{
    struct path_root* path_r = kmalloc(sizeof(struct path_root));
    if (!path_r)
    {
        return NULL;
    }

    path_r->drive_no = drive_number;
    path_r->first = 0;
    return path_r;
}


static const char* pathparser_get_path_part(const char** path)
{
    // 1) Descobre o tamanho do token (até '/' ou '\0')
    const char* start = *path;
    int len = 0;

    while (start[len] != '/' && start[len] != 0x00)
        len++;

    // Token vazio? (ex: "0:/" ou "0://")
    if (len == 0)
    {
        // Se está em '/', consome pra evitar loop travado
        if (**path == '/')
            *path += 1;
        return 0;
    }

    // 2) Aloca exatamente o necessário (+1 pro '\0')
    char* result = kmalloc(len + 1);
    if (!result)
        return 0;

    // 3) Copia e termina com '\0'
    kmemcpy(result, *path, len);
    result[len] = 0x00;

    // 4) Avança o ponteiro do path
    *path += len;
    if (**path == '/')
        *path += 1;

    return result;
}


struct path_part* pathparser_parse_path_part(struct path_part* last_part, const char** path)
{
    const char* path_part_str = pathparser_get_path_part(path);
    if (!path_part_str)
    {
        return 0;
    }

    struct path_part* part = kmalloc(sizeof(struct path_part));
    if (!part)
    {
        kfree((void*)path_part_str);
        return 0;
    }
    part->part = path_part_str;
    part->next = 0x00;

    if (last_part)
    {
        last_part->next = part;
    }

    return part;
}

void pathparser_free(struct path_root* root)
{
    struct path_part* part = root->first;
    while(part)
    {
        struct path_part* next_part = part->next;
        kfree((void*) part->part);
        kfree(part);
        part = next_part;
    }

    kfree(root);
}

struct path_root* pathparser_parse(const char* path, const char* current_directory_path)
{
    int res = 0;
    const char* tmp_path = path;
    struct path_root* path_root = NULL;
    struct path_part* first_part = NULL;
    struct path_part* part = NULL;

    //if (strlen(path) > PEACHOS_MAX_PATH)
    if (kstrlen(path) > PATH_MAX)
    {
        res = -1;
        goto out;
    }

    res = pathparser_get_drive_by_path(&tmp_path);
    if (res < 0)
    {
        res = -1;
        goto out;
    }

    path_root = pathparser_create_root(res);
    if (!path_root)
    {
        res = -1;
        goto out;
    }

    first_part = pathparser_parse_path_part(NULL, &tmp_path);
    if (!first_part)
    {
        res = -1;
        goto out;
    }

    path_root->first = first_part;
    part  = pathparser_parse_path_part(first_part, &tmp_path);
    while(part)
    {
        part = pathparser_parse_path_part(part, &tmp_path);
    }
    
out:
    if (res < 0)
    {
        if (path_root)
        {
            kfree(path_root);
            path_root = NULL;
        }
        if (first_part)
        {
            kfree(first_part);
        }
    }
    return path_root;
}