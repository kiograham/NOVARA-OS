#include <stdint.h>
#include "screen.h"
#include "panic.h"
#include "interrupts/idt.h"
#include "memory/multiboot2.h"
#include "memory/pmm.h"

#define MULTIBOOT2_BOOT_MAGIC 0x36D76289

static void print_decimal(uint64_t value)
{
    char buffer[20];
    int i = 0;

    if (value == 0)
    {
        screen_put_char('0');
        return;
    }

    while (value > 0)
    {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0)
    {
        screen_put_char(buffer[--i]);
    }
}

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

    screen_write("Welcome to NOVARA OS!\n");
    screen_write("Screen Manager initialized.\n");
    screen_write("Interrupt Descriptor Table initialized.\n");
    screen_write("Exception Handler initialized.\n");
    screen_write("Kernel Panic System initialized.\n");
    screen_write("Multiboot2 Boot Magic: VALID\n");
    screen_write("Multiboot2 Information: RECEIVED\n");

    multiboot2_print_memory_map(multiboot_info_address);

    screen_write("\n=== PHYSICAL MEMORY MANAGER ===\n");

    if (!pmm_init(multiboot_info_address))
    {
        kernel_panic("Physical Memory Manager Initialization Failed");
    }

    screen_write("Physical Memory Manager: INITIALIZED\n");

    uint64_t free_before = pmm_get_free_pages();

    screen_write("Free physical pages: ");
    print_decimal(free_before);
    screen_write("\n");

    screen_write("\n=== PMM ALLOCATION TEST ===\n");

    uint64_t allocated_page = pmm_alloc_page();

    if (allocated_page == 0)
        kernel_panic("PMM Allocation Failed");

    if (pmm_get_free_pages() != free_before - 1)
        kernel_panic("Incorrect Free Page Count After Allocation");

    screen_write("Page allocation: PASS\n");

    if (!pmm_free_page(allocated_page))
        kernel_panic("PMM Free Failed");

    if (pmm_get_free_pages() != free_before)
        kernel_panic("Incorrect Free Page Count After Free");

    screen_write("Page release: PASS\n");

    if (pmm_free_page(allocated_page))
        kernel_panic("Double Free Protection Failed");

    screen_write("Double free protection: PASS\n");

    if (pmm_free_page(0x100000ULL))
        kernel_panic("Kernel Memory Protection Failed");

    screen_write("Kernel memory protection: PASS\n");

    screen_write("All PMM tests passed.\n");
    screen_write("Kernel status: RUNNING");

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
