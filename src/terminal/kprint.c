#include "kprint.h"
#include "screen.h"
#include "../klib/string.h"
#include "../terminal/screen.h"



void kprint(const char* str)
{
    //char atrib=get_text_video_atrib(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    size_t len = kstrlen(str);    
    for (int i = 0; i < len; i++)
    {        
        kputchar(str[i]);
    }
}

static void kputhex_nibble(uint8_t nibble, char atrib)
{
    char c;
    if (nibble < 10)
        c = '0' + nibble;
    else
        c = 'A' + (nibble - 10);

    kputchar(c);
}

void kprint_hex(uint32_t value)
{
    char atrib = get_text_video_atrib(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    // prefixo padrão
    kputchar('0');
    kputchar('x');

    // imprime 8 nibbles (32 bits)
    for (int shift = 28; shift >= 0; shift -= 4) {
        uint8_t nibble = (value >> shift) & 0xF;
        kputhex_nibble(nibble, atrib);
    }
}