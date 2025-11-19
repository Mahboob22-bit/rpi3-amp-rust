# Raspberry Pi Dateien

Dateien die auf dem Raspberry Pi benötigt werden.

## Struktur
```
raspberry-pi/
├── core-loader/           # User-space Core 3 Loader
│   ├── core3_loader_v2.c
│   ├── Makefile
│   └── README.md
└── boot-config/           # Boot-Konfigurations-Beispiele
    ├── cmdline.txt.example
    └── config.txt.example
```

## Setup auf dem Raspberry Pi

### 1. Repository klonen
```bash
mkdir -p ~/dev
cd ~/dev
git clone https://github.com/Mahboob22-bit/rpi3-amp-rust.git
cd rpi3-amp-rust
```

### 2. Boot-Konfiguration anpassen
```bash
# cmdline.txt bearbeiten
sudo nano /boot/firmware/cmdline.txt
# Füge hinzu: maxcpus=3 isolcpus=3

# config.txt prüfen
grep enable_uart /boot/firmware/config.txt
# Sollte "enable_uart=1" enthalten

# Reboot
sudo reboot
```

### 3. Core Loader kompilieren
```bash
cd raspberry-pi/core-loader
make
```

### 4. Bare-Metal Binary übertragen

Von deinem Entwicklungs-PC (WSL):
```bash
# Im rust-baremetal/rpi3-core3 Verzeichnis
scp target/aarch64-unknown-none/release/kernel_core3.img \
    admin@rpi3-amp:~/dev/rpi3-amp-rust/raspberry-pi/core-loader/
```

### 5. Core 3 starten
```bash
cd ~/dev/rpi3-amp-rust/raspberry-pi/core-loader
sudo ./core3_loader_v2 kernel_core3.img
```

Die ACT-LED sollte blinken! 🎉
