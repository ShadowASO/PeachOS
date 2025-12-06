#include "screen.h"
#include <stddef.h>
#include <stdint.h>

uint16_t video_row = 0;
uint16_t video_col = 0;
volatile uint16_t *video_mem = (volatile uint16_t *) VGA_VIDEO_ADDRESS;

uint16_t get_text_video_elem(char c, char atrib)
{
    return (atrib << 8) | (uint8_t)c;
}

uint8_t get_text_video_atrib(char char_color, char background_color)
{
    return (background_color << 4) | (char_color & 0x0F);
}

void set_text_video_elem(int x, int y, char c, char atrib)
{
    video_mem[(y * VGA_WIDTH) + x] = get_text_video_elem(c, atrib);
}

/* === NOVO: rotina para rolar a tela uma linha para cima === */
static void scroll_screen(void)
{
    // Move as linhas 1..(VGA_HEIGHT-1) para 0..(VGA_HEIGHT-2)
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            video_mem[(y - 1) * VGA_WIDTH + x] =
                video_mem[y * VGA_WIDTH + x];
        }
    }

    // Limpa a última linha
    uint8_t atrib = get_text_video_atrib(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    for (int x = 0; x < VGA_WIDTH; x++) {
        video_mem[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            get_text_video_elem(' ', atrib);
    }

    // Cursor fica na última linha
    if (video_row > 0) {
        video_row = VGA_HEIGHT - 1;
    }
}

void video_init()
{
    video_row = 0;
    video_col = 0;
    uint8_t atrib = get_text_video_atrib(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            set_text_video_elem(x, y, ' ', atrib);
        }
    }
}

void clear_text_video()
{
    video_row = 0;
    video_col = 0;
    uint8_t atrib = get_text_video_atrib(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            set_text_video_elem(x, y, ' ', atrib);
        }
    }
}

void kputchar_attrib(char c, char atrib)
{
    if (c == '\n') {
        video_row += 1;
        video_col = 0;

        // Se passou da última linha, faz scroll
        if (video_row >= VGA_HEIGHT) {
            scroll_screen();
        }
        return;
    }

    set_text_video_elem(video_col, video_row, c, atrib);
    video_col += 1;

    // Corrigido: >= em vez de >
    if (video_col >= VGA_WIDTH) {
        video_col = 0;
        video_row += 1;
    }

    // Se passou da última linha, faz scroll
    if (video_row >= VGA_HEIGHT) {
        scroll_screen();
    }
}

void kputchar(char c)
{
    char atrib = get_text_video_atrib(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kputchar_attrib(c, atrib);
}
