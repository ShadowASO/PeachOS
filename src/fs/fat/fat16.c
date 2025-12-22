#include "fat16.h"
#include "../../klib/error.h"
#include "../../klib/string.h"
#include "../../klib/kprintf.h"
#include "../../klib/memory.h"
#include "../../status.h"
#include "../file.h"
#include "../path_parser.h"
#include "../../drivers/disk/disk.h"
#include "../../drivers/disk/streamer.h"
#include "../../mm/kheap.h"
#include "../path.h"

static struct fat_directory* fat16_load_directory_from_cluster_chain(struct disk_driver* disk, int start_cluster);

struct filesystem fat16_fs =
{
    .resolve = fat16_resolve,   
    .open = fat16_open
};

struct filesystem * fat16_init() {
    kstrcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

// ---------------------------
// Helpers de IO (stream)
// ---------------------------
static int stream_read_exact(struct disk_stream* s, void* out, int total)
{
    int n = diskstreamer_read(s, out, total);
    if (n < 0) return n;
    if (n != total) return -EIO;
    return 0;
}

static int stream_seek_ok(struct disk_stream* s, int abs_pos)
{
    int r = diskstreamer_seek(s, abs_pos);
    return (r == PEACHOS_ALL_OK) ? 0 : -EIO;
}

// ---------------------------
// Helpers de FAT16 (cluster)
// ---------------------------
static inline int fat16_is_eoc(uint16_t v) { return v >= 0xFFF8; }
static inline int fat16_is_bad(uint16_t v) { return v == 0xFFF7; }
static inline int fat16_is_reserved(uint16_t v) { return (v >= 0xFFF0 && v <= 0xFFF6); }
static inline int fat16_is_free(uint16_t v) { return v == 0x0000; }

// ---------------------------
// Helpers de diretório (entry)
// ---------------------------
static inline int fat16_entry_is_end(const struct fat_directory_item* it)
{
    return it->filename[0] == 0x00;
}

static inline int fat16_entry_is_deleted(const struct fat_directory_item* it)
{
    return it->filename[0] == 0xE5;
}

static inline int fat16_entry_is_lfn(const struct fat_directory_item* it)
{
    return (it->attribute & 0x0F) == 0x0F;
}

static inline int fat16_entry_is_valid_83(const struct fat_directory_item* it)
{
    // Aceita somente entradas reais (8.3), ignora LFN/deletadas/end marker
    if (fat16_entry_is_end(it)) return 0;
    if (fat16_entry_is_deleted(it)) return 0;
    if (fat16_entry_is_lfn(it)) return 0;
    return 1;
}


static int fat16_init_private(struct disk_driver *disk, struct fat_private *fs_private)
{
    kmemset(fs_private, 0, sizeof(struct fat_private));

    fs_private->cluster_read_stream = diskstreamer_new(disk->id);
    fs_private->fat_read_stream     = diskstreamer_new(disk->id);
    fs_private->directory_stream    = diskstreamer_new(disk->id);

    if (!fs_private->cluster_read_stream || !fs_private->fat_read_stream || !fs_private->directory_stream)
        return -ENOMEM;

    return 0;
}

void fat16_to_proper_string(char **out, const char *in, size_t size)
{
    int i = 0;
    while (*in != 0x00 && *in != 0x20)
    {
        **out = *in;
        *out += 1;
        in += 1;
        // We cant process anymore since we have exceeded the input buffer size
        if (i >= size-1)
        {
            break;
        }
        i++;
    }

    **out = 0x00;
}

void fat16_get_full_relative_filename(struct fat_directory_item *item, char *out, int max_len)
{
    kmemset(out, 0x00, max_len);
    char *out_tmp = out;
    fat16_to_proper_string(&out_tmp, (const char *)item->filename, sizeof(item->filename));
    if (item->ext[0] != 0x00 && item->ext[0] != 0x20)
    {
        *out_tmp++ = '.';
        fat16_to_proper_string(&out_tmp, (const char *)item->ext, sizeof(item->ext));
    }
}

struct fat_directory_item *fat16_clone_directory_item(struct fat_directory_item *item, int size)
{
    struct fat_directory_item *item_copy = 0;
    if (size < sizeof(struct fat_directory_item))
    {
        return 0;
    }

    item_copy = kzalloc(size);
    if (!item_copy)
    {
        return 0;
    }

