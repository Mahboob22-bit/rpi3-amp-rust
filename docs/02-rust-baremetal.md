# Rust Bare-Metal App erstellen

## Übersicht

Die Bare-Metal Rust-App läuft direkt auf Core 3 ohne Betriebssystem. Sie greift direkt auf die Hardware zu und lässt die ACT-LED (GPIO 47) blinken.

## Projektstruktur

Das Rust-Projekt befindet sich in [`rust-baremetal/rpi3-core3/`](../rust-baremetal/rpi3-core3/).

### Wichtige Dateien

#### 1. `Cargo.toml`
[Siehe Datei](../rust-baremetal/rpi3-core3/Cargo.toml)

**Wichtige Konfigurationen:**
```toml
[profile.release]
panic = "abort"        # Kein Unwinding - reduziert Binary-Größe
lto = true            # Link-Time Optimization
opt-level = "z"       # Optimierung für kleinste Binary-Größe
```

Diese Settings sind kritisch für Bare-Metal: Kein Panic-Handling-Code, maximale Optimierung.

#### 2. `.cargo/config.toml`
[Siehe Datei](../rust-baremetal/rpi3-core3/.cargo/config.toml)
```toml
[build]
target = "aarch64-unknown-none"    # Bare-Metal ARM64 target

[target.aarch64-unknown-none]
rustflags = ["-C", "link-arg=-Tlink.ld"]  # Verwendet unser Linker-Script
```

Das `aarch64-unknown-none` Target bedeutet:
- `aarch64`: ARM 64-bit Architektur
- `unknown`: Kein spezifischer Vendor
- `none`: **Kein Betriebssystem** (Bare-Metal)

#### 3. `link.ld` - Linker Script
[Siehe Datei](../rust-baremetal/rpi3-core3/link.ld)

**Wichtige Bereiche:**
```ld
ENTRY(_start)           # Entry-Point unserer App

SECTIONS
{
    . = 0x80000;       # ← KRITISCH: Startadresse!
                       # Core 3 wird bei 0x80000 im RAM starten
    
    .text : {
        KEEP(*(.text.boot))    # _start() muss als erstes kommen
        *(.text*)              # Rest des Codes
    }
    
    .rodata : { *(.rodata*) }  # Read-only Daten
    .data : { *(.data*) }      # Initialisierte Daten
    
    .bss : {
        __bss_start = .;
        *(.bss*)               # Uninitialisierte Daten
        __bss_end = .;
    }
}
```

**Warum 0x80000?**
- Das ist die Standard-Kernel-Load-Adresse für ARM64
- Linux lädt normalerweise dort
- Wir nutzen dieselbe Adresse für Core 3

#### 4. `src/main.rs` - Die Bare-Metal App
[Siehe Datei](../rust-baremetal/rpi3-core3/src/main.rs)

**Code-Analyse:**
```rust
#![no_std]              // Keine Standard-Library
#![no_main]             // Kein normaler main() Entry-Point
```

**GPIO-Adressen für Raspberry Pi 3:**
```rust
const GPIO_BASE: usize = 0x3F20_0000;  // BCM2837 Peripherals
const GPFSEL4: *mut u32 = (GPIO_BASE + 0x10) as *mut u32;  // Function Select
const GPSET1: *mut u32 = (GPIO_BASE + 0x20) as *mut u32;   // Set (LED an)
const GPCLR1: *mut u32 = (GPIO_BASE + 0x2C) as *mut u32;   // Clear (LED aus)
```

**Entry-Point:**
```rust
#[no_mangle]                    // Name nicht mangeln - wichtig für Linker
#[link_section = ".text.boot"]  // In .text.boot Section (kommt zuerst!)
pub extern "C" fn _start() -> ! {
    // GPIO 47 als Output konfigurieren
    unsafe {
        let mut val = GPFSEL4.read_volatile();
        val &= !(0b111 << 21);  // Clear bits 21-23
        val |= 0b001 << 21;     // Set als Output (001)
        GPFSEL4.write_volatile(val);
    }
    
    // Endlosschleife: LED blinken
    loop {
        unsafe { GPSET1.write_volatile(1 << (LED_GPIO - 32)); }
        delay(500000);
        unsafe { GPCLR1.write_volatile(1 << (LED_GPIO - 32)); }
        delay(500000);
    }
}
```

**Warum `LED_GPIO - 32`?**
- GPIO 47 ist in Register GPSET1/GPCLR1
- Diese Register steuern GPIO 32-63
- Bit-Position = GPIO - 32 = 47 - 32 = 15

## Kompilieren
```bash
cd rust-baremetal/rpi3-core3
cargo build --release
```

Das erzeugt:
1. ELF-Binary: `target/aarch64-unknown-none/release/rpi3-core3` (65KB)
2. Wird zu Raw-Binary konvertiert: `kernel_core3.img` (nur 84 Bytes!)

## Binary konvertieren
```bash
rust-objcopy --strip-all -O binary \
    target/aarch64-unknown-none/release/rpi3-core3 \
    target/aarch64-unknown-none/release/kernel_core3.img
```

Diese `.img` Datei wird auf den Raspberry Pi übertragen.

## Auf den Pi übertragen
```bash
scp target/aarch64-unknown-none/release/kernel_core3.img \
    admin@rpi3-amp:~/rpi3-amp-project/
```

## Build & Deploy Scripts

Für einfacheres Arbeiten gibt es zwei Helper-Scripts:

### build.sh
[Siehe Script](../rust-baremetal/rpi3-core3/build.sh)

Kompiliert das Projekt und konvertiert die Binary:
```bash
cd rust-baremetal/rpi3-core3
./build.sh
```

Das Script:
1. Führt `cargo build --release` aus
2. Konvertiert ELF zu Raw-Binary mit `rust-objcopy`
3. Zeigt die Binary-Größe an

### deploy.sh
[Siehe Script](../rust-baremetal/rpi3-core3/deploy.sh)

Überträgt die Binary zum Raspberry Pi:
```bash
./deploy.sh
```

**Umgebungsvariablen:**
- `RPI_HOST`: SSH-Ziel (default: `admin@rpi3-amp`)

Beispiel mit anderem Host:
```bash
RPI_HOST=pi@192.168.1.100 ./deploy.sh
```

### Kompletter Workflow
```bash
# 1. Bauen
./build.sh

# 2. Deployen
./deploy.sh

# 3. Auf dem Pi ausführen (via SSH)
ssh admin@rpi3-amp
cd ~/rpi3-amp-project/rpi3-amp-rust/raspberry-pi/core-loader
sudo ./core3_loader_v2 kernel_core3.img
```


## Nächster Schritt

➡️ [03-linux-kernel.md](03-linux-kernel.md) - Linux auf 3 Cores limitieren
