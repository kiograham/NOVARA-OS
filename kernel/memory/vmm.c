#include <stdint.h>
#include "vmm.h"
#include "pmm.h"

#define MEMORY_LIMIT       0x40000000ULL
#define ADDRESS_MASK       0x000FFFFFFFFFF000ULL
#define LARGE_2M_MASK      0x000FFFFFFFE00000ULL
#define LARGE_1G_MASK      0x000FFFFFC0000000ULL
#define PAGE_PRESENT       (1ULL << 0)
#define PAGE_WRITABLE      (1ULL << 1)
#define PAGE_USER          (1ULL << 2)
#define PAGE_HUGE          (1ULL << 7)

#define PHYSICAL_ADDRESS_LIMIT (1ULL << 52)

static void zero_memory(void *address, uint64_t size)
{
    uint8_t *ptr = (uint8_t *)address;

    for (uint64_t i = 0; i < size; i++)
        ptr[i] = 0;
}

static int is_canonical(uint64_t address)
{
    uint64_t upper = address >> 48;
    uint64_t sign = (address >> 47) & 1;

    if (sign)
        return upper == 0xFFFF;

    return upper == 0;
}

static volatile uint64_t *physical_to_table(uint64_t address)
{
    if (address >= MEMORY_LIMIT || address % VMM_PAGE_SIZE != 0)
        return 0;

    return (volatile uint64_t *)(uintptr_t)address;
}

static uint64_t allocate_table(void)
{
    uint64_t address = pmm_alloc_page();

    if (address == 0 || address >= MEMORY_LIMIT)
        return 0;

    zero_memory((void *)(uintptr_t)address, VMM_PAGE_SIZE);

    return address;
}

static uint64_t read_cr3(void)
{
    uint64_t value;

    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));

    return value & ADDRESS_MASK;
}

static void invalidate_page(uint64_t address)
{
    __asm__ volatile ("invlpg (%0)" : : "r"(address) : "memory");
}

/* Get an existing table or allocate a new one. */
static volatile uint64_t *get_child_table(
    volatile uint64_t *entry,
    int user_access)
{
    uint64_t value = *entry;

    if (!(value & PAGE_PRESENT))
    {
        uint64_t new_table = allocate_table();

        if (new_table == 0)
            return 0;

        value = new_table | PAGE_PRESENT | PAGE_WRITABLE;

        if (user_access)
            value |= PAGE_USER;

        *entry = value;
    }
    else
    {
        if (value & PAGE_HUGE)
            return 0;

        if (user_access && !(value & PAGE_USER))
        {
            value |= PAGE_USER;
            *entry = value;
        }
    }

    return physical_to_table(value & ADDRESS_MASK);
}

/* Convert one 2 MiB page into 512 pages of 4 KiB. */
static int split_large_page(volatile uint64_t *pde)
{
    uint64_t old_entry = *pde;

    if (!(old_entry & PAGE_PRESENT) ||
        !(old_entry & PAGE_HUGE))
        return 0;

    uint64_t table_address = allocate_table();

    if (table_address == 0)
        return 0;

    volatile uint64_t *pt = physical_to_table(table_address);

    if (pt == 0)
        return 0;

    uint64_t physical_base = old_entry & LARGE_2M_MASK;

    uint64_t flags = (old_entry & 0x1FFFULL) | (old_entry & VMM_NO_EXEC);

    /* Convert large-page PAT bit to 4 KiB PAT bit. */
    int pat = (flags & (1ULL << 12)) != 0;

    flags &= ~PAGE_HUGE;
    flags &= ~(1ULL << 12);

    if (pat)
        flags |= PAGE_HUGE;

    for (uint64_t i = 0; i < 512; i++)
    {
        pt[i] = (physical_base + i * VMM_PAGE_SIZE) | flags;
    }

    *pde = table_address |
           PAGE_PRESENT |
           PAGE_WRITABLE |
           (old_entry & PAGE_USER) |
           (old_entry & VMM_NO_EXEC);

    return 1;
}

static void cpuid(
    uint32_t leaf,
    uint32_t subleaf,
    uint32_t *a,
    uint32_t *b,
    uint32_t *c,
    uint32_t *d)
{
    __asm__ volatile (
        "cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "a"(leaf), "c"(subleaf)
    );
}

