#include "terminal.h"

static uint16_t* vga_buffer = (uint16_t*)VGA_ADDRESS;
static int term_row = 0;
static int term_col = 0;
static uint8_t term_color = 0;

// РЕАЛИЗАЦИЯ make_color
uint8_t make_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

static inline uint16_t make_char(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

void terminal_init(void) {
    term_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
    terminal_clear();
}

void terminal_setcolor(uint8_t color) {
    term_color = color;
}

void terminal_clear(void) {
    for (int i = 0; i < TERM_WIDTH * TERM_HEIGHT; i++) {
        vga_buffer[i] = make_char(' ', term_color);
    }
    term_row = 0;
    term_col = 0;
}

void terminal_scroll(void) {
    for (int row = 1; row < TERM_HEIGHT; row++) {
        for (int col = 0; col < TERM_WIDTH; col++) {
            int src = row * TERM_WIDTH + col;
            int dst = (row - 1) * TERM_WIDTH + col;
            vga_buffer[dst] = vga_buffer[src];
        }
    }
    
    int last_row = (TERM_HEIGHT - 1) * TERM_WIDTH;
    for (int col = 0; col < TERM_WIDTH; col++) {
        vga_buffer[last_row + col] = make_char(' ', term_color);
    }
    
    term_row = TERM_HEIGHT - 1;
    term_col = 0;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\r') {
        term_col = 0;
    } else if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            vga_buffer[term_row * TERM_WIDTH + term_col] = make_char(' ', term_color);
        }
    } else {
        vga_buffer[term_row * TERM_WIDTH + term_col] = make_char(c, term_color);
        term_col++;
    }
    
    if (term_col >= TERM_WIDTH) {
        term_col = 0;
        term_row++;
    }
    
    if (term_row >= TERM_HEIGHT) {
        terminal_scroll();
    }
}

void terminal_write(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

void terminal_writeline(const char* str) {
    terminal_write(str);
    terminal_putchar('\n');
}