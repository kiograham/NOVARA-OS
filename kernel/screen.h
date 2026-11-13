#ifndef NOVARA_SCREEN_H
#define NOVARA_SCREEN_H

#include <stdint.h>

void screen_clear(void);
void screen_put_char(char c);
void screen_write(const char *text);
void screen_set_color(uint8_t color);

#endif
