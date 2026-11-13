#include "panic.h"
#include "screen.h"

void kernel_panic_begin(const char *reason)
{
    __asm__ volatile ("cli");

    screen_set_color(0x4F);
    screen_clear();

    screen_write("================================");
    screen_write("\n");
    screen_write("       NOVARA OS");
    screen_write("\n");
    screen_write("        KERNEL PANIC");
    screen_write("\n");
    screen_write("================================");

    screen_write("\n\nSystem has encountered a fatal error.\n\n");
    screen_write("Reason: ");

    if (reason != 0)
    {
        screen_write(reason);
    }
    else
    {
        screen_write("Unknown error");
    }

    screen_write("\n\n");
}

__attribute__((noreturn))
void kernel_panic_halt(void)
{
    screen_write("System halted.");

    __asm__ volatile ("cli");

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}

__attribute__((noreturn))
void kernel_panic(const char *reason)
{
    kernel_panic_begin(reason);
    kernel_panic_halt();
}
