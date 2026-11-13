#ifndef NOVARA_IDT_H
#define NOVARA_IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

typedef struct
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed)) idt_entry_t;

typedef struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

void idt_init(void);

#endif
