#include "exception.h"
#include "../screen.h"
#include "../panic.h"

static const char *exception_names[32] =
{
    "Division By Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

static void print_hex(uint64_t value)
{
    const char hex[] = "0123456789ABCDEF";

    for (int i = 60; i >= 0; i -= 4)
    {
        screen_put_char(hex[(value >> i) & 0xF]);
    }
}

void exception_handler(exception_frame_t *frame)
{
    const char *reason = "Unknown CPU Exception";

    if (frame->vector < 32)
    {
        reason = exception_names[frame->vector];
    }

    kernel_panic_begin(reason);

    screen_write("Exception Details\n\n");

    screen_write("Vector:     0x");
    print_hex(frame->vector);

    screen_write("\nError Code: 0x");
    print_hex(frame->error_code);

    screen_write("\nRIP:        0x");
    print_hex(frame->rip);

    screen_write("\nCS:         0x");
    print_hex(frame->cs);

    screen_write("\nRFLAGS:     0x");
    print_hex(frame->rflags);

    screen_write("\n\n");

    kernel_panic_halt();
}
