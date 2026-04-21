#include "gfx.h"
#include "keyboard.h"
#include "fs.h"
#include "terminal.h"
#include "common.h"
#include <stdint.h>

#define VGA_MEM ((uint8_t*)0xA0000)
#define W 320
#define H 200
#define FONT_SCALE 2
#define CHAR_W (5 * FONT_SCALE)
#define CHAR_H (7 * FONT_SCALE)
#define CHAR_STEP_X (6 * FONT_SCALE)
#define LINE_STEP_Y (8 * FONT_SCALE)

// Цвета Windows 9x
#define WIN95_BG 0x01         // Синий фон рабочего стола
#define WIN95_BAR 0x09        // Синий заголовок окна
#define WIN95_GRAY 0x07       // Серый фон окон
#define WIN95_DKGRAY 0x08     // Темно-серый для границ
#define WIN95_WHITE 0x0F      // Белый
#define WIN95_BLACK 0x00      // Черный
#define WIN95_BLUE 0x01       // Синий
#define WIN95_GREEN 0x02      // Зеленый
#define WIN95_RED 0x04        // Красный
#define WIN95_YELLOW 0x0E     // Желтый
#define WIN95_CYAN 0x03       // Голубой

// Состояния GUI
static int gui_screen = 0; // 0=desktop, 1=notepad, 2=explorer, 3=calc, 4=minesweeper
static int gui_running = 1;

// Для блокнота
static char notepad_lines[20][48];
static int notepad_line_count = 0;
static int notepad_scroll = 0;

// Для калькулятора
static char calc_display[16] = "0";
static int calc_value = 0;
static int calc_op = 0; // 0=none, 1=+, 2=-, 3=*, 4=/
static int calc_new_num = 0;

// Для сапера
static char mines[10][10];
static char mines_revealed[10][10];
static int mines_game_over = 0;
static int mines_won = 0;

// Forward declarations
static int my_strlen(const char* s);
static void my_draw_text(int x, int y, const char* s, uint8_t c);

static void set_mode_13h(void) {
    static const uint8_t misc = 0x63;
    static const uint8_t seq[5] = {
        0x03, 0x01, 0x0F, 0x00, 0x0E
    };
    static const uint8_t crtc[25] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x41,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x9C, 0x0E, 0x8F, 0x28, 0x40,
        0x96, 0xB9, 0xA3, 0xFF, 0x00
    };
    static const uint8_t gc[9] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF
    };
    static const uint8_t ac[21] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
        0x01, 0x00, 0x0F, 0x00, 0x00
    };
    int i;

    outb(0x3C2, misc);
    outb(0x3C4, 0x00);
    outb(0x3C5, 0x01);

    for (i = 0; i < 5; i++) {
        outb(0x3C4, (uint8_t)i);
        outb(0x3C5, seq[i]);
    }

    outb(0x3C4, 0x00);
    outb(0x3C5, 0x03);

    outb(0x3D4, 0x03);
    outb(0x3D5, (uint8_t)(inb(0x3D5) | 0x80));
    outb(0x3D4, 0x11);
    outb(0x3D5, (uint8_t)(inb(0x3D5) & 0x7F));

    for (i = 0; i < 25; i++) {
        outb(0x3D4, (uint8_t)i);
        outb(0x3D5, crtc[i]);
    }

    for (i = 0; i < 9; i++) {
        outb(0x3CE, (uint8_t)i);
        outb(0x3CF, gc[i]);
    }

    for (i = 0; i < 21; i++) {
        (void)inb(0x3DA);
        outb(0x3C0, (uint8_t)i);
        outb(0x3C0, ac[i]);
    }
    (void)inb(0x3DA);
    outb(0x3C0, 0x20);
}

static void pset(int x, int y, uint8_t c) {
    if (x < 0 || y < 0 || x >= W || y >= H) return;
    VGA_MEM[y * W + x] = c;
}

