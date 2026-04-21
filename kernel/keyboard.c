#include "keyboard.h"
#include "common.h"

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

static const char scancode_map[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n', 0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*',
    0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0
};

static uint8_t read_scancode(void) {
    while ((inb(KEYBOARD_STATUS_PORT) & 0x01) == 0) {
        asm volatile("pause");
    }
    return inb(KEYBOARD_DATA_PORT);
}

void keyboard_init(void) {
    while (inb(KEYBOARD_STATUS_PORT) & 0x01) {
        (void)inb(KEYBOARD_DATA_PORT);
    }
}

char keyboard_getchar(void) {
    for (;;) {
        uint8_t scancode = read_scancode();
        if (scancode & 0x80) {
            continue;
        }
        if (scancode < sizeof(scancode_map)) {
            char c = scancode_map[scancode];
            if (c != 0) {
                return c;
            }
        }
    }
}