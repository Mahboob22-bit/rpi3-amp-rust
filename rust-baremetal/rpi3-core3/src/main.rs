#![no_std]
#![no_main]

use core::panic::PanicInfo;

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

#[no_mangle]
#[link_section = ".text.boot"]
pub extern "C" fn _start() -> ! {
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
