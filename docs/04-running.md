# Core 3 mit Bare-Metal Code starten

## Übersicht

Der **Core Loader** ist ein User-Space Programm, das:
1. Die Rust Bare-Metal Binary in den RAM lädt (0x80000)
2. Core 3 über die ARM Mailbox aufweckt
3. Core 3 startet bei 0x80000 und führt unseren Code aus

## Core Loader kompilieren

Auf dem **Raspberry Pi**:
```bash
cd ~/rpi3-amp-project/raspberry-pi/core-loader
make
```

Oder manuell:
```bash
gcc -O2 -Wall -o core3_loader_v2 core3_loader_v2.c
```

## Code-Erklärung

Die Datei [`core3_loader_v2.c`](../raspberry-pi/core-loader/core3_loader_v2.c) macht folgendes:

### 1. Memory-Mapping (/dev/mem)
```c
#define CORE3_ENTRY 0x00080000  // Startadresse für Core 3

// /dev/mem öffnen (braucht root!)
int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);

// 0x80000 im physischen RAM mappen
void* target_mem = mmap(NULL, 4096, 
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, 
                       mem_fd, 
                       CORE3_ENTRY);
```

**Wichtig:** `/dev/mem` erlaubt direkten Zugriff auf physischen Speicher - daher brauchen wir `sudo`!

### 2. Binary in RAM kopieren
```c
// kernel_core3.img einlesen
uint8_t* kernel_data = malloc(size);
fread(kernel_data, 1, size, f);

// In den RAM bei 0x80000 kopieren
memcpy(target_mem, kernel_data, size);

// Cache synchronisieren (KRITISCH!)
__sync_synchronize();
msync(target_mem, 4096, MS_SYNC);
```

### 3. Core 3 aufwecken
```c
// ARM Local Peripherals
#define ARM_LOCAL_BASE       0x40000000
#define CORE3_MAILBOX3_SET   (ARM_LOCAL_BASE + 0x8C)

// Mailbox mappen
void* mailbox_page = mmap(NULL, 4096,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         mem_fd,
                         ARM_LOCAL_BASE);

volatile uint32_t* mailbox3 = 
    (volatile uint32_t*)((uint8_t*)mailbox_page + 0x8C);

// Startadresse in Mailbox schreiben → Core 3 wacht auf!
*mailbox3 = CORE3_ENTRY;
__sync_synchronize();
```

**So funktioniert der Wakeup:**
- Raspberry Pi Firmware hält Core 1-3 initial im WFE (Wait-For-Event)
- Mailbox-Register triggern ein Event
- Core 3 liest seine Mailbox (0x8C) → findet 0x80000
- Core 3 springt zu 0x80000 → unser Rust-Code läuft!

## Ausführen
```bash
cd ~/rpi3-amp-project

# Als root ausführen (braucht /dev/mem Zugriff)
sudo raspberry-pi/core-loader/core3_loader_v2 kernel_core3.img
```

**Erwartete Ausgabe:**
```
=== Raspberry Pi 3 Core 3 Bare-Metal Loader v2 ===
Kernel image size: 84 bytes
First instruction: fe 0f 1e f8
Mapping physical memory at 0x80000...
Copying kernel to 0x80000...
Verifying copy...
✓ Kernel successfully loaded at 0x80000
Mapping ARM local peripherals at 0x40000000...
Sending wakeup to Core 3 with entry point 0x80000...

=== Core 3 wakeup signal sent! ===
Watch the ACT LED (GPIO 47) - it should blink!

Press Ctrl+C to exit (Core 3 keeps running)
```

## Erfolg verifizieren

🔴 **Die ACT-LED sollte jetzt blinken!** 🔴

Die grüne LED auf dem Raspberry Pi Board blinkt im 1-Sekunden-Takt:
- 0.5s an
- 0.5s aus

Das bedeutet: **Core 3 führt erfolgreich unseren Rust Bare-Metal Code aus!**

## System-Status prüfen
```bash
# Welche Cores sind online?
cat /sys/devices/system/cpu/online
# Erwartet: 0-2

# CPU-Last anzeigen
htop
# Core 0-2: Linux Prozesse
# Core 3: 0% (läuft außerhalb von Linux!)
```

## Troubleshooting

### LED blinkt nicht

1. **GPIO-Adresse prüfen:**
   - Pi 3: `0x3F20_0000`
   - Pi 4: `0xFE20_0000` (unterschiedlich!)

2. **Binary-Größe prüfen:**
```bash
   ls -lh kernel_core3.img
   # Sollte 84 bytes sein
```

3. **Memory-Mapping fehlgeschlagen:**
   - `sudo` verwenden!
   - Kernel-Parameter korrekt in cmdline.txt?

4. **dmesg checken:**
```bash
   sudo dmesg | tail
```

### Core 3 startet nicht

Prüfen ob Core 3 wirklich offline ist:
```bash
cat /sys/devices/system/cpu/cpu3/online
# Sollte nicht existieren oder "0" sein
```

## Nächste Schritte

Das Basis-System läuft! Mögliche Erweiterungen:

- [ ] UART-Output von Core 3 für Debugging
- [ ] Shared Memory für Linux ↔ Core 3 Kommunikation
- [ ] Performance-Messungen (Jitter, Latency)
- [ ] GPIO-Input verarbeiten auf Core 3

## Ressourcen

- [BCM2837 ARM Peripherals](https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf)
- [ARM Cortex-A53 Manual](https://developer.arm.com/documentation/ddi0500/latest/)