static void fill_rect(int x, int y, int w, int h, uint8_t c) {
    int yy, xx;
    for (yy = 0; yy < h; yy++) {
        for (xx = 0; xx < w; xx++) {
            pset(x + xx, y + yy, c);
        }
    }
}

static void draw_hline(int x, int y, int w, uint8_t c) {
    int i;
    for (i = 0; i < w; i++) {
        pset(x + i, y, c);
    }
}

static void draw_vline(int x, int y, int h, uint8_t c) {
    int i;
    for (i = 0; i < h; i++) {
        pset(x, y + i, c);
    }
}

static void draw_win95_border(int x, int y, int w, int h) {
    // Светлая граница (сверху и слева)
    draw_hline(x, y, w, WIN95_WHITE);
    draw_vline(x, y, h, WIN95_WHITE);
    draw_hline(x + 1, y + 1, w - 2, WIN95_WHITE);
    draw_vline(x + 1, y + 1, h - 2, WIN95_WHITE);
    
    // Темная граница (снизу и справа)
    draw_hline(x, y + h - 1, w, WIN95_BLACK);
    draw_vline(x + w - 1, y, h, WIN95_BLACK);
    draw_hline(x + 2, y + h - 2, w - 4, WIN95_DKGRAY);
    draw_vline(x + w - 2, y + 2, h - 4, WIN95_DKGRAY);
}

static void draw_win95_button(int x, int y, int w, int h, const char* text, uint8_t text_color) {
    // Фон кнопки
    fill_rect(x, y, w, h, WIN95_GRAY);
    
    // Границы кнопки
    draw_hline(x, y, w, WIN95_WHITE);
    draw_vline(x, y, h, WIN95_WHITE);
    draw_hline(x + 1, y + 1, w - 2, WIN95_WHITE);
    draw_vline(x + 1, y + 1, h - 2, WIN95_WHITE);
    draw_hline(x, y + h - 1, w, WIN95_BLACK);
    draw_vline(x + w - 1, y, h, WIN95_BLACK);
    
    // Текст
    if (text) {
        int tx = x + (w - (int)my_strlen(text) * CHAR_STEP_X) / 2;
        int ty = y + (h - CHAR_H) / 2;
        my_draw_text(tx, ty, text, text_color);
    }
}

