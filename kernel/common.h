// common.h - Общие утилиты и определения
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

// Сравнение строк
static inline int streq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

// Ввод-вывод через порты
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Объявление функции генератора случайных чисел (реализована в shell.c)
uint32_t prng_next(void);

#endif
