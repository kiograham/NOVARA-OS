#ifndef NOVARA_MULTIBOOT2_H
#define NOVARA_MULTIBOOT2_H

#include <stdint.h>

/* Multiboot2 information header */
typedef struct
{
    uint32_t total_size;
    uint32_t reserved;
} multiboot2_info_t;

/* Generic Multiboot2 tag */
typedef struct
{
    uint32_t type;
    uint32_t size;
} multiboot2_tag_t;

/* Multiboot2 memory map tag */
typedef struct
{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} multiboot2_mmap_tag_t;

/* Individual memory map entry */
typedef struct
{
    uint64_t address;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} multiboot2_mmap_entry_t;

#define MULTIBOOT2_TAG_END 0
#define MULTIBOOT2_TAG_MMAP 6

#define MULTIBOOT2_MEMORY_AVAILABLE 1
#define MULTIBOOT2_MEMORY_RESERVED 2

void multiboot2_print_memory_map(uint64_t address);

#endif