static uint8_t glyph_row(char ch, int row) {
    if (row < 0 || row > 6) return 0;
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 32);
    switch (ch) {
        case 'A': { uint8_t r[7]={14,17,17,31,17,17,17}; return r[row]; }
        case 'B': { uint8_t r[7]={30,17,17,30,17,17,30}; return r[row]; }
        case 'C': { uint8_t r[7]={14,17,16,16,16,17,14}; return r[row]; }
        case 'D': { uint8_t r[7]={30,17,17,17,17,17,30}; return r[row]; }
        case 'E': { uint8_t r[7]={31,16,16,30,16,16,31}; return r[row]; }
        case 'F': { uint8_t r[7]={31,16,16,30,16,16,16}; return r[row]; }
        case 'G': { uint8_t r[7]={14,17,16,23,17,17,14}; return r[row]; }
        case 'H': { uint8_t r[7]={17,17,17,31,17,17,17}; return r[row]; }
        case 'I': { uint8_t r[7]={31,4,4,4,4,4,31}; return r[row]; }
        case 'J': { uint8_t r[7]={1,1,1,1,17,17,14}; return r[row]; }
        case 'K': { uint8_t r[7]={17,18,20,24,20,18,17}; return r[row]; }
        case 'L': { uint8_t r[7]={16,16,16,16,16,16,31}; return r[row]; }
        case 'M': { uint8_t r[7]={17,27,21,21,17,17,17}; return r[row]; }
        case 'N': { uint8_t r[7]={17,25,21,19,17,17,17}; return r[row]; }
        case 'O': { uint8_t r[7]={14,17,17,17,17,17,14}; return r[row]; }
        case 'P': { uint8_t r[7]={30,17,17,30,16,16,16}; return r[row]; }
        case 'Q': { uint8_t r[7]={14,17,17,17,21,18,13}; return r[row]; }
        case 'R': { uint8_t r[7]={30,17,17,30,20,18,17}; return r[row]; }
        case 'S': { uint8_t r[7]={15,16,16,14,1,1,30}; return r[row]; }
        case 'T': { uint8_t r[7]={31,4,4,4,4,4,4}; return r[row]; }
        case 'U': { uint8_t r[7]={17,17,17,17,17,17,14}; return r[row]; }
        case 'V': { uint8_t r[7]={17,17,17,17,17,10,4}; return r[row]; }
        case 'W': { uint8_t r[7]={17,17,17,21,21,27,17}; return r[row]; }
        case 'X': { uint8_t r[7]={17,17,10,4,10,17,17}; return r[row]; }
        case 'Y': { uint8_t r[7]={17,17,10,4,4,4,4}; return r[row]; }
        case 'Z': { uint8_t r[7]={31,1,2,4,8,16,31}; return r[row]; }
        case '0': { uint8_t r[7]={14,17,19,21,25,17,14}; return r[row]; }
        case '1': { uint8_t r[7]={4,12,4,4,4,4,14}; return r[row]; }
        case '2': { uint8_t r[7]={14,17,1,2,4,8,31}; return r[row]; }
        case '3': { uint8_t r[7]={30,1,1,14,1,1,30}; return r[row]; }
        case '4': { uint8_t r[7]={2,6,10,18,31,2,2}; return r[row]; }
        case '5': { uint8_t r[7]={31,16,16,30,1,1,30}; return r[row]; }
        case '6': { uint8_t r[7]={14,16,16,30,17,17,14}; return r[row]; }
        case '7': { uint8_t r[7]={31,1,2,4,8,8,8}; return r[row]; }
        case '8': { uint8_t r[7]={14,17,17,14,17,17,14}; return r[row]; }
        case '9': { uint8_t r[7]={14,17,17,15,1,1,14}; return r[row]; }
        case '-': { uint8_t r[7]={0,0,0,31,0,0,0}; return r[row]; }
        case '>': { uint8_t r[7]={16,8,4,2,4,8,16}; return r[row]; }
        case '<': { uint8_t r[7]={1,2,4,8,4,2,1}; return r[row]; }
        case ':': { uint8_t r[7]={0,4,0,0,0,4,0}; return r[row]; }
        case '.': { uint8_t r[7]={0,0,0,0,0,0,4}; return r[row]; }
        case ',': { uint8_t r[7]={0,0,0,0,0,4,8}; return r[row]; }
        case '!': { uint8_t r[7]={4,4,4,4,4,0,4}; return r[row]; }
        case '?': { uint8_t r[7]={14,17,1,2,4,0,4}; return r[row]; }
        case '(': { uint8_t r[7]={2,4,8,8,8,4,2}; return r[row]; }
        case ')': { uint8_t r[7]={8,4,2,2,2,4,8}; return r[row]; }
        case '_': { uint8_t r[7]={0,0,0,0,0,0,31}; return r[row]; }
        case '/': { uint8_t r[7]={1,2,4,8,16,0,0}; return r[row]; }
        case ' ': default: return 0;
    }
}

static void draw_char(int x, int y, char ch, uint8_t c) {
    int row, col;
    for (row = 0; row < 7; row++) {
        uint8_t bits = glyph_row(ch, row);
        for (col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col))) {
                int dy, dx;
                for (dy = 0; dy < FONT_SCALE; dy++) {
                    for (dx = 0; dx < FONT_SCALE; dx++) {
                        pset(x + col * FONT_SCALE + dx, y + row * FONT_SCALE + dy, c);
                    }
                }
            }
        }
    }
}

static void my_draw_text(int x, int y, const char* s, uint8_t c) {
    while (*s) {
        draw_char(x, y, *s, c);
        x += CHAR_STEP_X;
        s++;
    }
}

