#include "fat16.h"
#include "../../klib/string.h"
#include "../../status.h"
#include "../file.h"
#include "../path_parser.h"
#include "../../drivers/disk/disk.h"
#include "../../drivers/disk/streamer.h"
#include "../../klib/memory.h"



struct fat_header_extended
{
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
} __attribute__((packed));

struct fat_header
{
    uint8_t short_jmp_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t number_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_setors;
    uint32_t sectors_big;
} __attribute__((packed));

struct fat_h
{
    struct fat_header primary_header;
    union fat_h_e {
        struct fat_header_extended extended_header;
    } shared;
};

struct fat_directory_item
{
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t creation_time_tenths_of_a_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t filesize;
} __attribute__((packed));

struct fat_directory
{
    struct fat_directory_item *item;
    int total;
    int sector_pos;
    int ending_sector_pos;
};

struct fat_item
{
    union {
        struct fat_directory_item *item;
        struct fat_directory *directory;
    };

    FAT_ITEM_TYPE type;
};

struct fat_file_descriptor
{
    struct fat_item *item;
    uint32_t pos;
};

struct fat_private
{
    struct fat_h header;
    struct fat_directory root_directory;

    // Used to stream data clusters
    struct disk_stream *cluster_read_stream;
    // Used to stream the file allocation table
    struct disk_stream *fat_read_stream;

    // Used in situations where we stream the directory
    struct disk_stream *directory_stream;
};

struct filesystem fat16_fs =
{
    .resolve = fat16_resolve,
    .open = (FS_OPEN_FUNCTION)fat16_open
};

struct filesystem * fat16_init() {
    kstrcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

static void fat16_init_private(struct disk_driver *disk, struct fat_private *fs_private)
{
    kmemset(fs_private, 0, sizeof(struct fat_private));
    fs_private->cluster_read_stream = diskstreamer_new(disk->id);
    fs_private->fat_read_stream = diskstreamer_new(disk->id);
    fs_private->directory_stream = diskstreamer_new(disk->id);
}

int fat16_sector_to_absolute(struct disk_driver *disk, int sector)
{
    return sector * disk->sector_size;
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
        if (diskstreamer_read(stream, &item, sizeof(item)) != PEACHOS_ALL_OK)
        {
            res = -EIO;
            goto out;
        }

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

        i++;
    }

    res = i;

out:
    return res;
}


int fat16_resolve(struct disk_driver *disk) {
    return 0;
}

void * fat16_open(struct disk_driver * disk, struct path_part * path, FILE_MODE mode) {
    return NULL;
}