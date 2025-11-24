

#ifndef SCREEN_H
#define SCREEN_H

#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 20
#define VGA_VIDEO_ADDRESS 0XB8000

//Cores 
#define VGA_COLOR_BLACK 0x0
#define VGA_COLOR_BLUE 0x1
#define VGA_COLOR_GREEN 0x2
#define VGA_COLOR_CYAN 0x3
#define VGA_COLOR_RED 0x4
#define VGA_COLOR_MAGENTA 0x5
#define VGA_COLOR_BROWN 0x6
#define VGA_COLOR_LIGHTGREY 0x7
#define VGA_COLOR_DARKGREY 0x8
#define VGA_COLOR_LIGHTBLUE 0x9
#define VGA_COLOR_LIGHTGREEN 0xA
#define VGA_COLOR_LIGHTCYAN 0xB
#define VGA_COLOR_LIGHTRED 0xC
#define VGA_COLOR_PINK 0xD
#define VGA_COLOR_YELLOW 0xE
#define VGA_COLOR_WHITE 0xF


uint16_t get_text_video_elem(char c, char atrib);
uint8_t get_text_video_atrib(char char_color, char background_color);
void set_text_video_elem(int x, int y, char c, char atrib);
void kputchar_attrib(char c, char atrib);
void kputchar(char c);
void kclear_text_video();
void video_init();


#endif