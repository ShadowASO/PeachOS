/*
 * Copyright (C) Daniel McCarthy <daniel@dragonzap.com>
 * This file is licensed under the GPLv2 license.
 * To learn how to build this get the kernel development course at https://dragonzap.com/course/developing-a-multithreaded-kernel-from-scratch
 */

#ifndef FAT16_H
#define FAT16_H

#include "../file.h"

#define FAT16_SIGNATURE 0x29
#define FAT16_FAT_ENTRY_SIZE 0x02
#define FAT16_BAD_SECTOR 0xFFF7
#define FAT16_UNUSED 0x00

typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1

// Fat directory entry attributes bitmask

// Atributos relevantes
enum {
    FAT_ATTR_READ_ONLY = 0x01,
    FAT_ATTR_HIDDEN    = 0x02,
    FAT_ATTR_SYSTEM    = 0x04,
    FAT_ATTR_VOLUME_ID = 0x08,
    FAT_ATTR_DIRECTORY = 0x10,
    FAT_ATTR_ARCHIVE   = 0x20,
    FAT_ATTR_LONG_NAME = 0x0F,
    FAT_ATTR_DEVICE    = 0x40,
    FAT_ATTR_RESERVED  = 0x80
};


// -----------------------------------------------------------------------------
// BPB/EBPB FAT16 (layout padrão do boot sector)
// -----------------------------------------------------------------------------
typedef struct bpb_header
{
    uint8_t  jmp[3];
    char     oem[8];

    uint16_t bytes_per_sector;      // BPB_BytsPerSec
    uint8_t  sectors_per_cluster;   // BPB_SecPerClus
    uint16_t reserved_sectors;      // BPB_RsvdSecCnt
    uint8_t  fat_count;             // BPB_NumFATs
    uint16_t root_entry_count;      // BPB_RootEntCnt
    uint16_t total_sectors_16;      // BPB_TotSec16
    uint8_t  media;                 // BPB_Media
    uint16_t sectors_per_fat_16;    // BPB_FATSz16
    uint16_t sectors_per_track;     // BPB_SecPerTrk
    uint16_t head_count;            // BPB_NumHeads
    uint32_t hidden_sectors;        // BPB_HiddSec
    uint32_t total_sectors_32;      // BPB_TotSec32

} __attribute__((packed)) bpb_header_t;

// EBPB FAT16
struct ebpb_ext
{
    uint8_t  drive_number;          // BS_DrvNum
    uint8_t  reserved1;             // BS_Reserved1
    uint8_t  boot_signature;        // BS_BootSig (0x29)
    uint32_t volume_id;             // BS_VolID
    char     volume_label[11];      // BS_VolLab
    char     fs_type[8];            // BS_FilSysType ("FAT16   ")

} __attribute__((packed));

// -----------------------------------------------------------------------------
// Info derivado do BPB (endereços LBA de FAT/ROOT/DATA)
// -----------------------------------------------------------------------------
typedef struct {
    //struct bpb_header bpb;

    uint32_t fat_start_lba;     // início da FAT1
    uint32_t root_start_lba;    // início do RootDir
    uint32_t data_start_lba;    // início da Data Area (cluster 2)
    uint32_t root_sectors_size;      // tamanho do root em setores
} fat16_ctx_t;
struct fat_h
{
    struct bpb_header bpb;
    union fat_h_e {
        struct ebpb_ext ext_header;
    } shared;
    
};



// -----------------------------------------------------------------------------
// Directory entry FAT (32 bytes)
// -----------------------------------------------------------------------------
struct fat_dir_entry
{
    char     name[8];     // 8.3 (padded with spaces)
    char ext[3];
    uint8_t  attr;
    uint8_t  nt_res;
    uint8_t  crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t fst_clus_hi;  // sempre 0 no FAT16
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus_lo;  // FirstCluster
    uint32_t file_size;

} __attribute__((packed));

struct fat_directory
{
    struct fat_dir_entry *item;
    int total;
    int sector_pos;
    int ending_sector_pos;
};

struct fat_item
{
    union {
        struct fat_dir_entry *item;
        struct fat_directory *directory;
    };

    FAT_ITEM_TYPE type;
};

// ------------------------------------------------------------
// 1) Atualize o descriptor (fat16.h)
// ------------------------------------------------------------
struct fat_file_descriptor
{
    struct fat_item *item;
    uint32_t pos;

    // cache para leitura sequencial
    uint16_t first_cluster;          // cluster inicial do arquivo (do dir entry)
    uint16_t current_cluster;        // cluster que contém 'pos' (ou onde estamos lendo)
    uint32_t current_cluster_index;  // 0 = first_cluster, 1 = próximo, etc.
};

//Layout
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

    fat16_ctx_t ctx;
};


struct filesystem* fat16_init();
int fat16_resolve(struct disk_driver * disk);
void * fat16_open(struct disk_driver * disk, struct path_part *path, FILE_MODE mode);
int fat16_read(struct disk_driver *disk, void *descriptor, uint32_t size, uint32_t nmemb, char *out_ptr);
int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode);
int fat16_stat(struct disk_driver* disk, void* private, struct file_stat* stat);
int fat16_close(void* private);


#endif