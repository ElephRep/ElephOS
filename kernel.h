// include/kernel.h
#ifndef KERNEL_H
#define KERNEL_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

void kernel_main(uint32_t magic, void* multiboot_info);
void vga_print(const char* str);

#endif