#include "shell.h"
#include "terminal.h"
#include "keyboard.h"
#include "fs.h"
#include "gfx.h"
#include "common.h"
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#define CMD_BUFFER_SIZE 256
#define GAME_MIN 1
#define GAME_MAX 100

static void cmd_help(char* args);
static void cmd_clear(char* args);
static void cmd_echo(char* args);
static void cmd_reboot(char* args);
static void cmd_about(char* args);
static void cmd_color(char* args);
static void cmd_add(char* args);
static void cmd_sub(char* args);
static void cmd_mul(char* args);
static void cmd_div(char* args);
static void cmd_game(char* args);
static void cmd_snake(char* args);
static void cmd_tictactoe(char* args);
static void cmd_ls(char* args);
static void cmd_touch(char* args);
static void cmd_write(char* args);
static void cmd_cat(char* args);
static void cmd_rm(char* args);
static void cmd_cli(char* args);
static void cmd_gui(char* args);
static void cmd_date(char* args);
static void cmd_mem(char* args);
static void execute_command(char* cmdline);

struct command {
    const char* name;
    void (*func)(char* args);
    const char* help;
};

static const struct command commands[] = {
    {"help", cmd_help, "Show this help"},
    {"clear", cmd_clear, "Clear screen"},
    {"echo", cmd_echo, "Echo text"},
    {"about", cmd_about, "Show OS info"},
    {"color", cmd_color, "Set text color: color <0-15>"},
    {"add", cmd_add, "Add two integers: add <a> <b>"},
    {"sub", cmd_sub, "Subtract two integers: sub <a> <b>"},
    {"mul", cmd_mul, "Multiply two integers: mul <a> <b>"},
    {"div", cmd_div, "Divide two integers: div <a> <b>"},
    {"game", cmd_game, "Guess game: game start|guess <n>|quit"},
    {"snake", cmd_snake, "Play Snake game"},
    {"tictactoe", cmd_tictactoe, "Play Tic-Tac-Toe"},
    {"ls", cmd_ls, "List files"},
    {"touch", cmd_touch, "Create file: touch <name>"},
    {"write", cmd_write, "Write file: write <name> <text>"},
    {"cat", cmd_cat, "Read file: cat <name>"},
    {"rm", cmd_rm, "Delete file: rm <name>"},
    {"cli", cmd_cli, "Run mini CLI shell"},
    {"gui", cmd_gui, "Run graphical shell"},
    {"date", cmd_date, "Show fake date"},
    {"mem", cmd_mem, "Show memory info"},
    {"reboot", cmd_reboot, "Reboot system"},
    {NULL, NULL, NULL}
};

uint32_t prng_state = 0xC0FFEE11u;
static int game_active = 0;
static int game_secret = 0;
static int game_tries = 0;

uint32_t prng_next(void) {
    prng_state = prng_state * 1664525u + 1013904223u;
    return prng_state;
}

static int parse_int(const char* s, int* out_value) {
    int sign = 1;
    int value = 0;
    int has_digits = 0;

    while (*s == ' ') s++;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    while (*s >= '0' && *s <= '9') {
        has_digits = 1;
        value = value * 10 + (*s - '0');
        s++;
    }

    while (*s == ' ') s++;
    if (!has_digits || *s != '\0') {
        return 0;
    }

    *out_value = sign * value;
    return 1;
}

static int parse_two_ints(char* s, int* a, int* b) {
    char* second = s;
    while (*second == ' ') second++;
    while (*second && *second != ' ') second++;
    if (*second == '\0') {
        return 0;
    }
    *second = '\0';
    second++;
    while (*second == ' ') second++;
    if (*second == '\0') {
        return 0;
    }
    return parse_int(s, a) && parse_int(second, b);
}

static int split_name_and_text(char* s, char** name, char** text) {
    while (*s == ' ') s++;
    if (*s == '\0') return 0;
    *name = s;
    while (*s && *s != ' ') s++;
    if (*s == '\0') return 0;
    *s = '\0';
    s++;
    while (*s == ' ') s++;
    if (*s == '\0') return 0;
    *text = s;
    return 1;
}

