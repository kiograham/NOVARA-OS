#include <stdint.h>
#include "heap.h"
#include "pmm.h"
#include "vmm.h"

#define HEAP_MAX_SIZE (16ULL * 1024ULL * 1024ULL)
#define HEAP_PAGE_FLAGS (VMM_PRESENT | VMM_WRITABLE)

typedef struct heap_block
{
    uint64_t size;
    uint32_t free;
    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

static heap_block_t *first_block = 0;
static heap_block_t *last_block = 0;
static uint64_t heap_capacity = 0;
static uint64_t heap_used = 0;
static int heap_initialized = 0;

static uint64_t align_size(uint64_t size)
{
    if (size > UINT64_MAX - (HEAP_ALIGNMENT - 1))
        return 0;

    return (size + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);
}

static int grow_heap(void)
{
    if (heap_capacity >= HEAP_MAX_SIZE)
        return 0;

    uint64_t virtual_address = HEAP_START + heap_capacity;
    uint64_t physical_address = pmm_alloc_page();

    if (physical_address == 0)
        return 0;

    if (!vmm_map_page(virtual_address, physical_address, HEAP_PAGE_FLAGS))
    {
        pmm_free_page(physical_address);
        return 0;
    }

    if (last_block != 0 && last_block->free)
    {
        last_block->size += HEAP_PAGE_SIZE;
    }
    else
    {
        heap_block_t *block = (heap_block_t *)(uintptr_t)virtual_address;

        block->size = HEAP_PAGE_SIZE - sizeof(heap_block_t);
        block->free = 1;
        block->next = 0;
        block->prev = last_block;

        if (last_block != 0)
            last_block->next = block;
        else
            first_block = block;

        last_block = block;
    }

    heap_capacity += HEAP_PAGE_SIZE;
    return 1;
}

int heap_init(void)
{
    if (heap_initialized)
        return 1;

    first_block = 0;
    last_block = 0;
    heap_capacity = 0;
    heap_used = 0;

    if (!grow_heap())
        return 0;

    heap_initialized = 1;
    return 1;
}

void *kmalloc(uint64_t size)
{
    if (!heap_initialized || size == 0)
        return 0;

    uint64_t aligned_size = align_size(size);

    if (aligned_size == 0)
        return 0;

    while (1)
    {
        heap_block_t *block = first_block;

        while (block != 0)
        {
            if (block->free && block->size >= aligned_size)
                break;

            block = block->next;
        }

        if (block != 0)
        {
            if (block->size >= aligned_size + sizeof(heap_block_t) + HEAP_ALIGNMENT)
            {
                heap_block_t *split = (heap_block_t *)(
                    (uintptr_t)(block + 1) + aligned_size
                );

                split->size = block->size - aligned_size - sizeof(heap_block_t);
                split->free = 1;
                split->next = block->next;
                split->prev = block;

                if (block->next != 0)
                    block->next->prev = split;
                else
                    last_block = split;

                block->next = split;
                block->size = aligned_size;
            }

            block->free = 0;
            heap_used += block->size;
            return (void *)(block + 1);
        }

        if (!grow_heap())
            return 0;
    }
}

int kfree(void *address)
{
    if (!heap_initialized || address == 0)
        return 0;

    heap_block_t *block = first_block;

    while (block != 0)
    {
        if ((void *)(block + 1) == address)
            break;

        block = block->next;
    }

    if (block == 0 || block->free)
        return 0;

    block->free = 1;
    heap_used -= block->size;

    if (block->prev != 0 && block->prev->free)
    {
        heap_block_t *previous = block->prev;

        previous->size += sizeof(heap_block_t) + block->size;
        previous->next = block->next;

        if (block->next != 0)
            block->next->prev = previous;
        else
            last_block = previous;

        block = previous;
    }

    if (block->next != 0 && block->next->free)
    {
        heap_block_t *next = block->next;

        block->size += sizeof(heap_block_t) + next->size;
        block->next = next->next;

        if (next->next != 0)
            next->next->prev = block;
        else
            last_block = block;
    }

    return 1;
}

uint64_t heap_get_used(void)
{
    return heap_used;
}

uint64_t heap_get_capacity(void)
{
    return heap_capacity;
}
