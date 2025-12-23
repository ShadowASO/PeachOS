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
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek,
    .stat = fat16_stat,
    .close = fat16_close
};

struct filesystem * fat16_init() {
    kstrcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static uint32_t fat16_total_sectors(const bpb_header_t* bpb) {
    return (bpb->total_sectors_16 != 0) ? (uint32_t)bpb->total_sectors_16 : bpb->total_sectors_32;
}

static uint32_t fat16_root_dir_sectors(const bpb_header_t* bpb) {
   
    uint32_t bytes = (uint32_t)bpb->root_entry_count * 32u;
    uint32_t bps   = (uint32_t)bpb->bytes_per_sector;
    return (bytes + (bps - 1)) / bps;
}

static uint32_t fat16_first_data_sector(const bpb_header_t* bpb) {
   
    return (uint32_t)bpb->hidden_sectors
         + (uint32_t)bpb->reserved_sectors
         + (uint32_t)bpb->fat_count * (uint32_t)bpb->sectors_per_fat_16
         + fat16_root_dir_sectors(bpb);
}

static uint32_t fat16_first_root_sector(const bpb_header_t* bpb) {
    return (uint32_t)bpb->hidden_sectors
         + (uint32_t)bpb->reserved_sectors
         + (uint32_t)bpb->fat_count * (uint32_t)bpb->sectors_per_fat_16;
}

static uint32_t fat16_cluster_to_lba(const struct fat_private *fat_priv, uint16_t cluster) {
    // Cluster 2 é o primeiro cluster de dados
    // FirstSectorOfCluster = DataStart + (cluster - 2) * SecPerClus
    return fat_priv->ctx.data_start_lba + (uint32_t)(cluster - 2u) * (uint32_t)fat_priv->header.bpb.sectors_per_cluster;
}

static int fat16_is_eoc(uint16_t v) {    
    return (v >= 0xFFF8u);
}

static inline int fat16_is_bad(uint16_t v) { return v == 0xFFF7; }
static inline int fat16_is_reserved(uint16_t v) { return (v >= 0xFFF0 && v <= 0xFFF6); }
static inline int fat16_is_free(uint16_t v) { return v == 0x0000; }

static inline int fat16_cluster_is_invalid(uint16_t v)
{
    return (v < 2) || fat16_is_bad(v) || fat16_is_reserved(v) || fat16_is_free(v);
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

// Comparação 8.3 em formato "NAMEEXT" (11 bytes) já preenchido com espaços.
static int fat16_name8_equals(const char a[8], const char b[11]) {
    for (int i = 0; i < 8; i++) {
        if ((uint8_t)a[i] != (uint8_t)b[i]) return 0;
    }
    return 1;
}


// ---------------------------
// Helpers de diretório (entry)
// ---------------------------
static inline int fat16_entry_is_end(const struct fat_dir_entry* it)
{
    return it->name[0] == 0x00;
}

static inline int fat16_entry_is_deleted(const struct fat_dir_entry* it)
{
    return it->name[0] == 0xE5;
}

static inline int fat16_entry_is_lfn(const struct fat_dir_entry* it)
{
    return (it->attr & 0x0F) == 0x0F;
}

static inline int fat16_entry_is_valid_83(const struct fat_dir_entry* it)
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

static int fat16_init_private_ctx(struct fat_private *fs_private)
{
    kmemset(&fs_private->ctx, 0, sizeof(fat16_ctx_t));

    // Copia BPB do setor 0
    const bpb_header_t* bpb = &fs_private->header.bpb;
    fat16_ctx_t *ctx =&fs_private->ctx;

    // Sanity checks mínimos
    if (bpb->bytes_per_sector == 0 || bpb->sectors_per_cluster == 0) return -2;
    if (bpb->sectors_per_fat_16 == 0) return -3;
    if (bpb->root_entry_count == 0) return -4;

    ctx->root_sectors_size   = fat16_root_dir_sectors(bpb);
    ctx->fat_start_lba  = (uint32_t)bpb->hidden_sectors + (uint32_t)bpb->reserved_sectors;
    ctx->root_start_lba = fat16_first_root_sector(bpb);
    ctx->data_start_lba = fat16_first_data_sector(bpb);

    
    return 0;
}


static size_t fat16_copy_trim_space(char* out, size_t out_cap, const char* in, size_t in_len)
{
    size_t w = 0;
    if (out_cap == 0) return 0;

    for (size_t i = 0; i < in_len; i++)
    {
        char c = in[i];
        if (c == '\0' || c == ' ') break;
        if (w + 1 >= out_cap) break;  // garante NUL
        out[w++] = c;
    }
    out[w] = '\0';
    return w;
}

void fat16_get_full_relative_filename(struct fat_dir_entry *item, char *out, int max_len)
{
    if (!out || max_len <= 0) return;

    out[0] = '\0';
    size_t cap = (size_t)max_len;

    size_t w = fat16_copy_trim_space(out, cap, (const char*)item->name, sizeof(item->name));

    // ext?
    if (item->ext[0] != '\0' && item->ext[0] != ' ')
    {
        if (w + 1 < cap) out[w++] = '.';
        if (w < cap) out[w] = '\0';

        fat16_copy_trim_space(out + w, cap - w, (const char*)item->ext, sizeof(item->ext));
    }
}

struct fat_dir_entry *fat16_clone_directory_item(struct fat_dir_entry *item, int size)
{
    struct fat_dir_entry *item_copy = 0;
    if (size < sizeof(struct fat_dir_entry))
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

static uint32_t fat16_get_first_cluster(struct fat_dir_entry *item)
{
    return (item->fst_clus_hi << 16) | item->fst_clus_lo;
};

int fat16_sector_to_absolute(struct disk_driver *disk, int sector)
{
    return sector * disk->sector_size;
}



static uint32_t fat16_cluster_to_lba_ctx(const struct fat_private* priv, uint16_t cluster)
{
    if (cluster < 2) return 0; // inválido
    return priv->ctx.data_start_lba
         + (uint32_t)(cluster - 2u) * (uint32_t)priv->header.bpb.sectors_per_cluster;
}

// Se quiser manter a assinatura antiga:
static int fat16_cluster_to_sector(struct fat_private* priv, int cluster)
{
    uint32_t lba = fat16_cluster_to_lba_ctx(priv, (uint16_t)cluster);
    return (int)lba;
}


static uint32_t fat16_get_first_fat_lba(struct fat_private *private)
{
    //return private->header.bpb.reserved_sectors;
    return private->ctx.fat_start_lba;
}

static int fat16_get_fat_entry(struct disk_driver *disk, uint16_t cluster)
{
    if (!disk || !disk->fs_private) return -EIO;

    struct fat_private *priv = disk->fs_private;
    struct disk_stream *stream = priv->fat_read_stream;
    if (!stream) return -EIO;

    // clusters válidos na cadeia: 0x0002..0xFFF6
    // (0xFFF7 bad, 0xFFF8..0xFFFF EOC/reservados)
    if (cluster < 2) return -EIO;

    uint32_t fat_abs = fat16_get_first_fat_lba(priv) * (uint32_t)disk->sector_size;
    uint32_t off     = (uint32_t)cluster * (uint32_t)FAT16_FAT_ENTRY_SIZE;

    uint32_t abs = fat_abs + off;

    // Protege cast para int (se sua API só aceita int)
    if (abs > 0x7FFFFFFFu) return -EIO;

    if (stream_seek_ok(stream, (int)abs) < 0)
        return -EIO;

    uint16_t result = 0;
    int r = stream_read_exact(stream, &result, sizeof(result));
    if (r < 0) return r;

    return (int)result;
}


static int fat16_next_cluster(struct disk_driver* disk, uint16_t cluster, uint16_t* out_next)
{
    int entry = fat16_get_fat_entry(disk, (int)cluster);
    if (entry < 0) return entry;

    uint16_t v = (uint16_t)entry;
    if (fat16_is_eoc(v)) return -EIO;
    if (fat16_cluster_is_invalid(v)) return -EIO;

    *out_next = v;
    return 0;
}

// Avança "steps" clusters a partir de start. steps=0 => retorna start.
static int fat16_advance_clusters(struct disk_driver* disk, uint16_t start, uint32_t steps, uint16_t* out_cluster)
{
    if (fat16_cluster_is_invalid(start)) return -EIO;

    uint16_t cur = start;
    for (uint32_t i = 0; i < steps; i++)
    {
        uint16_t next = 0;
        int r = fat16_next_cluster(disk, cur, &next);
        if (r < 0) return r;
        cur = next;
    }
    *out_cluster = cur;
    return 0;
}
/**
 * Gets the correct cluster to use based on the starting cluster and the offset
 */
static int fat16_get_cluster_for_offset(struct disk_driver *disk, int starting_cluster, uint32_t offset)
{
    struct fat_private *priv = disk->fs_private;
    if (!priv) return -EIO;

    uint32_t cluster_bytes =
        (uint32_t)priv->header.bpb.sectors_per_cluster * (uint32_t)disk->sector_size;

    if (cluster_bytes == 0) return -EIO;
    if (starting_cluster < 2) return -EIO;

    uint32_t wanted_index = offset / cluster_bytes;

    uint16_t out = 0;
    int r = fat16_advance_clusters(disk, (uint16_t)starting_cluster, wanted_index, &out);
    if (r < 0) return r;

    return (int)out;
}




static int fat16_seek_cache_to_offset(struct disk_driver* disk,
                                      struct fat_file_descriptor* d,
                                      uint32_t offset_bytes)
{
    struct fat_private* priv = disk->fs_private;
    if (!priv) return -EIO;

    uint32_t cluster_bytes = (uint32_t)priv->header.bpb.sectors_per_cluster * (uint32_t)disk->sector_size;
    if (cluster_bytes == 0) return -EIO;

    if (d->first_cluster < 2) return -EIO; // sem isso, não há de onde recalcular

    uint32_t wanted_index = offset_bytes / cluster_bytes;

    // Cache válido e avanço para frente: anda só o necessário
    if (d->current_cluster >= 2 && wanted_index >= d->current_cluster_index)
    {
        uint32_t steps = wanted_index - d->current_cluster_index;
        if (steps == 0) return 0;

        uint16_t new_cluster = 0;
        int r = fat16_advance_clusters(disk, d->current_cluster, steps, &new_cluster);
        if (r < 0) return r;

        d->current_cluster = new_cluster;
        d->current_cluster_index = wanted_index;
        return 0;
    }

    // Cache vazio ou seek para trás: recalcula do início do arquivo
    uint16_t new_cluster = 0;
    int r = fat16_advance_clusters(disk, d->first_cluster, wanted_index, &new_cluster);
    if (r < 0) return r;

    d->current_cluster = new_cluster;
    d->current_cluster_index = wanted_index;
    return 0;
}

static int fat16_read_cached(struct disk_driver* disk,
                             struct fat_file_descriptor* d,
                             uint32_t offset, uint32_t total,
                             void* out)
{
    struct fat_private* priv = disk->fs_private;
    if (!priv) return -EIO;

    struct disk_stream* stream = priv->cluster_read_stream;
    if (!stream) return -EIO;

    uint32_t cluster_bytes = (uint32_t)priv->header.bpb.sectors_per_cluster * (uint32_t)disk->sector_size;
    if (cluster_bytes == 0) return -EIO;

    if (d->first_cluster < 2) return -EIO;

    uint8_t* outp = (uint8_t*)out;
    uint32_t remaining = total;
    uint32_t cur_off = offset;

    while (remaining > 0)
    {
        int r = fat16_seek_cache_to_offset(disk, d, cur_off);
        if (r < 0) return r;

        uint32_t off_in_cluster = cur_off % cluster_bytes;
        uint32_t take = cluster_bytes - off_in_cluster;
        if (take > remaining) take = remaining;

        if (d->current_cluster < 2) return -EIO;

        uint32_t lba = fat16_cluster_to_lba(priv, d->current_cluster);
        uint32_t abs = lba * (uint32_t)disk->sector_size + off_in_cluster;

        // ideal: stream_seek_ok aceitar uint32_t; se não aceitar, cheque overflow antes do cast
        if (abs > 0x7FFFFFFFu) return -EIO; // protege cast para int
        if (stream_seek_ok(stream, (int)abs) < 0) return -EIO;

        int n = diskstreamer_read(stream, outp, (int)take);
        if (n < 0) return n;
        if ((uint32_t)n != take) return -EIO;

        outp += take;
        cur_off += take;
        remaining -= take;
    }

    return 0;
}


static int fat16_read_internal_from_stream(struct disk_driver *disk, struct disk_stream *stream,
                                          int start_cluster, int offset, int total, void *out)
{
    struct fat_private *priv = disk->fs_private;
    const int cluster_bytes = priv->header.bpb.sectors_per_cluster * disk->sector_size;

    uint8_t* outp = (uint8_t*)out;
    int remaining = total;
    int cur_off = offset;

    while (remaining > 0)
    {
        int cluster_to_use = fat16_get_cluster_for_offset(disk, start_cluster, cur_off);
        if (cluster_to_use < 0) return cluster_to_use;

        int off_in_cluster = cur_off % cluster_bytes;
        int take = cluster_bytes - off_in_cluster;
        if (take > remaining) take = remaining;

        uint32_t lba = fat16_cluster_to_lba_ctx(priv, (uint16_t)cluster_to_use);
        if (lba == 0) return -EIO;

        uint32_t abs = lba * (uint32_t)disk->sector_size + (uint32_t)off_in_cluster;
        if (stream_seek_ok(stream, (int)abs) < 0) return -EIO;

        int n = diskstreamer_read(stream, outp, take);
        if (n < 0) return n;
        if (n != take) return -EIO;

        outp += take;
        cur_off += take;
        remaining -= take;
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
    struct fat_dir_entry item;
    struct fat_dir_entry empty_item;
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
        

        if (item.name[0] == 0x00)
        {
            // We are done
            break;
        }

        // Is the item unused
        if (item.name[0] == 0xE5)
        {
            continue;
        }

        if ((item.attr & 0x0F) == 0x0F) {
            continue;
        }

        i++;
    }

    res = i;

out:
    return res;
}


struct fat_directory *fat16_load_fat_directory(struct disk_driver *disk, struct fat_dir_entry *item)
{
    if (!(item->attr & FAT_ATTR_DIRECTORY ))
        return 0;

    int cluster = (int)fat16_get_first_cluster(item);
    if (cluster < 2)
        return 0;

    // Usa o loader “bonito” (chain + compactação)
    return fat16_load_directory_from_cluster_chain(disk, cluster);
}



struct fat_item *fat16_new_fat_item_for_directory_item(struct disk_driver *disk, struct fat_dir_entry *item)
{
    struct fat_item *f_item = kzalloc(sizeof(struct fat_item));
    if (!f_item) return 0;

    if (item->attr & FAT_ATTR_DIRECTORY )
    {
        f_item->directory = fat16_load_fat_directory(disk, item);
        if (!f_item->directory) { kfree(f_item); return 0; }

        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
        return f_item;
    }

    f_item->type = FAT_ITEM_TYPE_FILE;
    f_item->item = fat16_clone_directory_item(item, sizeof(struct fat_dir_entry));
    if (!f_item->item) { kfree(f_item); return 0; }

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

    struct bpb_header *h = &fat_private->header.bpb;

    //int root_dir_sector_pos = (h->fat_count * h->sectors_per_fat_16) + h->reserved_sectors;
    int root_dir_sector_pos = fat_private->ctx.root_start_lba;
    int root_dir_entries    = h->root_entry_count;
    int root_dir_bytes      = root_dir_entries * (int)sizeof(struct fat_dir_entry);

    //int total_sectors = root_dir_bytes / disk->sector_size;
    int total_sectors = fat_private->ctx.root_sectors_size;
    if (root_dir_bytes % disk->sector_size) total_sectors++;

    // buffer bruto com todas as entradas do root
    struct fat_dir_entry* raw = kzalloc(root_dir_bytes);
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
    struct fat_dir_entry* compact = 0;
    if (count > 0)
    {
        compact = kzalloc(count * sizeof(struct fat_dir_entry));
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

            kmemcpy(&compact[out_i++], &raw[i], sizeof(struct fat_dir_entry));
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

static void fat16_destroy_private(struct fat_private* p)
{
    if (!p) return;

    // se tiver API:
    if (p->cluster_read_stream) diskstreamer_close(p->cluster_read_stream);
    if (p->fat_read_stream)     diskstreamer_close(p->fat_read_stream);
    if (p->directory_stream)    diskstreamer_close(p->directory_stream);

    if (p->root_directory.item) kfree(p->root_directory.item);

    kfree(p);
}


int fat16_resolve(struct disk_driver *disk)
{
    int res = 0;

    struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));
    if (!fat_private) 
        return -ENOMEM;

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
    //Faz a leitura do BPB + EBPB
    int n = diskstreamer_read(stream, &fat_private->header, sizeof(fat_private->header));
    if (n < 0 || n != sizeof(fat_private->header))
    {
        res = -EIO;
        goto out;
    }

    if (fat_private->header.shared.ext_header.boot_signature != 0x29)
    {
        res = -EFSNOTUS;
        goto out;
    }

    //Carregar as variáveis de contexto
    int err=fat16_init_private_ctx(fat_private);
    if(err < 0) {
        return err;
    }

    //Leitura do root directory
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
        fat16_destroy_private(fat_private);
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
    int cluster_bytes = priv->header.bpb.sectors_per_cluster * disk->sector_size;

    uint8_t* buf = kzalloc(cluster_bytes);
    if (!buf) return 0;

    // 1) contar válidas
    int total_valid = 0;
    int cur = start_cluster;
    int done = 0;

    while (!done)
    {
        if (fat16_read_entire_cluster(disk, cur, buf, cluster_bytes) < 0) { kfree(buf); return 0; }

        int entries = cluster_bytes / (int)sizeof(struct fat_dir_entry);
        struct fat_dir_entry* items = (struct fat_dir_entry*)buf;

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
    dir->item = (total_valid > 0) ? kzalloc(total_valid * sizeof(struct fat_dir_entry)) : 0;
    if (total_valid > 0 && !dir->item) { kfree(buf); kfree(dir); return 0; }

    // 2) preencher compactado
    cur = start_cluster;
    done = 0;
    int out_i = 0;

    while (!done)
    {
        if (fat16_read_entire_cluster(disk, cur, buf, cluster_bytes) < 0) { fat16_free_directory(dir); kfree(buf); return 0; }

        int entries = cluster_bytes / (int)sizeof(struct fat_dir_entry);
        struct fat_dir_entry* items = (struct fat_dir_entry*)buf;

        for (int i = 0; i < entries; i++)
        {
            if (fat16_entry_is_end(&items[i])) { done = 1; break; }
            if (!fat16_entry_is_valid_83(&items[i])) continue;

            kmemcpy(&dir->item[out_i++], &items[i], sizeof(struct fat_dir_entry));
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
        goto err_out;
    }

    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!descriptor)
    {
        err_code = -ENOMEM;
        goto err_out;
    }

    descriptor->item = fat16_get_directory_entry(disk, path);
    if (!descriptor->item || descriptor->item->type != FAT_ITEM_TYPE_FILE)
    {
        err_code = -EIO;
        goto err_out;
    }

    struct fat_dir_entry* item = descriptor->item->item;
    uint16_t first = (uint16_t)fat16_get_first_cluster(item);
    if (first < 2)
    {
        err_code = -EIO;
        goto err_out;
    }

    descriptor->pos = 0;
    descriptor->first_cluster = first;
    descriptor->current_cluster = first;
    descriptor->current_cluster_index = 0;

    return descriptor;

err_out:
    if (descriptor)
    {
        if (descriptor->item) fat16_fat_item_free(descriptor->item);
        kfree(descriptor);
    }
    return ERROR(err_code);
}


int fat16_read(struct disk_driver *disk,
               void *descriptor,
               uint32_t size,
               uint32_t nmemb,
               char *out_ptr)
{
    struct fat_file_descriptor *d = (struct fat_file_descriptor*)descriptor;
    if (!d || !d->item || d->item->type != FAT_ITEM_TYPE_FILE) return -EINVARG;
    if (!out_ptr) return -EINVARG;

    struct fat_dir_entry *item = d->item->item;
    if (!item) return -EIO;

    // total bytes solicitados
    uint64_t req64 = (uint64_t)size * (uint64_t)nmemb;
    if (req64 == 0) return 0;

    uint32_t file_size = item->file_size;

    // já está no EOF
    if (d->pos >= file_size) return 0;

    // limita pedido ao máximo representável em uint32_t (pois can_read é u32)
    uint32_t req = (req64 > 0xFFFFFFFFu) ? 0xFFFFFFFFu : (uint32_t)req64;

    // clamp no EOF com proteção de overflow em d->pos + req
    uint32_t remaining_in_file = file_size - d->pos;
    uint32_t can_read = (req > remaining_in_file) ? remaining_in_file : req;

    int r = fat16_read_cached(disk, d, d->pos, can_read, out_ptr);
    if (r < 0) return r;

    d->pos += can_read;

    // retorna bytes lidos
    // (se sua API usa int, proteja para não virar negativo por overflow)
    if (can_read > 0x7FFFFFFFu) return 0x7FFFFFFF;
    return (int)can_read;
}


int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode)
{
    struct fat_file_descriptor *desc = (struct fat_file_descriptor*) private;
    if (!desc || !desc->item) return -EINVARG;

    if (desc->item->type != FAT_ITEM_TYPE_FILE)
        return -EINVARG;

    struct fat_dir_entry *ritem = desc->item->item;
    if (!ritem) return -EIO;

    uint32_t file_size = ritem->file_size;
    uint32_t newpos = 0;

    switch (seek_mode)
    {
        case SEEK_SET:
            newpos = offset;
            break;

        case SEEK_CUR:
            // overflow check
            if (offset > (0xFFFFFFFFu - desc->pos))
                return -EIO;
            newpos = desc->pos + offset;
            break;

        case SEEK_END:
            return -EUNIMP;

        default:
            return -EINVARG;
    }

    // permitir seek exatamente no EOF; proibir passar do EOF
    if (newpos > file_size)
        return -EIO;

    desc->pos = newpos;

    // Invalida cache: próxima read recalcula a cadeia FAT desde first_cluster.
    // Se você ainda não tem first_cluster no descriptor, deixe como está (0) e
    // a read deve recalcular do início de todo jeito.
    desc->current_cluster = desc->first_cluster; // ou 0 se preferir "cache vazio"
    desc->current_cluster_index = 0;

    return 0;
}


int fat16_stat(struct disk_driver* disk, void* private, struct file_stat* stat)
{
    int res = 0;
    struct fat_file_descriptor* descriptor = (struct fat_file_descriptor*) private;
    struct fat_item* desc_item = descriptor->item;
    if (desc_item->type != FAT_ITEM_TYPE_FILE)
    {
        res = -EINVARG;
        goto out;
    }

    struct fat_dir_entry* ritem = desc_item->item;
    stat->filesize = ritem->file_size;
    stat->flags = 0x00;

    if (ritem->attr & FAT_ATTR_READ_ONLY)
    {
        stat->flags |= FILE_STAT_READ_ONLY;
    }
out:
    return res;
}
static void fat16_free_file_descriptor(struct fat_file_descriptor* desc)
{
    fat16_fat_item_free(desc->item);
    kfree(desc);
}

int fat16_close(void* private_data)
{
    fat16_free_file_descriptor((struct fat_file_descriptor*) private_data);
    return 0;
}