static void print_int(int value) {
    char buf[16];
    int i = 0;
    int neg = 0;
    unsigned int x;

    if (value == 0) {
        terminal_write("0");
        return;
    }

    if (value < 0) {
        neg = 1;
        // Исправление для INT_MIN
        if (value == INT_MIN) {
            terminal_write("-2147483648");
            return;
        }
        x = (unsigned int)(-value);
    } else {
        x = (unsigned int)value;
    }

    while (x > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (x % 10));
        x /= 10;
    }

    if (neg) {
        terminal_write("-");
    }

    while (i > 0) {
        terminal_putchar(buf[--i]);
    }
}

static void cmd_help(char* args) {
    (void)args;
    terminal_writeline("Available commands:");
    for (int i = 0; commands[i].name != NULL; i++) {
        terminal_write("  ");
        terminal_write(commands[i].name);
        terminal_write(" - ");
        terminal_writeline(commands[i].help);
    }
} 

static void cmd_clear(char* args) {
    (void)args;
    terminal_clear();
}

static void cmd_echo(char* args) {
    if (args && *args) {
        terminal_writeline(args);
    } else {
        terminal_writeline("");
    }
}

static void cmd_about(char* args) {
    (void)args;
    terminal_writeline("Eleph OS v1.1");
    terminal_writeline("2.1 kernel");
    terminal_writeline("6 build");
}

static void cmd_color(char* args) {
    int color = 0;
    if (!parse_int(args, &color) || color < 0 || color > 15) {
        terminal_writeline("Usage: color <0-15>");
        return;
    }

    terminal_setcolor(make_color((enum vga_color)color, COLOR_BLACK));
    terminal_writeline("Color updated.");
}

static void cmd_add(char* args) {
    int a = 0;
    int b = 0;

    if (!parse_two_ints(args, &a, &b)) {
        terminal_writeline("Usage: add <a> <b>");
        return;
    }

    terminal_write("Result: ");
    print_int(a + b);
    terminal_putchar('\n');
}

static void cmd_sub(char* args) {
    int a = 0;
    int b = 0;

    if (!parse_two_ints(args, &a, &b)) {
        terminal_writeline("Usage: sub <a> <b>");
        return;
    }

    terminal_write("Result: ");
    print_int(a - b);
    terminal_putchar('\n');
}

static void cmd_mul(char* args) {
    int a = 0;
    int b = 0;

    if (!parse_two_ints(args, &a, &b)) {
        terminal_writeline("Usage: mul <a> <b>");
        return;
    }

    terminal_write("Result: ");
    print_int(a * b);
    terminal_putchar('\n');
}

static void cmd_div(char* args) {
    int a = 0;
    int b = 0;

    if (!parse_two_ints(args, &a, &b)) {
        terminal_writeline("Usage: div <a> <b>");
        return;
    }

    if (b == 0) {
        terminal_writeline("Error: Division by zero!");
        return;
    }

    terminal_write("Result: ");
    print_int(a / b);
    terminal_putchar('\n');
}

static void game_start(void) {
    game_active = 1;
    game_tries = 0;
    game_secret = (int)(prng_next() % GAME_MAX) + GAME_MIN;
    terminal_writeline("Game started: guess a number from 1 to 100.");
    terminal_writeline("Use: game guess <number>");
}

static void cmd_game(char* args) {
    char* sub = args;
    while (*sub == ' ') sub++;

    if (*sub == '\0' || streq(sub, "start")) {
        game_start();
        return;
    }

    if (streq(sub, "quit")) {
        if (game_active) {
            game_active = 0;
            terminal_writeline("Game ended.");
        } else {
            terminal_writeline("No active game. Start with: game start");
        }
        return;
    }

    if (sub[0] == 'g' && sub[1] == 'u' && sub[2] == 'e' && sub[3] == 's' &&
        sub[4] == 's' && sub[5] == ' ') {
        int guess = 0;
        if (!game_active) {
            terminal_writeline("No active game. Start with: game start");
            return;
        }

        if (!parse_int(sub + 6, &guess)) {
            terminal_writeline("Usage: game guess <number>");
            return;
        }

        game_tries++;
        if (guess < game_secret) {
            terminal_writeline("Too low.");
            return;
        }
        if (guess > game_secret) {
            terminal_writeline("Too high.");
            return;
        }

        terminal_write("You win in ");
        print_int(game_tries);
        terminal_writeline(" tries!");
        game_active = 0;
        return;
    }

    terminal_writeline("Usage: game start | game guess <n> | game quit");
}

