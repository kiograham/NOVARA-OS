#ifndef NOVARA_PMM_H
#define NOVARA_PMM_H

#include <stdint.h>

/* Initialize Physical Memory Manager */
int pmm_init(uint64_t multiboot_info_address);

/* Allocate one 4 KiB physical page */
uint64_t pmm_alloc_page(void);

/* Release one 4 KiB physical page */
int pmm_free_page(uint64_t address);

/* Return the number of free physical pages */
uint64_t pmm_get_free_pages(void);

#endif
