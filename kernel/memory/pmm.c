#include <stdint.h>
#include "pmm.h"
#include "multiboot2.h"

#define PAGE_SIZE       4096ULL
#define MEMORY_LIMIT    0x40000000ULL
#define TOTAL_PAGES     (MEMORY_LIMIT / PAGE_SIZE)
#define BITMAP_SIZE     (TOTAL_PAGES / 8)

static uint8_t used_bitmap[BITMAP_SIZE];
static uint8_t usable_bitmap[BITMAP_SIZE];

static uint64_t free_pages = 0;

static uint64_t boot_info_start = 0;
static uint64_t boot_info_end = 0;

extern uint8_t __kernel_start;
extern uint8_t __kernel_end;

static int bitmap_get(uint8_t *bitmap, uint64_t page)
{
    return (bitmap[page / 8] >> (page % 8)) & 1;
}

static void bitmap_set(uint8_t *bitmap, uint64_t page)
{
    bitmap[page / 8] |= (uint8_t)(1U << (page % 8));
}

static void bitmap_clear(uint8_t *bitmap, uint64_t page)
{
    bitmap[page / 8] &= (uint8_t)~(1U << (page % 8));
}

static void reserve_range(uint64_t start, uint64_t end)
{
    if (end <= start)
        return;

    uint64_t first = start / PAGE_SIZE;
    uint64_t last = (end + PAGE_SIZE - 1) / PAGE_SIZE;

    if (last > TOTAL_PAGES)
        last = TOTAL_PAGES;

    for (uint64_t page = first; page < last; page++)
    {
        if (bitmap_get(&used_bitmap[0], page) == 0)
        {
            bitmap_set(used_bitmap, page);
            free_pages--;
        }
    }
}

static int mark_usable_range(uint64_t start, uint64_t length)
{
    if (length == 0 || start >= MEMORY_LIMIT)
        return 1;

    if (length > UINT64_MAX - start)
        return 0;

    uint64_t end = start + length;

    if (end > MEMORY_LIMIT)
        end = MEMORY_LIMIT;

    /* Only use complete 4 KiB pages. */
    uint64_t first = (start + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t last = end / PAGE_SIZE;

    for (uint64_t page = first; page < last; page++)
    {
        if (!bitmap_get(usable_bitmap, page))
        {
            bitmap_set(usable_bitmap, page);
            bitmap_clear(used_bitmap, page);
            free_pages++;
        }
    }

    return 1;
}

static int is_protected(uint64_t address)
{
    uint64_t kernel_start = (uint64_t)(uintptr_t)&__kernel_start;
    uint64_t kernel_end = (uint64_t)(uintptr_t)&__kernel_end;

    if (address < 0x100000ULL)
        return 1;

    if (address >= kernel_start && address < kernel_end)
        return 1;

    if (address < boot_info_end &&
        address + PAGE_SIZE > boot_info_start)
        return 1;

    return 0;
}

int pmm_init(uint64_t address)
{
    /* Initially, every page is considered occupied. */
    for (uint64_t i = 0; i < BITMAP_SIZE; i++)
    {
        used_bitmap[i] = 0xFF;
        usable_bitmap[i] = 0;
    }

    free_pages = 0;

    if (address == 0 || address >= MEMORY_LIMIT)
        return 0;

    const multiboot2_info_t *info =
        (const multiboot2_info_t *)(uintptr_t)address;

    uint32_t total_size = info->total_size;

    if (total_size < 16 ||
        total_size > MEMORY_LIMIT - address)
        return 0;

    boot_info_start = address;
    boot_info_end = address + total_size;

    const uint8_t *base = (const uint8_t *)info;
    uint32_t offset = 8;
    int found_mmap = 0;
    int found_end = 0;

    while (offset <= total_size - 8)
    {
        const multiboot2_tag_t *tag =
            (const multiboot2_tag_t *)(base + offset);

        if (tag->size < 8 ||
            tag->size > total_size - offset)
            return 0;

        if (tag->type == MULTIBOOT2_TAG_END)
        {
            found_end = 1;
            break;
        }

        if (tag->type == MULTIBOOT2_TAG_MMAP)
        {
            const multiboot2_mmap_tag_t *mmap =
                (const multiboot2_mmap_tag_t *)tag;

            if (tag->size < sizeof(multiboot2_mmap_tag_t) ||
                mmap->entry_size < sizeof(multiboot2_mmap_entry_t) ||
                (tag->size - sizeof(multiboot2_mmap_tag_t))
                    % mmap->entry_size != 0)
                return 0;

            uint32_t count =
                (tag->size - sizeof(multiboot2_mmap_tag_t))
                / mmap->entry_size;

            const uint8_t *entry_ptr =
                (const uint8_t *)mmap
                + sizeof(multiboot2_mmap_tag_t);

            for (uint32_t i = 0; i < count; i++)
            {
                const multiboot2_mmap_entry_t *entry =
                    (const multiboot2_mmap_entry_t *)entry_ptr;

                if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE)
                {
                    if (!mark_usable_range(
                            entry->address,
                            entry->length))
                        return 0;
                }

                entry_ptr += mmap->entry_size;
            }

            found_mmap = 1;
        }

        uint32_t next = (tag->size + 7U) & ~7U;

        if (next > total_size - offset)
            return 0;

        offset += next;
    }

    if (!found_mmap || !found_end)
        return 0;

    /* Protect low memory, kernel, and Multiboot2 information. */
    reserve_range(0, 0x100000ULL);

    reserve_range(
        (uint64_t)(uintptr_t)&__kernel_start,
        (uint64_t)(uintptr_t)&__kernel_end
    );

    reserve_range(boot_info_start, boot_info_end);

    return 1;
}

uint64_t pmm_alloc_page(void)
{
    for (uint64_t page = 0; page < TOTAL_PAGES; page++)
    {
        if (bitmap_get(usable_bitmap, page) &&
            !bitmap_get(used_bitmap, page))
        {
            bitmap_set(used_bitmap, page);
            free_pages--;

            return page * PAGE_SIZE;
        }
    }

    return 0;
}

int pmm_free_page(uint64_t address)
{
    if (address == 0 ||
        address >= MEMORY_LIMIT ||
        address % PAGE_SIZE != 0)
        return 0;

    if (is_protected(address))
        return 0;

    uint64_t page = address / PAGE_SIZE;

    if (!bitmap_get(usable_bitmap, page) ||
        !bitmap_get(used_bitmap, page))
        return 0;

    bitmap_clear(used_bitmap, page);
    free_pages++;

    return 1;
}

uint64_t pmm_get_free_pages(void)
{
    return free_pages;
}
