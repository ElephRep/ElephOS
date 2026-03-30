; boot/long_mode.asm
section .text
global long_mode_start
extern kernel_main

long_mode_start:
    mov eax, cr4
    or eax, 1 << 5      
    mov cr4, eax
    
    mov eax, p4_table
    mov cr3, eax
    
    mov ecx, 0xC0000080  ; EFER MSR
    rdmsr
    or eax, 1 << 8       
    wrmsr

    mov eax, cr0
    or eax, 1 << 31      
    mov cr0, eax
    
    lgdt [gdt64.pointer]
    jmp gdt64.code:init_64

[BITS 64]
init_64:
    mov ax, gdt64.data
    mov ss, ax
    mov ds, ax
    mov es, ax
    

    mov rdi, rax        ; magic
    mov rsi, rbx        ; multiboot_info
    call kernel_main
    

    cli
    hang:
        hlt
        jmp hang


section .data
gdt64:
    dq 0        
.code: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.data: equ $ - gdt64
    dq (1 << 44) | (1 << 47) | (1 << 53) ; data segment
.pointer:
    dw $ - gdt64 - 1
    dq gdt64


section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096