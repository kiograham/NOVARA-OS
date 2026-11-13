#include "screen.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

static uint8_t current_color = 0x0F;
static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;

static uint16_t vga_entry(char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void screen_scroll(void)
{
    if (cursor_y < VGA_HEIGHT)
        return;

    for (uint16_t y = 1; y < VGA_HEIGHT; y++)
    {
        for (uint16_t x = 0; x < VGA_WIDTH; x++)
        {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] =
                VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }

    for (uint16_t x = 0; x < VGA_WIDTH; x++)
    {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            vga_entry(' ', current_color);
    }

    cursor_y = VGA_HEIGHT - 1;
}

void screen_set_color(uint8_t color)
{
    current_color = color;
}

void screen_clear(void)
{
    for (uint16_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (uint16_t x = 0; x < VGA_WIDTH; x++)
        {
            VGA_MEMORY[y * VGA_WIDTH + x] =
                vga_entry(' ', current_color);
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}

void screen_put_char(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
        screen_scroll();
        return;
    }

    VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] =
        vga_entry(c, current_color);

    cursor_x++;

    if (cursor_x >= VGA_WIDTH)
    {
        cursor_x = 0;
        cursor_y++;
        screen_scroll();
    }
}

void screen_write(const char *text)
{
    for (uint32_t i = 0; text[i] != '\0'; i++)
    {
        screen_put_char(text[i]);
    }
}
