# Setup Guide

## 1. Entwicklungs-PC Setup (WSL/Linux)

### Rust installieren
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup target add aarch64-unknown-none
rustup component add rust-src llvm-tools-preview
cargo install cargo-binutils
```

### Cross-Compiler
```bash
sudo apt update
sudo apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

## 2. Raspberry Pi Setup

### OS flashen
1. Raspberry Pi Imager herunterladen
2. OS wählen: **Raspberry Pi OS Lite (64-bit)**
3. Einstellungen (⚙️):
   - SSH aktivieren
   - Hostname: `rpi3-amp`
   - WLAN konfigurieren (optional)
   - Benutzer anlegen

### Boot-Konfiguration anpassen

**Nach dem Flashen**, `/boot/firmware/config.txt` bearbeiten:
```ini
# Am Ende hinzufügen:
enable_uart=1
```

### Erste Schritte auf dem Pi
```bash
# SSH verbinden
ssh admin@rpi3-amp.local

# System updaten
sudo apt update && sudo apt upgrade -y

# Build-Tools installieren
sudo apt install -y git build-essential
```
