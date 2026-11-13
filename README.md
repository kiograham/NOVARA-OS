# NOVARA OS

**A hobby operating system built from scratch using Assembly and C.**

NOVARA OS is an experimental x86_64 operating system project developed for learning and exploring low-level systems programming, kernel development, and computer architecture.

## Project Goals

The goal of NOVARA OS is to build a minimal operating system from the ground up, without relying on an existing operating system kernel.

## Technologies

- **Architecture:** x86_64
- **Bootloader:** GRUB
- **Boot Protocol:** Multiboot2
- **Assembly:** NASM
- **Kernel Language:** C
- **Linker:** GNU LD
- **Virtualization:** QEMU

## Current Features

- [x] Multiboot2 boot support
- [x] x86_64 long mode transition
- [x] Initial kernel boot
- [x] VGA text-mode output
- [x] Screen Manager
- [ ] Keyboard driver
- [ ] Interrupt handling
- [ ] Memory management
- [ ] Basic shell

## Project Structure

```text
NOVARA-OS/
├── boot/
│   └── boot.asm
├── iso/
│   └── boot/
│       └── grub/
│           └── grub.cfg
├── kernel/
│   ├── kernel.c
│   ├── screen.c
│   └── screen.h
├── scripts/
├── linker.ld
└── README.md
```

## Development Environment

NOVARA OS is currently developed and tested on Ubuntu using QEMU.

## Project Status

**Early Development — Initial Kernel Boot Successful**

NOVARA OS has successfully booted in QEMU and displayed its initial kernel messages.

## Disclaimer

NOVARA OS is an experimental hobby operating system. It is currently under active development and is not intended for production use.

---

*Built from scratch, one layer at a time.*