static int my_strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// Алиасы для совместимости
static void draw_text(int x, int y, const char* s, uint8_t c) {
    my_draw_text(x, y, s, c);
}

static int strlen(const char* s) {
    return my_strlen(s);
}

// Отрисовка рабочего стола Windows 9x
static void draw_win95_desktop(void) {
    // Фон рабочего стола (бирюзовый)
    fill_rect(0, 0, W, H, WIN95_BG);
    
    // Панель задач внизу
    fill_rect(0, H - 28, W, 28, WIN95_GRAY);
    draw_hline(0, H - 28, W, WIN95_WHITE);
    draw_hline(0, H - 27, W, WIN95_WHITE);
    
    // Кнопка "Пуск"
    fill_rect(2, H - 26, 50, 22, WIN95_GRAY);
    draw_win95_border(2, H - 26, 50, 22);
    draw_text(6, H - 20, "Start", WIN95_BLACK);
    
    // Часы
    fill_rect(260, H - 26, 58, 22, WIN95_GRAY);
    draw_win95_border(260, H - 26, 58, 22);
    draw_text(264, H - 20, "14:30", WIN95_BLACK);
    
    // Иконки на рабочем столе
    // Мой компьютер
    fill_rect(10, 10, 40, 40, WIN95_GRAY);
    draw_win95_border(10, 10, 40, 40);
    draw_text(14, 20, "My", WIN95_BLACK);
    draw_text(14, 28, "Comp", WIN95_BLACK);
    
    // Корзина
    fill_rect(10, 60, 40, 40, WIN95_GRAY);
    draw_win95_border(10, 60, 40, 40);
    draw_text(14, 75, "Trash", WIN95_BLACK);
    
    // Блокнот
    fill_rect(10, 110, 40, 40, WIN95_GRAY);
    draw_win95_border(10, 110, 40, 40);
    draw_text(14, 120, "Note", WIN95_BLACK);
    draw_text(14, 128, "pad", WIN95_BLACK);
    
    // Калькулятор
    fill_rect(70, 10, 40, 40, WIN95_GRAY);
    draw_win95_border(70, 10, 40, 40);
    draw_text(74, 20, "Calc", WIN95_BLACK);
    
    // Сапер
    fill_rect(70, 60, 40, 40, WIN95_GRAY);
    draw_win95_border(70, 60, 40, 40);
    draw_text(74, 75, "Mine", WIN95_BLACK);
}

// Отрисовка окна в стиле Windows 9x
static void draw_win95_window(int x, int y, int w, int h, const char* title) {
    // Фон окна
    fill_rect(x, y, w, h, WIN95_GRAY);
    
    // Заголовок окна (синий градиент)
    fill_rect(x + 2, y + 2, w - 4, 16, WIN95_BAR);
    draw_text(x + 4, y + 5, title, WIN95_WHITE);
    
    // Кнопка закрытия
    fill_rect(x + w - 18, y + 3, 14, 14, WIN95_GRAY);
    draw_win95_border(x + w - 18, y + 3, 14, 14);
    draw_text(x + w - 15, y + 6, "X", WIN95_BLACK);
    
    // Границы окна
    draw_win95_border(x, y, w, h);
}

// Блокнот
static void draw_notepad(void) {
    draw_win95_window(20, 20, 280, 140, "Notepad - Eleph OS");
    
    // Область текста
    fill_rect(24, 40, 272, 100, WIN95_WHITE);
    draw_win95_border(24, 40, 272, 100);
    
    // Текст
    for (int i = 0; i < 10 && (notepad_scroll + i) < notepad_line_count; i++) {
        draw_text(28, 44 + i * LINE_STEP_Y, notepad_lines[notepad_scroll + i], WIN95_BLACK);
    }
    
    // Кнопки
    draw_win95_button(24, 145, 40, 14, "New", WIN95_BLACK);
    draw_win95_button(70, 145, 40, 14, "Clear", WIN95_BLACK);
    draw_win95_button(120, 145, 40, 14, "Up", WIN95_BLACK);
    draw_win95_button(166, 145, 40, 14, "Down", WIN95_BLACK);
    draw_win95_button(220, 145, 40, 14, "Back", WIN95_BLACK);
}