static void cmd_ls(char* args) {
    int i;
    const char* name;
    int count;
    (void)args;
    count = fs_file_count();
    if (count == 0) {
        terminal_writeline("(empty)");
        return;
    }
    for (i = 0; i < count; i++) {
        if (fs_list_name(i, &name)) {
            terminal_writeline(name);
        }
    }
}

static void cmd_touch(char* args) {
    while (*args == ' ') args++;
    if (*args == '\0') {
        terminal_writeline("Usage: touch <name>");
        return;
    }
    if (fs_create(args)) {
        terminal_writeline("File created.");
    } else {
        terminal_writeline("Cannot create file.");
    }
}

static void cmd_write(char* args) {
    char* name;
    char* text;
    if (!split_name_and_text(args, &name, &text)) {
        terminal_writeline("Usage: write <name> <text>");
        return;
    }
    if (!fs_write(name, text)) {
        terminal_writeline("File not found.");
        return;
    }
    terminal_writeline("Saved.");
}

static void cmd_cat(char* args) {
    const char* data;
    while (*args == ' ') args++;
    if (*args == '\0') {
        terminal_writeline("Usage: cat <name>");
        return;
    }
    data = fs_read(args);
    if (!data) {
        terminal_writeline("File not found.");
        return;
    }
    terminal_writeline(data);
}

static void cmd_rm(char* args) {
    while (*args == ' ') args++;
    if (*args == '\0') {
        terminal_writeline("Usage: rm <name>");
        return;
    }
    if (fs_delete(args)) {
        terminal_writeline("File deleted.");
    } else {
        terminal_writeline("File not found.");
    }
}

static void cli_print_help(void) {
    terminal_writeline("Mini CLI commands:");
    terminal_writeline("  help         - Show this help");
    terminal_writeline("  whoami       - Print current user");
    terminal_writeline("  pwd          - Print virtual path");
    terminal_writeline("  time         - Print fake uptime tick");
    terminal_writeline("  exit         - Return to Eleph shell");
}

static int cli_starts_with(const char* s, const char* p) {
    while (*p) {
        if (*s != *p) return 0;
        s++;
        p++;
    }
    return 1;
}

static void cli_run(void) {
    char buffer[CMD_BUFFER_SIZE];
    int tick = 0;

    terminal_writeline("Mini CLI started. Type 'help'.");
    while (1) {
        terminal_write("cli$ ");
        {
            int pos = 0;
            while (1) {
                char c = keyboard_getchar();
                if (c == '\n') {
                    terminal_putchar('\n');
                    break;
                } else if (c == '\b') {
                    if (pos > 0) {
                        pos--;
                        terminal_putchar('\b');
                    }
                } else if (c >= 0x20 && c < 0x7F && pos < CMD_BUFFER_SIZE - 1) {
                    buffer[pos++] = c;
                    terminal_putchar(c);
                }
            }
            buffer[pos] = '\0';
        }

        {
            char* cmd = buffer;
            while (*cmd == ' ') cmd++;
            if (*cmd == '\0') {
                continue;
            } else if (streq(cmd, "help")) {
                cli_print_help();
            } else if (streq(cmd, "whoami")) {
                terminal_writeline("root");
            } else if (streq(cmd, "pwd")) {
                terminal_writeline("/");
            } else if (streq(cmd, "time")) {
                terminal_write("uptime tick: ");
                print_int(tick++);
                terminal_putchar('\n');
            } else if (streq(cmd, "exit")) {
                terminal_writeline("Leaving mini CLI.");
                return;
            } else if (cli_starts_with(cmd, "echo ")) {
                terminal_writeline(cmd + 5);
            } else {
                terminal_writeline("Unknown mini CLI command.");
            }
        }
    }
}

static void cmd_cli(char* args) {
    (void)args;
    cli_run();
}

static void gui_header(void) {
    terminal_setcolor(make_color(COLOR_WHITE, COLOR_BLUE));
    terminal_writeline(" Eleph Desktop                                         [x] ");
    terminal_setcolor(make_color(COLOR_LIGHT_GREY, COLOR_BLACK));
}

