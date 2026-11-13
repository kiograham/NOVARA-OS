#include <stdint.h>
#include "screen.h"
#include "panic.h"
#include "interrupts/idt.h"
#include "memory/multiboot2.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"

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

    screen_write("\n=== VIRTUAL MEMORY MANAGER ===\n");

    if (!vmm_init())
        kernel_panic("Virtual Memory Manager Initialization Failed");

    screen_write("Virtual Memory Manager: INITIALIZED\n");

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

    screen_write("\n=== VMM TEST ===\n");

    uint64_t identity_address = 0x200000ULL;

    if (vmm_get_physical_address(identity_address) != identity_address)
        kernel_panic("VMM Identity Mapping Test Failed");

    screen_write("Identity mapping: PASS\n");

    uint64_t virtual_test_address = 0x40000000ULL;
    uint64_t physical_test_page = pmm_alloc_page();

    if (physical_test_page == 0)
        kernel_panic("VMM Test Physical Allocation Failed");

    if (!vmm_map_page(
            virtual_test_address,
            physical_test_page,
            VMM_PRESENT | VMM_WRITABLE))
        kernel_panic("VMM Page Mapping Failed");

    if (vmm_get_physical_address(virtual_test_address) != physical_test_page)
        kernel_panic("VMM Address Translation Test Failed");

    screen_write("Virtual page mapping: PASS\n");
    screen_write("Address translation: PASS\n");

    volatile uint64_t *test_memory =
        (volatile uint64_t *)(uintptr_t)virtual_test_address;

    *test_memory = 0x4E4F564152414F53ULL;

    if (*test_memory != 0x4E4F564152414F53ULL)
        kernel_panic("VMM Memory Access Test Failed");

    screen_write("Virtual memory read/write: PASS\n");

    if (!vmm_unmap_page(virtual_test_address))
        kernel_panic("VMM Page Unmapping Failed");

    if (vmm_get_physical_address(virtual_test_address) != 0)
        kernel_panic("VMM Unmapping Verification Failed");

    screen_write("Virtual page unmapping: PASS\n");

    if (!pmm_free_page(physical_test_page))
        kernel_panic("VMM Test Physical Page Release Failed");

    screen_write("VMM physical page release: PASS\n");

    screen_write("\n=== KERNEL HEAP ALLOCATOR ===\n");

    if (!heap_init())
        kernel_panic("Heap Initialization Failed");

    screen_write("Heap initialization: PASS\n");

    void *block_a = kmalloc(64);
    void *block_b = kmalloc(128);

    if (block_a == 0 || block_b == 0)
        kernel_panic("Heap Allocation Failed");

    if (block_a == block_b ||
        (uintptr_t)block_a % HEAP_ALIGNMENT != 0 ||
        (uintptr_t)block_b % HEAP_ALIGNMENT != 0)
        kernel_panic("Heap Alignment Test Failed");

    screen_write("Memory allocation and alignment: PASS\n");

    *(uint64_t *)block_a = 0x4E4F56415241ULL;
    *(uint64_t *)block_b = 0x484541504F53ULL;

    if (*(uint64_t *)block_a != 0x4E4F56415241ULL ||
        *(uint64_t *)block_b != 0x484541504F53ULL)
        kernel_panic("Heap Read Write Test Failed");

    screen_write("Heap read/write: PASS\n");

    if (!kfree(block_a) || !kfree(block_b))
        kernel_panic("Heap Free Test Failed");

    if (heap_get_used() != 0)
        kernel_panic("Heap Usage Tracking Failed");

    screen_write("Memory release and coalescing: PASS\n");

    if (kfree(block_b))
        kernel_panic("Heap Double Free Protection Failed");

    screen_write("Double free protection: PASS\n");

    void *large_block = kmalloc(5000);

    if (large_block == 0)
        kernel_panic("Heap Growth Allocation Failed");

    if (heap_get_capacity() < 2 * HEAP_PAGE_SIZE)
        kernel_panic("Heap Growth Test Failed");

    volatile uint64_t *large_memory =
        (volatile uint64_t *)large_block;

    large_memory[0] = 0x4E4F56415241ULL;
    large_memory[624] = 0x484541504F53ULL;

    if (large_memory[0] != 0x4E4F56415241ULL ||
        large_memory[624] != 0x484541504F53ULL)
        kernel_panic("Heap Growth Read Write Test Failed");

    screen_write("Dynamic heap growth: PASS\n");

    if (!kfree(large_block))
        kernel_panic("Large Heap Block Release Failed");

    if (heap_get_used() != 0)
        kernel_panic("Final Heap Usage Tracking Failed");

    screen_write("All heap tests passed.\n");


    screen_write("All PMM and VMM tests passed.\n");
    screen_write("Kernel status: RUNNING");

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