    kmemcpy(item_copy, item, size);
    return item_copy;
}

static uint32_t fat16_get_first_cluster(struct fat_directory_item *item)
{
    return (item->high_16_bits_first_cluster << 16) | item->low_16_bits_first_cluster;
};

int fat16_sector_to_absolute(struct disk_driver *disk, int sector)
{
    return sector * disk->sector_size;
}

static int fat16_cluster_to_sector(struct fat_private *private, int cluster)
{
    return private->root_directory.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

static uint32_t fat16_get_first_fat_sector(struct fat_private *private)
{
    return private->header.primary_header.reserved_sectors;
}

static int fat16_get_fat_entry(struct disk_driver *disk, int cluster)
{
    struct fat_private *private = disk->fs_private;
    struct disk_stream *stream = private->fat_read_stream;
    if (!stream) return -EIO;

    uint32_t fat_table_position = fat16_get_first_fat_sector(private) * disk->sector_size;

    // FAT16: entry = 2 bytes
    if (stream_seek_ok(stream, fat_table_position + (cluster * FAT16_FAT_ENTRY_SIZE)) < 0)
        return -EIO;

    uint16_t result = 0;
    int r = stream_read_exact(stream, &result, sizeof(result));
    if (r < 0) return r;

    return (int)result;
}

/**
 * Gets the correct cluster to use based on the starting cluster and the offset
 */
static int fat16_get_cluster_for_offset(struct disk_driver *disk, int starting_cluster, int offset)
{
    struct fat_private *private = disk->fs_private;
    int cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;

    int cluster_to_use = starting_cluster;
    int clusters_ahead = offset / cluster_bytes;

    for (int i = 0; i < clusters_ahead; i++)
    {
        int entry = fat16_get_fat_entry(disk, cluster_to_use);
        if (entry < 0) return entry;

        uint16_t v = (uint16_t)entry;

        if (fat16_is_eoc(v)) return -EIO;        // passou do fim
        if (fat16_is_bad(v)) return -EIO;
        if (fat16_is_reserved(v)) return -EIO;
        if (fat16_is_free(v)) return -EIO;

        cluster_to_use = entry;
    }

    return cluster_to_use;
}


static int fat16_read_internal_from_stream(struct disk_driver *disk, struct disk_stream *stream,
                                          int cluster, int offset, int total, void *out)
{
    struct fat_private *private = disk->fs_private;
    int cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;

    int cluster_to_use = fat16_get_cluster_for_offset(disk, cluster, offset);
    if (cluster_to_use < 0) return cluster_to_use;

    int offset_from_cluster = offset % cluster_bytes;

    int starting_sector = fat16_cluster_to_sector(private, cluster_to_use);
    int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster;

    int to_read = (total > (cluster_bytes - offset_from_cluster)) ? (cluster_bytes - offset_from_cluster) : total;

    if (stream_seek_ok(stream, starting_pos) < 0) return -EIO;

    int n = diskstreamer_read(stream, out, to_read);
    if (n < 0) return n;
    if (n != to_read) return -EIO;

    total -= to_read;
    if (total > 0)
    {
        return fat16_read_internal_from_stream(
            disk, stream, cluster,
            offset + to_read,
            total,
            (uint8_t*)out + to_read
        );
    }

    return 0;
}


static int fat16_read_internal(struct disk_driver *disk, int starting_cluster, int offset, int total, void *out)
{
    struct fat_private *fs_private = disk->fs_private;
    struct disk_stream *stream = fs_private->cluster_read_stream;
    return fat16_read_internal_from_stream(disk, stream, starting_cluster, offset, total, out);
}

void fat16_free_directory(struct fat_directory *directory)
{
    if (!directory)
    {
        return;
    }

    if (directory->item)
    {
        kfree(directory->item);
    }

    kfree(directory);
}

void fat16_fat_item_free(struct fat_item *item)
{
    if (item->type == FAT_ITEM_TYPE_DIRECTORY)
    {
        fat16_free_directory(item->directory);
    }
    else if (item->type == FAT_ITEM_TYPE_FILE)
    {
        kfree(item->item);
    }

    kfree(item);
}

int fat16_get_total_items_for_directory(struct disk_driver *disk, uint32_t directory_start_sector)
{
    struct fat_directory_item item;
    struct fat_directory_item empty_item;
    kmemset(&empty_item, 0, sizeof(empty_item));

    struct fat_private *fat_private = disk->fs_private;

    int res = 0;
    int i = 0;
    int directory_start_pos = directory_start_sector * disk->sector_size;
    struct disk_stream *stream = fat_private->directory_stream;
    if (diskstreamer_seek(stream, directory_start_pos) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    while (1)
    {
        int n = diskstreamer_read(stream, &item, sizeof(item));
        if (n < 0) { 
            res = -EIO;
            goto out;
        }
        if (n != sizeof(item)) { 
            res = -EIO; 
            goto out; 
        } // leitura curta
        

        if (item.filename[0] == 0x00)
        {
            // We are done
            break;
        }

        // Is the item unused
        if (item.filename[0] == 0xE5)
        {
            continue;
        }

        if ((item.attribute & 0x0F) == 0x0F) {
            continue;
        }

        i++;
    }

    res = i;

out:
    return res;
}


struct fat_directory *fat16_load_fat_directory(struct disk_driver *disk, struct fat_directory_item *item)
{
    if (!(item->attribute & FAT_FILE_SUBDIRECTORY))
        return 0;

    int cluster = (int)fat16_get_first_cluster(item);
    if (cluster < 2)
        return 0;

    // Usa o loader “bonito” (chain + compactação)
    return fat16_load_directory_from_cluster_chain(disk, cluster);
}



struct fat_item *fat16_new_fat_item_for_directory_item(struct disk_driver *disk, struct fat_directory_item *item)
{
    struct fat_item *f_item = kzalloc(sizeof(struct fat_item));
    if (!f_item) return 0;

    if (item->attribute & FAT_FILE_SUBDIRECTORY)
    {
        f_item->directory = fat16_load_fat_directory(disk, item);
        if (!f_item->directory) { kfree(f_item); return 0; }

        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
        return f_item;
    }

    f_item->type = FAT_ITEM_TYPE_FILE;
    f_item->item = fat16_clone_directory_item(item, sizeof(struct fat_directory_item));
    if (!f_item->item) { kfree(f_item); return 0; }

    return f_item;
}


struct fat_item *fat16_find_item_in_directory0(struct disk_driver *disk, struct fat_directory *directory, const char *name)
{
    struct fat_item *f_item = 0;
    char tmp_filename[PATH_MAX];
    for (int i = 0; i < directory->total; i++)
    {
        fat16_get_full_relative_filename(&directory->item[i], tmp_filename, sizeof(tmp_filename));
        if (kstrnicmp(tmp_filename, name, sizeof(tmp_filename)) == 0)
        {
            // Found it let's create a new fat_item
            f_item = fat16_new_fat_item_for_directory_item(disk, &directory->item[i]);
            break;
        }
    }

    return f_item;
}


struct fat_item *fat16_find_item_in_directory(struct disk_driver *disk,
                                              struct fat_directory *directory,
                                              const char *name)
{
    if (!disk || !directory || !name) return 0;
    if (directory->total <= 0 || !directory->item) return 0;

    char tmp_filename[PATH_MAX];

    for (int i = 0; i < directory->total; i++)
    {
        fat16_get_full_relative_filename(&directory->item[i], tmp_filename, sizeof(tmp_filename));
        if (kstrnicmp(tmp_filename, name, sizeof(tmp_filename)) == 0)
            return fat16_new_fat_item_for_directory_item(disk, &directory->item[i]);
    }

    return 0;
}



struct fat_item *fat16_get_directory_entry(struct disk_driver *disk, struct path_part *path)
{
    struct fat_private *fat_private = disk->fs_private;
    struct fat_item *current_item = 0;
    struct fat_item *root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);
    if (!root_item)
    {
        kprintf("\nroot_item nulo!");
        goto out;
    }

    struct path_part *next_part = path->next;
    current_item = root_item;
    while (next_part != 0)
    {
        if (current_item->type != FAT_ITEM_TYPE_DIRECTORY)
        {
            current_item = 0;
            break;
        }

        struct fat_item *tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
        fat16_fat_item_free(current_item);
        current_item = tmp_item;
        next_part = next_part->next;
    }
out:
    return current_item;
}


int fat16_get_root_directory(struct disk_driver *disk, struct fat_private *fat_private, struct fat_directory *directory)
{
    int res = 0;

    struct fat_header *h = &fat_private->header.primary_header;

    int root_dir_sector_pos = (h->fat_copies * h->sectors_per_fat) + h->reserved_sectors;
    int root_dir_entries    = h->root_dir_entries;
    int root_dir_bytes      = root_dir_entries * (int)sizeof(struct fat_directory_item);

    int total_sectors = root_dir_bytes / disk->sector_size;
    if (root_dir_bytes % disk->sector_size) total_sectors++;

    // buffer bruto com todas as entradas do root
    struct fat_directory_item* raw = kzalloc(root_dir_bytes);
    if (!raw) return -ENOMEM;

    struct disk_stream *stream = fat_private->directory_stream;
    if (!stream) { kfree(raw); return -EIO; }

    if (stream_seek_ok(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) < 0)
    {
        kfree(raw);
        return -EIO;
    }

    // lê tudo do root
    int n = diskstreamer_read(stream, raw, root_dir_bytes);
    if (n < 0 || n != root_dir_bytes)
    {
        kfree(raw);
        return -EIO;
    }

    // 1) conta válidas 8.3
    int count = 0;
    for (int i = 0; i < root_dir_entries; i++)
    {
        if (fat16_entry_is_end(&raw[i])) break;
        if (!fat16_entry_is_valid_83(&raw[i])) continue;
        count++;
    }

    // 2) aloca compactado (somente válidas)
    struct fat_directory_item* compact = 0;
    if (count > 0)
    {
        compact = kzalloc(count * sizeof(struct fat_directory_item));
        if (!compact)
        {
            kfree(raw);
            return -ENOMEM;
        }

        int out_i = 0;
        for (int i = 0; i < root_dir_entries; i++)
        {
            if (fat16_entry_is_end(&raw[i])) break;
            if (!fat16_entry_is_valid_83(&raw[i])) continue;

            kmemcpy(&compact[out_i++], &raw[i], sizeof(struct fat_directory_item));
            if (out_i == count) break;
        }
    }

    kfree(raw);

    directory->item = compact;      // pode ser NULL se count==0
    directory->total = count;
    directory->sector_pos = root_dir_sector_pos;
    directory->ending_sector_pos = root_dir_sector_pos + total_sectors;

    return res;
}

int fat16_resolve(struct disk_driver *disk)
{
    int res = 0;

    struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));
    if (!fat_private) return -ENOMEM;

