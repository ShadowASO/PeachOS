// disk.h
#ifndef PATH_PARSER_H
#define PATH_PARSER_H

#include <stdint.h>

struct path_root
{
    int drive_nr;
    struct path_part * first;    
};

struct path_part
{
    const char * part;
    struct path_part * next;    
};

struct path_root * path_parser(const char * path, const char * current_dir_path);
void path_free(struct path_root * root);

#endif