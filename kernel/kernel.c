// kernel/kernel.c
#include "include/kernel.h"

// Видеопамять VGA (текстовый режим 80x25)
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer = (uint16_t*) VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        uint16_t entry = (uint16_t) c | (0x0F << 8);  // белый на черном
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = entry;
        cursor_x++;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= VGA_HEIGHT) {
        // Простой скролл (очищаем экран)
        for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            vga_buffer[i] = ' ' | (0x0F << 8);
        }
        cursor_x = 0;
        cursor_y = 0;
    }
}

void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void kernel_main(uint32_t magic, void* multiboot_info) {
    vga_print("Hello from 64-bit Elephkernel!\n");
    vga_print("Running in Long Mode\n");
    
    // Простой тест: считаем до 10
    vga_print("Counting: ");
    for (int i = 0; i < 10; i++) {
        vga_putchar('0' + i);
        vga_putchar(' ');
        
        for (volatile int j = 0; j < 10000000; j++);
    }
    vga_print("\nDone!\n");
    
    while (1) {
        // Бесконечный цикл
        asm volatile ("hlt");
    }
}