static void gui_draw_window(const char* title) {
    terminal_writeline("+------------------------------------------------------------+");
    terminal_write("| ");
    terminal_write(title);
    terminal_writeline("");
    terminal_writeline("+------------------------------------------------------------+");
}

static void gui_list_files(void) {
    int i;
    int count = fs_file_count();
    const char* name;
    terminal_writeline("");
    terminal_writeline("[File manager]");
    if (count == 0) {
        terminal_writeline("No files.");
        return;
    }
    for (i = 0; i < count; i++) {
        if (fs_list_name(i, &name)) {
            terminal_write("- ");
            terminal_writeline(name);
        }
    }
}

static void gui_calculator_app(void) {
    char buf[CMD_BUFFER_SIZE];
    terminal_clear();
    gui_header();
    gui_draw_window("Calculator App");
    terminal_writeline("Enter: <a> <b>  (example: 12 7)");
    terminal_writeline("Type 'back' to desktop.");

    while (1) {
        int pos = 0;
        int a = 0, b = 0;
        terminal_write("calc> ");
        while (1) {
            char c = keyboard_getchar();
            if (c == '\n') {
                terminal_putchar('\n');
                break;
            } else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    terminal_putchar('\b');
                }
            } else if (c >= 0x20 && c < 0x7F && pos < CMD_BUFFER_SIZE - 1) {
                buf[pos++] = c;
                terminal_putchar(c);
            }
        }
        buf[pos] = '\0';

        if (streq(buf, "back")) {
            return;
        }
        if (parse_two_ints(buf, &a, &b)) {
            terminal_write("sum = ");
            print_int(a + b);
            terminal_putchar('\n');
        } else {
            terminal_writeline("Invalid input. Use: <a> <b> or back");
        }
    }
}

static void gui_system_info(void) {
    terminal_writeline("");
    terminal_writeline("[System info]");
    terminal_writeline("Eleph OS v1.0");
    terminal_writeline("Mode: text UI pseudo-graphics");
    terminal_writeline("Keyboard: polling PS/2");
}

static void gui_terminal_app(void) {
    char buffer[CMD_BUFFER_SIZE];
    terminal_clear();
    gui_header();
    gui_draw_window("Terminal App");
    terminal_writeline("Type shell commands here. Type 'back' to desktop.");

    while (1) {
        int pos = 0;
        terminal_write("term$ ");
        while (1) {
            char c = keyboard_getchar();
            if (c == '\n') {
                terminal_putchar('\n');
                break;
            } else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    terminal_putchar('\b');
                }
            } else if (c >= 0x20 && c < 0x7F && pos < CMD_BUFFER_SIZE - 1) {
                buffer[pos++] = c;
                terminal_putchar(c);
            }
        }
        buffer[pos] = '\0';
        if (streq(buffer, "back")) {
            return;
        }
        execute_command(buffer);
    }
}

static void gui_files_app(void) {
    terminal_clear();
    gui_header();
    gui_draw_window("Files App");
    gui_list_files();
    terminal_writeline("");
    terminal_writeline("Press any key to return...");
    (void)keyboard_getchar();
}

static void gui_game_app(void) {
    char cmd[CMD_BUFFER_SIZE];
    terminal_clear();
    gui_header();
    gui_draw_window("Guess Game App");
    terminal_writeline("Type: start, guess <n>, quit, back");

    while (1) {
        int pos = 0;
        terminal_write("game> ");
        while (1) {
            char c = keyboard_getchar();
            if (c == '\n') {
                terminal_putchar('\n');
                break;
            } else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    terminal_putchar('\b');
                }
            } else if (c >= 0x20 && c < 0x7F && pos < CMD_BUFFER_SIZE - 1) {
                cmd[pos++] = c;
                terminal_putchar(c);
            }
        }
        cmd[pos] = '\0';

        if (streq(cmd, "back")) return;
        if (streq(cmd, "start")) {
            game_start();
        } else if (streq(cmd, "quit")) {
            cmd_game("quit");
        } else if (cmd[0] == 'g' && cmd[1] == 'u' && cmd[2] == 'e' && cmd[3] == 's' &&
                   cmd[4] == 's' && cmd[5] == ' ') {
            cmd_game(cmd);
        } else {
            terminal_writeline("Use: start | guess <n> | quit | back");
        }
    }
}

