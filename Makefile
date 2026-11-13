NASM := nasm
CC := gcc
LD := ld
QEMU := qemu-system-x86_64

CFLAGS := -m64 -ffreestanding -mno-red-zone -fno-pie \
          -fno-stack-protector -mcmodel=small \
          -O2 -Wall -Wextra

.PHONY: all iso run check clean

all: build/novara.iso

build:
	mkdir -p build

build/boot.o: boot/boot.asm | build
	$(NASM) -f elf64 $< -o $@

build/kernel.o: kernel/kernel.c kernel/screen.h kernel/interrupts/idt.h kernel/memory/multiboot2.h kernel/memory/vmm.h kernel/memory/heap.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/screen.o: kernel/screen.c kernel/screen.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/idt.o: kernel/interrupts/idt.c kernel/interrupts/idt.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/exception.o: kernel/interrupts/exception.c kernel/interrupts/exception.h kernel/screen.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/panic.o: kernel/panic.c kernel/panic.h kernel/screen.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/multiboot2.o: kernel/memory/multiboot2.c kernel/memory/multiboot2.h kernel/screen.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/pmm.o: kernel/memory/pmm.c kernel/memory/pmm.h kernel/memory/multiboot2.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/vmm.o: kernel/memory/vmm.c kernel/memory/vmm.h kernel/memory/pmm.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/heap.o: kernel/memory/heap.c kernel/memory/heap.h kernel/memory/pmm.h kernel/memory/vmm.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/isr.o: kernel/interrupts/isr.asm | build
	$(NASM) -f elf64 $< -o $@

build/kernel.elf: build/boot.o build/kernel.o build/screen.o build/idt.o build/exception.o build/panic.o build/multiboot2.o build/pmm.o build/vmm.o build/heap.o build/isr.o linker.ld
	$(LD) -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld -o $@ $(filter %.o,$^)

build/novara.iso: build/kernel.elf iso/boot/grub/grub.cfg
	mkdir -p iso/boot
	cp build/kernel.elf iso/boot/kernel.elf
	grub-mkrescue -o $@ iso

iso: build/novara.iso

check: build/kernel.elf
	grub-file --is-x86-multiboot2 build/kernel.elf

run: build/novara.iso
	$(QEMU) -m 256M -cdrom build/novara.iso -boot d -no-reboot -no-shutdown

clean:
	rm -rf build/*
	rm -f iso/boot/kernel.elf
