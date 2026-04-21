[bits 32]
%ifdef ENTRY_UNDERSCORE
    extern _kernel_main
    call _kernel_main
%else
    extern kernel_main
    call kernel_main
%endif

; После завершения kernel_main просто останавливаем процессор
; так как shell может вызывать GUI, который не возвращает управление
.halt:
    hlt
    jmp .halt
