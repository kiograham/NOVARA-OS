
BITS 32

section .multiboot2
align 8

header_start:
    dd 0xe85250d6
    dd 0
    dd header_end - header_start
    dd -(0xe85250d6 + (header_end - header_start))

    ; Request Multiboot2 Memory Map (information type 6)
    dw 1
    dw 0
    dd 12
    dd 6
    align 8

    ; End Tag
    dw 0
    dw 0
    dd 8

header_end:


section .bss
align 4096

p4_table:
    resb 4096

p3_table:
    resb 4096

p2_table:
    resb 4096

align 4
multiboot_info_ptr:
    resd 1

multiboot_magic:
    resd 1

alignb 16
stack_bottom:
    resb 16384
stack_top:


section .text
global _start
extern kernel_main

_start:
    cli

    ; Simpan Boot Magic dari GRUB
    mov [multiboot_magic], eax

    ; Simpan alamat informasi Multiboot2 dari GRUB
    mov [multiboot_info_ptr], ebx

    mov esp, stack_top

    ; Setup PML4
    mov eax, p3_table
    or eax, 3
    mov [p4_table], eax

    ; Setup PDPT
    mov eax, p2_table
    or eax, 3
    mov [p3_table], eax

    ; Identity map first 1 GiB using 2 MiB pages
    mov ecx, 0

.map_pages:
    mov eax, ecx
    shl eax, 21
    or eax, 0x83

    mov [p2_table + ecx * 8], eax
    mov dword [p2_table + ecx * 8 + 4], 0

    inc ecx
    cmp ecx, 512
    jne .map_pages

    ; Configure FPU
    mov eax, cr0
    and eax, 0xFFFFFFFB
    or eax, 1 << 1
    mov cr0, eax

    ; Enable PAE and SSE
    mov eax, cr4
    or eax, (1 << 5) | (1 << 9) | (1 << 10)
    mov cr4, eax

    ; Load page table
    mov eax, p4_table
    mov cr3, eax

    ; Enable Long Mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Load GDT
    lgdt [gdt64.pointer]

    ; Enter 64-bit mode
    jmp 0x08:long_mode_start


BITS 64

long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov rsp, stack_top

    ; Argumen pertama: Boot Magic
    mov edi, dword [rel multiboot_magic]

    ; Argumen kedua: alamat informasi Multiboot2
    mov esi, dword [rel multiboot_info_ptr]

    call kernel_main

.hang:
    hlt
    jmp .hang


section .rodata

align 8

gdt64:
    dq 0
    dq 0x00AF9A000000FFFF
    dq 0x00CF92000000FFFF

.pointer:
    dw $ - gdt64 - 1
    dq gdt64
