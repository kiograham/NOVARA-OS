#include <stdint.h>
#include "multiboot2.h"
#include "../screen.h"

#define NOVARA_IDENTITY_MAP_LIMIT 0x40000000ULL

static void print_hex(uint64_t value)
{
    const char hex[] = "0123456789ABCDEF";

    for (int i = 15; i >= 0; i--)
    {
        screen_put_char(hex[(value >> (i * 4)) & 0xF]);
    }
}

static void print_type(uint32_t type)
{
    if (type == MULTIBOOT2_MEMORY_AVAILABLE)
        screen_write("AVAILABLE");
    else if (type == MULTIBOOT2_MEMORY_RESERVED)
        screen_write("RESERVED");
    else
        screen_write("OTHER");
}

void multiboot2_print_memory_map(uint64_t address)
{
    if (address == 0 || address >= NOVARA_IDENTITY_MAP_LIMIT)
    {
        screen_write("ERROR: Invalid Multiboot2 address\n");
        return;
    }

    const multiboot2_info_t *info =
        (const multiboot2_info_t *)(uintptr_t)address;

    uint32_t total_size = info->total_size;

    if (total_size < 16 ||
        total_size > NOVARA_IDENTITY_MAP_LIMIT - address)
    {
        screen_write("ERROR: Invalid information size\n");
        return;
    }

    const uint8_t *base = (const uint8_t *)info;
    uint32_t offset = 8;
    uint32_t count = 0;
    uint32_t found = 0;

    screen_write("\n=== NOVARA MEMORY MAP ===\n");

    while (offset <= total_size - 8)
    {
        const multiboot2_tag_t *tag =
            (const multiboot2_tag_t *)(base + offset);

        if (tag->size < 8 || tag->size > total_size - offset)
        {
            screen_write("ERROR: Invalid tag\n");
            return;
        }

        if (tag->type == MULTIBOOT2_TAG_END)
            break;

        if (tag->type == MULTIBOOT2_TAG_MMAP)
        {
            const multiboot2_mmap_tag_t *mmap =
                (const multiboot2_mmap_tag_t *)tag;

            if (tag->size < sizeof(multiboot2_mmap_tag_t) ||
                mmap->entry_size < sizeof(multiboot2_mmap_entry_t) ||
                (tag->size - sizeof(multiboot2_mmap_tag_t))
                    % mmap->entry_size != 0)
            {
                screen_write("ERROR: Invalid Memory Map\n");
                return;
            }

            uint32_t entries_count =
                (tag->size - sizeof(multiboot2_mmap_tag_t))
                / mmap->entry_size;

            const uint8_t *entry_ptr =
                (const uint8_t *)mmap
                + sizeof(multiboot2_mmap_tag_t);

            for (uint32_t i = 0; i < entries_count; i++)
            {
                const multiboot2_mmap_entry_t *entry =
                    (const multiboot2_mmap_entry_t *)entry_ptr;

                screen_write("#");
                screen_put_char("0123456789ABCDEF"[(count >> 4) & 0xF]);
                screen_put_char("0123456789ABCDEF"[count & 0xF]);

                screen_write(" A=");
                print_hex(entry->address);

                screen_write(" L=");
                print_hex(entry->length);

                screen_put_char(' ');
                print_type(entry->type);
                screen_put_char('\n');

                count++;
                entry_ptr += mmap->entry_size;
            }

            found = 1;
        }

        uint32_t next = (tag->size + 7) & ~7U;

        if (next > total_size - offset)
        {
            screen_write("ERROR: Invalid tag alignment\n");
            return;
        }

        offset += next;
    }

    if (!found)
    {
        screen_write("ERROR: Memory Map not found\n");
        return;
    }

    screen_write("Total entries: ");
    print_hex(count);
    screen_write("\nMemory Map parsing completed.\n");
}
