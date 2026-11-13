#ifndef NOVARA_PANIC_H
#define NOVARA_PANIC_H

void kernel_panic_begin(const char *reason);

__attribute__((noreturn))
void kernel_panic_halt(void);

__attribute__((noreturn))
void kernel_panic(const char *reason);

#endif