static void gui_run(void) {
    terminal_clear();
    terminal_writeline("Launching Eleph Desktop...");

    while (1) {
        char c;
        terminal_clear();
        gui_header();
        gui_draw_window("Applications");
        terminal_writeline(" 1) Terminal");
        terminal_writeline(" 2) Files");
        terminal_writeline(" 3) Calculator");
        terminal_writeline(" 4) Game");
        terminal_writeline(" 5) System info");
        terminal_writeline(" q) Shutdown GUI");
        terminal_writeline("");
        terminal_write("desktop> ");
        c = keyboard_getchar();
        terminal_putchar(c);
        terminal_putchar('\n');

        if (c == '1') {
            gui_terminal_app();
        } else if (c == '2') {
            gui_files_app();
        } else if (c == '3') {
            gui_calculator_app();
        } else if (c == '4') {
            gui_game_app();
        } else if (c == '5') {
            gui_system_info();
            terminal_writeline("");
            terminal_writeline("Press any key to return...");
            (void)keyboard_getchar();
        } else if (c == 'q' || c == 'Q' || c == 'x' || c == 'X') {
            terminal_writeline("Leaving GUI shell.");
            return;
        } else {
            terminal_writeline("Unknown choice.");
            terminal_writeline("Press any key to continue...");
            (void)keyboard_getchar();
        }
    }
}

static void cmd_gui(char* args) {
    (void)args;
    terminal_writeline("Launching Windows 9x GUI...");
    gfx_run_desktop();
    terminal_clear();
    terminal_writeline("GUI exited. Returning to shell...");
}

static void cmd_reboot(char* args) {
    (void)args;
    terminal_writeline("Rebooting...");
    outb(0x64, 0xFE);
    for (;;) {
        asm volatile("hlt");
    }
}

static void cmd_date(char* args) {
    (void)args;
    terminal_writeline("Date: 2026-04-21");
    terminal_writeline("Time: 14:30:00 (fake)");
}


static void cmd_mem(char* args) {
    (void)args;
    terminal_writeline("Memory Information:");
    terminal_write("  Total: ");
    print_int(64);
    terminal_writeline(" MB");
    terminal_write("  Used: ");
    print_int(1);
    terminal_writeline(" MB");
    terminal_write("  Free: ");
    print_int(63);
    terminal_writeline(" MB");
}

// ========== SNAKE GAME ==========
#define SNAKE_WIDTH 20
#define SNAKE_HEIGHT 15
#define SNAKE_MAX_LEN 50

static int snake_x[SNAKE_MAX_LEN];
static int snake_y[SNAKE_MAX_LEN];
static int snake_len;
static int snake_dir; // 0=up, 1=right, 2=down, 3=left
static int food_x, food_y;
static int snake_score;
static int snake_game_over;

static void snake_init(void) {
    snake_len = 3;
    snake_x[0] = SNAKE_WIDTH / 2;
    snake_y[0] = SNAKE_HEIGHT / 2;
    for (int i = 1; i < snake_len; i++) {
        snake_x[i] = snake_x[0] - i;
        snake_y[i] = snake_y[0];
    }
    snake_dir = 1; // right
    snake_score = 0;
    snake_game_over = 0;
    
    // Place food
    food_x = (prng_next() % (SNAKE_WIDTH - 2)) + 1;
    food_y = (prng_next() % (SNAKE_HEIGHT - 2)) + 1;
}

static void snake_place_food(void) {
    int valid;
    do {
        valid = 1;
        food_x = (prng_next() % (SNAKE_WIDTH - 2)) + 1;
        food_y = (prng_next() % (SNAKE_HEIGHT - 2)) + 1;
        // Check not on snake
        for (int i = 0; i < snake_len; i++) {
            if (snake_x[i] == food_x && snake_y[i] == food_y) {
                valid = 0;
                break;
            }
        }
    } while (!valid);
}