    res = fat16_init_private(disk, fat_private);
    if (res < 0) { kfree(fat_private); return res; }

    disk->fs_private = fat_private;
    disk->filesystem = &fat16_fs;

    struct disk_stream *stream = diskstreamer_new(disk->id);
    if (!stream)
    {
        res = -ENOMEM;
        goto out;
    }

    int n = diskstreamer_read(stream, &fat_private->header, sizeof(fat_private->header));
    if (n < 0 || n != sizeof(fat_private->header))
    {
        res = -EIO;
        goto out;
    }

    if (fat_private->header.shared.extended_header.signature != 0x29)
    {
        res = -EFSNOTUS;
        goto out;
    }

    res = fat16_get_root_directory(disk, fat_private, &fat_private->root_directory);
    if (res != 0)
    {
        res = -EIO;
        goto out;
    }

out:
    if (stream) diskstreamer_close(stream);

    if (res < 0)
    {
        // TODO: fechar streams do private se você tiver API de close para diskstreamer
        kfree(fat_private);
        disk->fs_private = 0;
        disk->filesystem = 0;
    }

    return res;
}

static int fat16_read_entire_cluster(struct disk_driver* disk, int cluster, void* out, int cluster_bytes)
{
    struct fat_private* priv = disk->fs_private;
    struct disk_stream* s = priv->cluster_read_stream;
    if (!s) return -EIO;

    int sector = fat16_cluster_to_sector(priv, cluster);
    int abs = sector * disk->sector_size;

    if (stream_seek_ok(s, abs) < 0) return -EIO;

    int n = diskstreamer_read(s, out, cluster_bytes);
    if (n < 0) return n;
    if (n != cluster_bytes) return -EIO;

    return 0;
}

