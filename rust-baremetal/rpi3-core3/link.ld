ENTRY(_start)

SECTIONS
{
    /* Core 3 startet bei 0x38000000 (896MB - beyond Linux, within ARM RAM with gpu_mem=16) */
    . = 0x38000000;
    
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
