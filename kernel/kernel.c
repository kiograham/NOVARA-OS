#include <stdint.h>
#include "screen.h"
#include "panic.h"
#include "interrupts/idt.h"
#include "memory/multiboot2.h"

#define MULTIBOOT2_BOOT_MAGIC 0x36D76289

void kernel_main(uint32_t boot_magic, uint64_t multiboot_info_address)
{
    screen_clear();

    idt_init();

    if (boot_magic != MULTIBOOT2_BOOT_MAGIC)
    {
        kernel_panic("Invalid Multiboot2 Boot Magic");
    }

    if (multiboot_info_address == 0)
    {
        kernel_panic("Multiboot2 Information Pointer is NULL");
    }

    screen_set_color(0x0F);

    screen_write("Welcome to NOVARA OS!");
    screen_write("\n");
    screen_write("Screen Manager initialized.");
    screen_write("\n");
    screen_write("Interrupt Descriptor Table initialized.");
    screen_write("\n");
    screen_write("Exception Handler initialized.");
    screen_write("\n");
    screen_write("Kernel Panic System initialized.");
    screen_write("\n");
    screen_write("Multiboot2 Boot Magic: VALID");
    screen_write("\n");
    screen_write("Multiboot2 Information: RECEIVED");
    screen_write("\n");

    multiboot2_print_memory_map(multiboot_info_address);

    screen_write("\nKernel status: RUNNING");

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