static struct fat_directory* fat16_load_directory_from_cluster_chain(struct disk_driver* disk, int start_cluster)
{
    struct fat_private* priv = disk->fs_private;
    int cluster_bytes = priv->header.primary_header.sectors_per_cluster * disk->sector_size;

    uint8_t* buf = kzalloc(cluster_bytes);
    if (!buf) return 0;

    // 1) contar válidas
    int total_valid = 0;
    int cur = start_cluster;
    int done = 0;

    while (!done)
    {
        if (fat16_read_entire_cluster(disk, cur, buf, cluster_bytes) < 0) { kfree(buf); return 0; }

        int entries = cluster_bytes / (int)sizeof(struct fat_directory_item);
        struct fat_directory_item* items = (struct fat_directory_item*)buf;

        for (int i = 0; i < entries; i++)
        {
            if (fat16_entry_is_end(&items[i])) { done = 1; break; }
            if (!fat16_entry_is_valid_83(&items[i])) continue;
            total_valid++;
        }

        if (done) break;

        int next = fat16_get_fat_entry(disk, cur);
        if (next < 0) { kfree(buf); return 0; }

        uint16_t v = (uint16_t)next;
        if (fat16_is_eoc(v)) break;
        if (fat16_is_bad(v) || fat16_is_reserved(v) || fat16_is_free(v)) { kfree(buf); return 0; }

        cur = next;
    }

