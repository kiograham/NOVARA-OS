BITS 64

section .text
global isr_default

isr_default:
    cli

.hang:
    hlt
    jmp .hang
