#include "idt.h"

extern void isr_default(void);

static idt_entry_t idt[IDT_ENTRIES];
static idtr_t idtr;

static void idt_set_gate(uint8_t vector, uint64_t handler)
{
    idt[vector].offset_low  = handler & 0xFFFF;
    idt[vector].selector    = 0x08;
    idt[vector].ist         = 0;
    idt[vector].type_attr   = 0x8E;
    idt[vector].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[vector].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

void idt_init(void)
{
    uint64_t handler = (uint64_t)isr_default;

    for (uint16_t i = 0; i < IDT_ENTRIES; i++)
    {
        idt_set_gate(i, handler);
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint64_t)&idt;

    __asm__ volatile ("lidt %0" : : "m"(idtr));
}
