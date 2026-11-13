#include "screen.h"

void kernel_main(void)
{
    screen_clear();

    screen_set_color(0x0F);

    screen_write("Welcome to NOVARA OS!");
    screen_write("\n");
    screen_write("Screen Manager initialized.");
    screen_write("\n");
    screen_write("Kernel status: RUNNING");

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
