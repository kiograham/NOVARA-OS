#ifndef NOVARA_VMM_H
#define NOVARA_VMM_H

#include <stdint.h>

/* Page size */
#define VMM_PAGE_SIZE 4096ULL

/* Page flags */
#define VMM_PRESENT   (1ULL << 0)
#define VMM_WRITABLE  (1ULL << 1)
#define VMM_USER      (1ULL << 2)
#define VMM_NO_EXEC   (1ULL << 63)

/* Initialize Virtual Memory Manager */
int vmm_init(void);

/* Map virtual address to physical address */
int vmm_map_page(uint64_t virtual_address,
                 uint64_t physical_address,
                 uint64_t flags);

/* Remove virtual page mapping */
int vmm_unmap_page(uint64_t virtual_address);

/* Translate virtual address to physical address */
uint64_t vmm_get_physical_address(uint64_t virtual_address);
int vmm_is_page_no_exec(uint64_t virtual_address);

#endif