    struct fat_directory* dir = kzalloc(sizeof(struct fat_directory));
    if (!dir) { kfree(buf); return 0; }

    dir->total = total_valid;
    dir->item = (total_valid > 0) ? kzalloc(total_valid * sizeof(struct fat_directory_item)) : 0;
    if (total_valid > 0 && !dir->item) { kfree(buf); kfree(dir); return 0; }

    // 2) preencher compactado
    cur = start_cluster;
    done = 0;
    int out_i = 0;

    while (!done)
    {
        if (fat16_read_entire_cluster(disk, cur, buf, cluster_bytes) < 0) { fat16_free_directory(dir); kfree(buf); return 0; }

        int entries = cluster_bytes / (int)sizeof(struct fat_directory_item);
        struct fat_directory_item* items = (struct fat_directory_item*)buf;

        for (int i = 0; i < entries; i++)
        {
            if (fat16_entry_is_end(&items[i])) { done = 1; break; }
            if (!fat16_entry_is_valid_83(&items[i])) continue;

            kmemcpy(&dir->item[out_i++], &items[i], sizeof(struct fat_directory_item));
            if (out_i == total_valid) { done = 1; break; }
        }

        if (done) break;

        int next = fat16_get_fat_entry(disk, cur);
        if (next < 0) { fat16_free_directory(dir); kfree(buf); return 0; }

        uint16_t v = (uint16_t)next;
        if (fat16_is_eoc(v)) break;
        if (fat16_is_bad(v) || fat16_is_reserved(v) || fat16_is_free(v)) { fat16_free_directory(dir); kfree(buf); return 0; }

        cur = next;
    }

    kfree(buf);
    return dir;
}



void *fat16_open(struct disk_driver *disk, struct path_part *path, FILE_MODE mode)
{
    struct fat_file_descriptor *descriptor = 0;
    int err_code = 0;
    if (mode != FILE_MODE_READ)
    {
        err_code = -ERDONLY;
        kprintf("\nerr_code=%d", err_code);
        goto err_out;
    }

    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!descriptor)
    {
        err_code = -ENOMEM;
        kprintf("\nerr_code=%d", err_code);
        goto err_out;
    }

    kprintf("\n[FAT16][open] path->part='%s'", path ? path->part : "(null)");
    //kprintf("\n[FAT16][open] path->next=%p", path ? path->next : 0);

    descriptor->item = fat16_get_directory_entry(disk, path);
    if (!descriptor->item)
    {
        err_code = -EIO;
        kprintf("\nfat16_open: err_code=%d", err_code);
        goto err_out;
    }

    descriptor->pos = 0;
    return descriptor;

err_out:
    if(descriptor)
        kfree(descriptor);

    return ERROR(err_code);
}

int fat16_read(struct disk_driver *disk, void *descriptor, uint32_t size, uint32_t nmemb, char *out_ptr)
{
    int res = 0;
    struct fat_file_descriptor *fat_desc = descriptor;
    struct fat_directory_item *item = fat_desc->item->item;
    int offset = fat_desc->pos;
    for (uint32_t i = 0; i < nmemb; i++)
    {
        res = fat16_read_internal(disk, fat16_get_first_cluster(item), offset, size, out_ptr);
        if (ISERR(res))
        {
            goto out;
        }

        out_ptr += size;
        offset += size;
    }
    fat_desc->pos = offset;
    res = nmemb;
out:
    return res;
}
