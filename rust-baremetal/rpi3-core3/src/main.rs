#![no_std]
#![no_main]
#![feature(naked_functions)]

use core::panic::PanicInfo;
use core::arch::asm;

// GPIO Base Address für Raspberry Pi 3
const GPIO_BASE: usize = 0x3F20_0000;
const GPFSEL4: *mut u32 = (GPIO_BASE + 0x10) as *mut u32;
const GPSET1: *mut u32 = (GPIO_BASE + 0x20) as *mut u32;
const GPCLR1: *mut u32 = (GPIO_BASE + 0x2C) as *mut u32;

// LED am GPIO 47 (ACT LED auf Pi 3)
const LED_GPIO: u32 = 47;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

// ARM64 Boot Stub - MUSS als erste Funktion kommen!
#[no_mangle]
#[naked]
#[link_section = ".text.boot"]
pub unsafe extern "C" fn _start() -> ! {
    asm!(
        // Disable interrupts
        "msr daifset, #0xF",

        // Set stack pointer to end of our memory region + 64KB
        "ldr x0, =0x38010000",
        "mov sp, x0",

        // Clear BSS section
        "ldr x0, =__bss_start",
        "ldr x1, =__bss_end",
        "1:",
        "cmp x0, x1",
        "b.ge 2f",
        "str xzr, [x0], #8",
        "b 1b",
        "2:",

        // Branch to Rust main
        "bl rust_main",

        // If main returns, loop forever
        "3: b 3b",
        options(noreturn)
    )
}

#[no_mangle]
pub extern "C" fn rust_main() -> ! {
    // GPIO 47 als Output konfigurieren
    unsafe {
        // GPFSEL4 steuert GPIO 40-49
        // GPIO 47 = Bit 21-23 in GPFSEL4 (3 bits pro Pin)
        let mut val = GPFSEL4.read_volatile();
        val &= !(0b111 << 21);  // Clear
        val |= 0b001 << 21;     // Set als Output
        GPFSEL4.write_volatile(val);
    }

    // LED blinken lassen
    loop {
        // LED an
        unsafe {
            GPSET1.write_volatile(1 << (LED_GPIO - 32));
        }
        delay(500000);

        // LED aus
        unsafe {
            GPCLR1.write_volatile(1 << (LED_GPIO - 32));
        }
        delay(500000);
    }
}

fn delay(count: u32) {
    for _ in 0..count {
        unsafe {
            core::arch::asm!("nop");
        }
    }
}
