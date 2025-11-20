ENTRY(_start)

SECTIONS
{
    /* Core 3 startet bei 0x3B000000 (high memory before GPU/Peripherals) */
    . = 0x3B000000;
    
    .text : {
        KEEP(*(.text.boot))
        *(.text*)
    }
    
    .rodata : {
        *(.rodata*)
    }
    
    .data : {
        *(.data*)
    }
    
    .bss : {
        __bss_start = .;
        *(.bss*)
        __bss_end = .;
    }
    
    /DISCARD/ : {
        *(.comment)
        *(.gnu*)
        *(.note*)
        *(.eh_frame*)
    }
}
