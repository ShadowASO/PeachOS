#ifndef KPRINT_H
#define KPRINT_H

#include <stddef.h>
#include <stdint.h>

void kprint(const char* str);
void kprint_hex(uint32_t value);
void kprint_hex_dump_lines(const void* data, size_t size, size_t bytes_per_line);


#endif