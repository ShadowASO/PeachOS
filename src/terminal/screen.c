#include "screen.h"
#include <stddef.h>
#include <stdint.h>

uint16_t video_row=0;
uint16_t video_col=0;
volatile uint16_t * video_mem=(volatile uint16_t *) VGA_VIDEO_ADDRESS;

uint16_t get_text_video_elem(char c, char atrib)
{
    return (atrib << 8) | c;
}

uint8_t get_text_video_atrib(char char_color, char background_color)
{
    return (background_color << 4 | char_color);
}

void set_text_video_elem(int x, int y, char c, char atrib)
{
    video_mem[(y * VGA_WIDTH)+x]=get_text_video_elem(c, atrib);
   
}

void video_init()
{    
    video_row = 0;
    video_col = 0;
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            set_text_video_elem(x, y, ' ', 0x0E);
        }
    }   
}

void clear_text_video()
{    
    video_row = 0;
    video_col = 0;
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            set_text_video_elem(x, y, ' ', 0x0E);
        }
    }   
}

void kputchar_attrib(char c, char atrib) {
    if(c=='\n') {
        video_row +=1;
        video_col =0;
        return;
    }

    set_text_video_elem(video_col, video_row, c, atrib);
    video_col +=1;
    if (video_col > VGA_WIDTH) {
        video_col=0;
        video_row+=1;
    }
}

void kputchar(char c) {
    char atrib=get_text_video_atrib(VGA_COLOR_WHITE, VGA_COLOR_BLACK);      
        
        kputchar_attrib(c, atrib);
   
}