static void snake_draw(void) {
    terminal_clear();
    
    // Top border
    terminal_setcolor(make_color(COLOR_YELLOW, COLOR_BLACK));
    for (int x = 0; x <= SNAKE_WIDTH + 1; x++) {
        terminal_putchar('#');
    }
    terminal_putchar('\n');
    
    for (int y = 0; y < SNAKE_HEIGHT; y++) {
        terminal_setcolor(make_color(COLOR_YELLOW, COLOR_BLACK));
        terminal_putchar('#');
        
        for (int x = 0; x < SNAKE_WIDTH; x++) {
            int is_snake = 0;
            int is_food = (x == food_x && y == food_y);
            
            for (int i = 0; i < snake_len; i++) {
                if (snake_x[i] == x && snake_y[i] == y) {
                    is_snake = 1;
                    break;
                }
            }
            
            if (is_food) {
                terminal_setcolor(make_color(COLOR_RED, COLOR_BLACK));
                terminal_putchar('@');
            } else if (is_snake) {
                terminal_setcolor(make_color(COLOR_GREEN, COLOR_BLACK));
                terminal_putchar('o');
            } else {
                terminal_setcolor(make_color(COLOR_DARK_GREY, COLOR_BLACK));
                terminal_putchar('.');
            }
        }
        
        terminal_setcolor(make_color(COLOR_YELLOW, COLOR_BLACK));
        terminal_writeline("#");
    }
    
    // Bottom border
    terminal_setcolor(make_color(COLOR_YELLOW, COLOR_BLACK));
    for (int x = 0; x <= SNAKE_WIDTH + 1; x++) {
        terminal_putchar('#');
    }
    terminal_putchar('\n');
    
    terminal_setcolor(make_color(COLOR_LIGHT_CYAN, COLOR_BLACK));
    terminal_write("Score: ");
    print_int(snake_score);
    terminal_writeline("  |  Use WASD/Arrows  |  Q=Quit");
}

static void snake_update(void) {
    // Calculate new head position
    int new_x = snake_x[0];
    int new_y = snake_y[0];
    
    switch (snake_dir) {
        case 0: new_y--; break; // up
        case 1: new_x++; break; // right
        case 2: new_y++; break; // down
        case 3: new_x--; break; // left
    }
    
    // Check wall collision
    if (new_x < 0 || new_x >= SNAKE_WIDTH || new_y < 0 || new_y >= SNAKE_HEIGHT) {
        snake_game_over = 1;
        return;
    }
    
    // Check self collision
    for (int i = 0; i < snake_len; i++) {
        if (snake_x[i] == new_x && snake_y[i] == new_y) {
            snake_game_over = 1;
            return;
        }
    }
    
    // Check food
    int ate = (new_x == food_x && new_y == food_y);
    
    // Move snake
    if (ate) {
        if (snake_len < SNAKE_MAX_LEN) {
            // Shift and grow
            for (int i = snake_len; i > 0; i--) {
                snake_x[i] = snake_x[i-1];
                snake_y[i] = snake_y[i-1];
            }
            snake_len++;
            snake_score += 10;
        }
        snake_place_food();
    } else {
        // Shift without growing
        for (int i = snake_len - 1; i > 0; i--) {
            snake_x[i] = snake_x[i-1];
            snake_y[i] = snake_y[i-1];
        }
    }
    
    snake_x[0] = new_x;
    snake_y[0] = new_y;
}

static int snake_get_dir(char c) {
    switch (c) {
        case 'w': case 'W': case 72: return 0; // up
        case 'd': case 'D': case 75: return 1; // right
        case 's': case 'S': case 80: return 2; // down
        case 'a': case 'A': case 77: return 3; // left
        default: return -1;
    }
}

static void cmd_snake(char* args) {
    (void)args;
    terminal_writeline("=== SNAKE GAME ===");
    terminal_writeline("Use WASD or Arrow keys to move");
    terminal_writeline("Press Q to quit");
    terminal_writeline("Press any key to start...");
    (void)keyboard_getchar();
    
    snake_init();
    
    while (!snake_game_over) {
        snake_draw();
        
        // Get input with a small delay (polling)
        char c = keyboard_getchar();
        int new_dir = snake_get_dir(c);
        if (new_dir >= 0) {
            // Prevent 180-degree turns
            if ((new_dir == 0 && snake_dir != 2) ||
                (new_dir == 1 && snake_dir != 3) ||
                (new_dir == 2 && snake_dir != 0) ||
                (new_dir == 3 && snake_dir != 1)) {
                snake_dir = new_dir;
            }
        }
        if (c == 'q' || c == 'Q') {
            break;
        }
        
        snake_update();
    }
    
    terminal_clear();
    terminal_setcolor(make_color(COLOR_LIGHT_RED, COLOR_BLACK));
    terminal_writeline("=== GAME OVER ===");
    terminal_setcolor(make_color(COLOR_LIGHT_CYAN, COLOR_BLACK));
    terminal_write("Final Score: ");
    print_int(snake_score);
    terminal_putchar('\n');
    terminal_writeline("Press any key to exit...");
    (void)keyboard_getchar();
}