static int enable_nx(void)
{
    uint32_t a, b, c, d;

    cpuid(0x80000000U, 0, &a, &b, &c, &d);

    if (a < 0x80000001U)
        return 0;

    cpuid(0x80000001U, 0, &a, &b, &c, &d);

    /* Extended CPUID EDX bit 20 indicates NX support. */
    if (!(d & (1U << 20)))
        return 0;

    uint32_t low, high;

    __asm__ volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(0xC0000080U)
    );

    low |= (1U << 11);

    __asm__ volatile (
        "wrmsr"
        :
        : "c"(0xC0000080U), "a"(low), "d"(high)
    );

    __asm__ volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(0xC0000080U)
    );

    return (low & (1U << 11)) != 0;
}

int vmm_init(void)
{
    if (!enable_nx())
        return 0;

    uint64_t root = read_cr3();

    if (root == 0 || root >= MEMORY_LIMIT)
        return 0;

    volatile uint64_t *pml4 = physical_to_table(root);

    if (pml4 == 0 || !(pml4[0] & PAGE_PRESENT))
        return 0;

    return 1;
}

int vmm_map_page(
    uint64_t virtual_address,
    uint64_t physical_address,
    uint64_t flags)
{
    if (!is_canonical(virtual_address) ||
        virtual_address % VMM_PAGE_SIZE != 0 ||
        physical_address % VMM_PAGE_SIZE != 0 ||
        physical_address >= PHYSICAL_ADDRESS_LIMIT)
        return 0;

    if (flags & ~(VMM_PRESENT | VMM_WRITABLE | VMM_USER | VMM_NO_EXEC))
        return 0;

    uint64_t root = read_cr3();

    volatile uint64_t *pml4 = physical_to_table(root);

    if (pml4 == 0)
        return 0;

    uint64_t pml4_index = (virtual_address >> 39) & 0x1FF;
    uint64_t pdpt_index = (virtual_address >> 30) & 0x1FF;
    uint64_t pd_index = (virtual_address >> 21) & 0x1FF;
    uint64_t pt_index = (virtual_address >> 12) & 0x1FF;

    int user_access = (flags & VMM_USER) != 0;

    volatile uint64_t *pdpt =
        get_child_table(&pml4[pml4_index], user_access);

    if (pdpt == 0)
        return 0;

    volatile uint64_t *pd =
        get_child_table(&pdpt[pdpt_index], user_access);

    if (pd == 0)
        return 0;

    volatile uint64_t *pde = &pd[pd_index];

    if ((*pde & PAGE_PRESENT) && (*pde & PAGE_HUGE))
    {
        if (!split_large_page(pde))
            return 0;
    }

    volatile uint64_t *pt =
        get_child_table(pde, user_access);

    if (pt == 0)
        return 0;

    if (pt[pt_index] & PAGE_PRESENT)
        return 0;

    uint64_t entry_flags = PAGE_PRESENT;

    if (flags & VMM_WRITABLE)
        entry_flags |= PAGE_WRITABLE;

    if (flags & VMM_USER)
        entry_flags |= PAGE_USER;

    if (flags & VMM_NO_EXEC)
        entry_flags |= VMM_NO_EXEC;

    pt[pt_index] = physical_address | entry_flags;

    invalidate_page(virtual_address);

    return 1;
}

int vmm_unmap_page(uint64_t virtual_address)
{
    if (!is_canonical(virtual_address) ||
        virtual_address % VMM_PAGE_SIZE != 0)
        return 0;

    volatile uint64_t *pml4 =
        physical_to_table(read_cr3());

    if (pml4 == 0)
        return 0;

    uint64_t i4 = (virtual_address >> 39) & 0x1FF;
    uint64_t i3 = (virtual_address >> 30) & 0x1FF;
    uint64_t i2 = (virtual_address >> 21) & 0x1FF;
    uint64_t i1 = (virtual_address >> 12) & 0x1FF;

    if (!(pml4[i4] & PAGE_PRESENT))
        return 0;

    volatile uint64_t *pdpt =
        physical_to_table(pml4[i4] & ADDRESS_MASK);

    if (pdpt == 0 || !(pdpt[i3] & PAGE_PRESENT))
        return 0;

    if (pdpt[i3] & PAGE_HUGE)
        return 0;

    volatile uint64_t *pd =
        physical_to_table(pdpt[i3] & ADDRESS_MASK);

    if (pd == 0 || !(pd[i2] & PAGE_PRESENT))
        return 0;

    volatile uint64_t *pde = &pd[i2];

    if (*pde & PAGE_HUGE)
    {
        if (!split_large_page(pde))
            return 0;
    }

    volatile uint64_t *pt =
        physical_to_table(*pde & ADDRESS_MASK);

    if (pt == 0 || !(pt[i1] & PAGE_PRESENT))
        return 0;

    pt[i1] = 0;

    invalidate_page(virtual_address);

    return 1;
}

