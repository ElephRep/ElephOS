/* linker.ld */
ENTRY(start)

SECTIONS
{
    . = 1M;
    
    .multiboot2 : {
        *(.multiboot2)
    }
    
    .text : {
        *(.text)
    }
    
    .data : {
        *(.data)
        *(.rodata)
    }
    
    .bss : {
        *(COMMON)
        *(.bss)
    }
}