// ========== TIC-TAC-TOE GAME ==========
#define TTT_SIZE 3

static char ttt_board[TTT_SIZE][TTT_SIZE];
static int ttt_player; // 1 = X, 2 = O
static int ttt_game_over;
static int ttt_winner;

static void ttt_init(void) {
    for (int i = 0; i < TTT_SIZE; i++) {
        for (int j = 0; j < TTT_SIZE; j++) {
            ttt_board[i][j] = ' ';
        }
    }
    ttt_player = 1;
    ttt_game_over = 0;
    ttt_winner = 0;
}

static void ttt_draw(void) {
    terminal_clear();
    terminal_setcolor(make_color(COLOR_LIGHT_CYAN, COLOR_BLACK));
    terminal_writeline("=== TIC-TAC-TOE ===");
    terminal_writeline("");
    
    terminal_setcolor(make_color(COLOR_YELLOW, COLOR_BLACK));
    terminal_writeline("    1   2   3");
    terminal_writeline("  +---+---+---+");
    
    for (int i = 0; i < TTT_SIZE; i++) {
        terminal_write(" ");
        print_int(i + 1);
        terminal_write(" |");
        for (int j = 0; j < TTT_SIZE; j++) {
            if (ttt_board[i][j] == 'X') {
                terminal_setcolor(make_color(COLOR_LIGHT_GREEN, COLOR_BLACK));
            } else if (ttt_board[i][j] == 'O') {
                terminal_setcolor(make_color(COLOR_LIGHT_RED, COLOR_BLACK));
            } else {
                terminal_setcolor(make_color(COLOR_DARK_GREY, COLOR_BLACK));
            }
            terminal_write(" ");
            terminal_putchar(ttt_board[i][j]);
            terminal_write(" |");
        }
        terminal_putchar('\n');
        terminal_setcolor(make_color(COLOR_YELLOW, COLOR_BLACK));
        terminal_writeline("  +---+---+---+");
    }
    
    terminal_putchar('\n');
    terminal_setcolor(make_color(COLOR_LIGHT_GREY, COLOR_BLACK));
    
    if (!ttt_game_over) {
        terminal_write("Player ");
        terminal_putchar(ttt_player == 1 ? 'X' : 'O');
        terminal_writeline("'s turn");
        terminal_writeline("Enter: row col (e.g., 1 2) or Q to quit");
    } else if (ttt_winner == 0) {
        terminal_writeline("It's a DRAW!");
        terminal_writeline("Press any key to exit...");
    } else {
        terminal_write("Player ");
        terminal_putchar(ttt_winner == 1 ? 'X' : 'O');
        terminal_writeline(" WINS!");
        terminal_writeline("Press any key to exit...");
    }
}

static int ttt_check_win(void) {
    // Check rows
    for (int i = 0; i < TTT_SIZE; i++) {
        if (ttt_board[i][0] != ' ' &&
            ttt_board[i][0] == ttt_board[i][1] &&
            ttt_board[i][1] == ttt_board[i][2]) {
            return ttt_board[i][0] == 'X' ? 1 : 2;
        }
    }
    
    // Check columns
    for (int j = 0; j < TTT_SIZE; j++) {
        if (ttt_board[0][j] != ' ' &&
            ttt_board[0][j] == ttt_board[1][j] &&
            ttt_board[1][j] == ttt_board[2][j]) {
            return ttt_board[0][j] == 'X' ? 1 : 2;
        }
    }
    
    // Check diagonals
    if (ttt_board[0][0] != ' ' &&
        ttt_board[0][0] == ttt_board[1][1] &&
        ttt_board[1][1] == ttt_board[2][2]) {
        return ttt_board[0][0] == 'X' ? 1 : 2;
    }
    
    if (ttt_board[0][2] != ' ' &&
        ttt_board[0][2] == ttt_board[1][1] &&
        ttt_board[1][1] == ttt_board[2][0]) {
        return ttt_board[0][2] == 'X' ? 1 : 2;
    }
    
    // Check draw
    int draw = 1;
    for (int i = 0; i < TTT_SIZE; i++) {
        for (int j = 0; j < TTT_SIZE; j++) {
            if (ttt_board[i][j] == ' ') {
                draw = 0;
                break;
            }
        }
        if (!draw) break;
    }
    
    if (draw) return -1; // draw
    
    return 0; // continue
}

