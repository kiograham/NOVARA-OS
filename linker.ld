ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 1M;
    __kernel_start = .;

    .multiboot2 :
    {
        KEEP(*(.multiboot2))
    } :text

    .text :
    {
        *(.text)
        *(.text.*)
    } :text

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    } :text

    . = ALIGN(0x1000);
    __kernel_data_start = .;

    .data :
    {
        *(.data)
        *(.data.*)
    } :data

    .bss (NOLOAD) :
    {
        . = ALIGN(0x1000);
        __bss_start = .;

        *(COMMON)
        *(.bss)
        *(.bss.*)

        . = ALIGN(0x1000);
        __bss_end = .;
    } :data

    __kernel_end = .;

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.note*)
        *(.comment)
    }
}