uint64_t vmm_get_physical_address(uint64_t virtual_address)
{
    if (!is_canonical(virtual_address))
        return 0;

    volatile uint64_t *pml4 =
        physical_to_table(read_cr3());

    if (pml4 == 0)
        return 0;

    uint64_t i4 = (virtual_address >> 39) & 0x1FF;
    uint64_t i3 = (virtual_address >> 30) & 0x1FF;
    uint64_t i2 = (virtual_address >> 21) & 0x1FF;
    uint64_t i1 = (virtual_address >> 12) & 0x1FF;

    if (!(pml4[i4] & PAGE_PRESENT))
        return 0;

    volatile uint64_t *pdpt =
        physical_to_table(pml4[i4] & ADDRESS_MASK);

    if (pdpt == 0 || !(pdpt[i3] & PAGE_PRESENT))
        return 0;

    uint64_t pdpt_entry = pdpt[i3];

    if (pdpt_entry & PAGE_HUGE)
    {
        return (pdpt_entry & LARGE_1G_MASK) |
               (virtual_address & 0x3FFFFFFFULL);
    }

    volatile uint64_t *pd =
        physical_to_table(pdpt_entry & ADDRESS_MASK);

    if (pd == 0 || !(pd[i2] & PAGE_PRESENT))
        return 0;

    uint64_t pd_entry = pd[i2];

    if (pd_entry & PAGE_HUGE)
    {
        return (pd_entry & LARGE_2M_MASK) |
               (virtual_address & 0x1FFFFFULL);
    }

    volatile uint64_t *pt =
        physical_to_table(pd_entry & ADDRESS_MASK);

    if (pt == 0 || !(pt[i1] & PAGE_PRESENT))
        return 0;

    return (pt[i1] & ADDRESS_MASK) |
           (virtual_address & 0xFFFULL);
}

int vmm_is_page_no_exec(uint64_t virtual_address)
{
    if (!is_canonical(virtual_address))
        return -1;

    volatile uint64_t *pml4 =
        physical_to_table(read_cr3());

    if (pml4 == 0)
        return -1;

    uint64_t i4 = (virtual_address >> 39) & 0x1FF;
    uint64_t i3 = (virtual_address >> 30) & 0x1FF;
    uint64_t i2 = (virtual_address >> 21) & 0x1FF;
    uint64_t i1 = (virtual_address >> 12) & 0x1FF;

    uint64_t entry = pml4[i4];

    if (!(entry & PAGE_PRESENT))
        return -1;

    int no_exec = (entry & VMM_NO_EXEC) != 0;

    volatile uint64_t *pdpt =
        physical_to_table(entry & ADDRESS_MASK);

    if (pdpt == 0)
        return -1;

    entry = pdpt[i3];

    if (!(entry & PAGE_PRESENT))
        return -1;

    if (entry & VMM_NO_EXEC)
        no_exec = 1;

    if (entry & PAGE_HUGE)
        return no_exec;

    volatile uint64_t *pd =
        physical_to_table(entry & ADDRESS_MASK);

    if (pd == 0)
        return -1;

    entry = pd[i2];

    if (!(entry & PAGE_PRESENT))
        return -1;

    if (entry & VMM_NO_EXEC)
        no_exec = 1;

    if (entry & PAGE_HUGE)
        return no_exec;

    volatile uint64_t *pt =
        physical_to_table(entry & ADDRESS_MASK);

    if (pt == 0)
        return -1;

    entry = pt[i1];

    if (!(entry & PAGE_PRESENT))
        return -1;

    if (entry & VMM_NO_EXEC)
        no_exec = 1;

    return no_exec;
}
