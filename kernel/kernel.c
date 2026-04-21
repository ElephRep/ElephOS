#include "terminal.h"
#include "keyboard.h"
#include "shell.h"
#include "fs.h"

void kernel_main(void) {
    terminal_init();
    keyboard_init();
    fs_init();

    terminal_setcolor(make_color(COLOR_LIGHT_GREEN, COLOR_BLACK));
    terminal_writeline("======================================");
    terminal_setcolor(make_color(COLOR_LIGHT_CYAN, COLOR_BLACK));
    terminal_writeline("      Welcome to Eleph OS v1.0");
    terminal_setcolor(make_color(COLOR_LIGHT_GREEN, COLOR_BLACK));
    terminal_writeline("======================================");
    terminal_setcolor(make_color(COLOR_LIGHT_GREY, COLOR_BLACK));
    terminal_writeline("");

    shell_run();

    while (1) {
        asm volatile("hlt");
    }
}