// Проводник
static void draw_explorer(void) {
    draw_win95_window(10, 10, 300, 160, "Explorer - My Computer");
    
    // Список файлов
    fill_rect(14, 30, 292, 110, WIN95_WHITE);
    draw_win95_border(14, 30, 292, 110);
    
    int count = fs_file_count();
    if (count == 0) {
        draw_text(20, 50, "No files found", WIN95_BLACK);
    } else {
        for (int i = 0; i < count && i < 8; i++) {
            const char* name;
            if (fs_list_name(i, &name)) {
                draw_text(20, 34 + i * LINE_STEP_Y, name, WIN95_BLACK);
            }
        }
    }
    
    draw_win95_button(250, 145, 50, 14, "Back", WIN95_BLACK);
}

// Калькулятор
static void draw_calculator(void) {
    draw_win95_window(60, 30, 200, 140, "Calculator");
    
    // Дисплей
    fill_rect(70, 50, 180, 24, WIN95_WHITE);
    draw_win95_border(70, 50, 180, 24);
    draw_text(80, 55, calc_display, WIN95_BLACK);
    
    // Кнопки
    draw_win95_button(70, 80, 30, 20, "7", WIN95_BLACK);
    draw_win95_button(110, 80, 30, 20, "8", WIN95_BLACK);
    draw_win95_button(150, 80, 30, 20, "9", WIN95_BLACK);
    draw_win95_button(190, 80, 50, 20, "+", WIN95_BLACK);
    
    draw_win95_button(70, 105, 30, 20, "4", WIN95_BLACK);
    draw_win95_button(110, 105, 30, 20, "5", WIN95_BLACK);
    draw_win95_button(150, 105, 30, 20, "6", WIN95_BLACK);
    draw_win95_button(190, 105, 50, 20, "-", WIN95_BLACK);
    
    draw_win95_button(70, 130, 30, 20, "1", WIN95_BLACK);
    draw_win95_button(110, 130, 30, 20, "2", WIN95_BLACK);
    draw_win95_button(150, 130, 30, 20, "3", WIN95_BLACK);
    draw_win95_button(190, 130, 50, 20, "*", WIN95_BLACK);
    
    draw_win95_button(70, 155, 70, 20, "0", WIN95_BLACK);
    draw_win95_button(150, 155, 30, 20, "=", WIN95_BLACK);
    draw_win95_button(190, 155, 50, 20, "/", WIN95_BLACK);
    
    draw_win95_button(250, 145, 50, 14, "Back", WIN95_BLACK);
}

// Сапер
static void draw_minesweeper(void) {
    draw_win95_window(40, 20, 240, 160, "Minesweeper");
    
    // Игровое поле
    fill_rect(50, 40, 220, 120, WIN95_GRAY);
    
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            int x = 50 + i * 22;
            int y = 40 + j * 12;
            
            if (mines_revealed[i][j]) {
                fill_rect(x, y, 22, 12, WIN95_WHITE);
                if (mines[i][j] == '*') {
                    draw_text(x + 8, y + 2, "*", WIN95_RED);
                } else if (mines[i][j] >= '0' && mines[i][j] <= '8') {
                    draw_text(x + 8, y + 2, "0", WIN95_BLACK); // Упрощено
                }
            } else {
                fill_rect(x, y, 22, 12, WIN95_GRAY);
                draw_win95_border(x, y, 22, 12);
            }
        }
    }
    
    draw_win95_button(250, 145, 50, 14, "Back", WIN95_BLACK);
}

