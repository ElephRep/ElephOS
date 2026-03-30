; boot/boot.asm
; Multiboot2 header
section .multiboot2
align 8
    dd 0xE85250D6                 
    dd 0                          
    dd header_end - header_start  
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start))

header_start:
    dw 0    ; тип
    dw 0    ; флаги
    dd 8    ; размер
header_end:

section .text
global start
extern kernel_main
extern long_mode_start

start:
    mov esp, stack_top
    
    push ebx
    push eax
    
    call long_mode_start
    
    cli
    hang:
        hlt
        jmp hang

section .bss
align 16
stack_bottom:
    resb 16384      
stack_top: