#ifndef NOVARA_HEAP_H
#define NOVARA_HEAP_H

#include <stdint.h>

#define HEAP_START       0x40000000ULL
#define HEAP_PAGE_SIZE   4096ULL
#define HEAP_ALIGNMENT  16ULL

int heap_init(void);
void *kmalloc(uint64_t size);
int kfree(void *address);
uint64_t heap_get_used(void);
uint64_t heap_get_capacity(void);

#endif