// Инициализация сапера
static void mines_init(void) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            mines[i][j] = '0';
            mines_revealed[i][j] = 0;
        }
    }
    mines_game_over = 0;
    mines_won = 0;
    
    // Разместить мины (10 мин)
    int placed = 0;
    while (placed < 10) {
        int r = prng_next() % 10;
        int c = prng_next() % 10;
        if (mines[r][c] != '*') {
            mines[r][c] = '*';
            placed++;
        }
    }
}

// Главный цикл GUI
void gfx_run_desktop(void) {
    terminal_clear();
    terminal_writeline("=== Windows 9x GUI ===");
    terminal_writeline("Launching desktop...");
    terminal_writeline("");
    gui_running = 1;
    gui_screen = 0;
    
    // Инициализация сапера
    mines_init();
    
    while (gui_running) {
        switch (gui_screen) {
            case 0: // Рабочий стол
                draw_win95_desktop();
                break;
                
            case 1: // Блокнот
                draw_notepad();
                break;
                
            case 2: // Проводник
                draw_explorer();
                break;
                
            case 3: // Калькулятор
                draw_calculator();
                break;
                
            case 4: // Сапер
                draw_minesweeper();
                break;
        }
        
        // Обработка ввода
        char c = keyboard_getchar();
        
        switch (gui_screen) {
            case 0: // Рабочий стол
                if (c == '1') gui_screen = 1; // Блокнот
                else if (c == '2') gui_screen = 2; // Проводник
                else if (c == '3') gui_screen = 3; // Калькулятор
                else if (c == '4') gui_screen = 4; // Сапер
                else if (c == 27) gui_running = 0; // ESC - выход
                break;
                
            case 1: // Блокнот
                if (c == 27) gui_screen = 0;
                else if (c == 'n' || c == 'N') {
                    notepad_line_count = 0;
                    notepad_scroll = 0;
                }
                else if (c == 'c' || c == 'C') {
                    notepad_line_count = 0;
                    notepad_scroll = 0;
                }
                else if (c == 'u' || c == 'U') {
                    if (notepad_scroll > 0) notepad_scroll--;
                }
                else if (c == 'd' || c == 'D') {
                    if (notepad_scroll < notepad_line_count - 10) notepad_scroll++;
                }
                else if (c == 'b' || c == 'B') gui_screen = 0;
                break;
                
            case 2: // Проводник
                if (c == 27 || c == 'b' || c == 'B') gui_screen = 0;
                break;
                
            case 3: // Калькулятор
                if (c == 27 || c == 'b' || c == 'B') gui_screen = 0;
                else if (c >= '0' && c <= '9') {
                    calc_value = calc_value * 10 + (c - '0');
                    calc_display[0] = (char)c;
                    calc_display[1] = '\0';
                }
                else if (c == '+') calc_op = 1;
                else if (c == '-') calc_op = 2;
                else if (c == '*') calc_op = 3;
                else if (c == '/') calc_op = 4;
                else if (c == '=' || c == '\n') {
                    // Простая операция
                    int result = calc_value;
                    calc_display[0] = (char)('0' + result);
                    calc_display[1] = '\0';
                    calc_value = result;
                    calc_op = 0;
                }
                else if (c == 'c' || c == 'C') {
                    calc_value = 0;
                    calc_op = 0;
                    calc_display[0] = '0';
                    calc_display[1] = '\0';
                }
                break;
                
            case 4: // Сапер
                if (c == 27 || c == 'b' || c == 'B') gui_screen = 0;
                else if (c >= '1' && c <= '9') {
                    int pos = c - '1';
                    int r = pos / 10;
                    int col = pos % 10;
                    if (r < 10 && col < 10) {
                        mines_revealed[r][col] = 1;
                        if (mines[r][col] == '*') {
                            mines_game_over = 1;
                        }
                    }
                }
                else if (c == 'r' || c == 'R') {
                    mines_init();
                }
                break;
        }
    }
    
    // Возврат в текстовый режим
    // (упрощенно - просто черный экран)
    fill_rect(0, 0, W, H, 0);
}