static void cmd_tictactoe(char* args) {
    (void)args;
    terminal_writeline("=== TIC-TAC-TOE ===");
    terminal_writeline("Two players: X and O");
    terminal_writeline("Enter row and column (1-3)");
    terminal_writeline("Press any key to start...");
    (void)keyboard_getchar();
    
    ttt_init();
    
    while (!ttt_game_over) {
        ttt_draw();
        
        if (ttt_game_over) break;
        
        // Get input
        char buffer[CMD_BUFFER_SIZE];
        int pos = 0;
        
        while (1) {
            char c = keyboard_getchar();
            if (c == '\n') {
                terminal_putchar('\n');
                break;
            } else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    terminal_putchar('\b');
                }
            } else if (c == 'q' || c == 'Q') {
                terminal_writeline("");
                return;
            } else if (c >= 0x20 && c < 0x7F && pos < CMD_BUFFER_SIZE - 1) {
                buffer[pos++] = c;
                terminal_putchar(c);
            }
        }
        buffer[pos] = '\0';
        
        // Parse input
        int row, col;
        if (!parse_two_ints(buffer, &row, &col)) {
            terminal_writeline("Invalid input! Use: row col");
            continue;
        }
        
        if (row < 1 || row > 3 || col < 1 || col > 3) {
            terminal_writeline("Row and col must be 1-3!");
            continue;
        }
        
        row--; col--;
        
        if (ttt_board[row][col] != ' ') {
            terminal_writeline("Cell already taken!");
            continue;
        }
        
        // Make move
        ttt_board[row][col] = ttt_player == 1 ? 'X' : 'O';
        
        // Check win
        int result = ttt_check_win();
        if (result == -1) {
            ttt_game_over = 1;
            ttt_winner = 0;
        } else if (result != 0) {
            ttt_game_over = 1;
            ttt_winner = result;
        } else {
            // Switch player
            ttt_player = ttt_player == 1 ? 2 : 1;
        }
    }
    
    ttt_draw();
    (void)keyboard_getchar();
}

static void execute_command(char* cmdline) {
    while (*cmdline == ' ') cmdline++;

    if (*cmdline == '\0') return;

    char* args = cmdline;
    while (*args != ' ' && *args != '\0') args++;

    if (*args == ' ') {
        *args = '\0';
        args++;
        while (*args == ' ') args++;
    } else {
        args = cmdline;
        while (*args) args++;
    }

    for (int i = 0; commands[i].name != NULL; i++) {
        if (streq(commands[i].name, cmdline)) {
            commands[i].func(args);
            return;
        }
    }

    terminal_write("Unknown command: ");
    terminal_writeline(cmdline);
}

void shell_run(void) {
    char buffer[CMD_BUFFER_SIZE];
    terminal_clear();
    terminal_writeline("Eleph OS Terminal v1.1");
    terminal_writeline("Type 'help' for commands");
    terminal_writeline("");

    while (1) {
        terminal_write("> ");
        int pos = 0;

        while (1) {
            char c = keyboard_getchar();

            if (c == '\n') {
                terminal_putchar('\n');
                break;
            } else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    terminal_putchar('\b');
                }
            } else if (c >= 0x20 && c < 0x7F && pos < CMD_BUFFER_SIZE - 1) {
                buffer[pos++] = c;
                terminal_putchar(c);
            }
        }

        buffer[pos] = '\0';
        execute_command(buffer);
    }
}