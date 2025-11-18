# Raspberry Pi 3 AMP: Linux + Rust Bare-Metal

Dieses Projekt demonstriert Asymmetric Multiprocessing (AMP) auf dem Raspberry Pi 3 Model B, wobei 3 CPU-Cores Linux ausführen und ein Core eine Bare-Metal Rust-Applikation läuft.

## 🎯 Projektziel

- **Core 0-2**: Raspberry Pi OS (Linux)
- **Core 3**: Rust Bare-Metal App (LED Blinker)

## 📋 Hardware-Anforderungen

- Raspberry Pi 3 Model B (V1.2)
- SD-Karte (min. 8GB)
- USB-UART Adapter (für Debugging)
- Netzteil

## 🛠️ Software-Anforderungen

### Entwicklungs-PC (Windows/WSL oder Linux)
- Rust toolchain (stable)
- `aarch64-unknown-none` target
- `cargo-binutils`

### Raspberry Pi
- Raspberry Pi OS Lite (64-bit)
- Build tools (gcc, make)

## 🚀 Quick Start

Siehe detaillierte Anleitung in:
- [docs/01-setup.md](docs/01-setup.md) - Initial Setup
- [docs/02-rust-baremetal.md](docs/02-rust-baremetal.md) - Rust App bauen
- [docs/03-linux-kernel.md](docs/03-linux-kernel.md) - Linux konfigurieren
- [docs/04-running.md](docs/04-running.md) - Alles starten

## 📊 Status

- [x] Linux auf 3 Cores limitieren
- [x] Rust Bare-Metal App (LED Blinker)
- [x] Core 3 Loader (User-Space)
- [x] Core 3 erfolgreich gestartet
- [ ] UART Output von Core 3
- [ ] Shared Memory Kommunikation
- [ ] Performance-Messungen

## 📝 Blog Post

Dieser Code ist Teil eines Blog-Posts über AMP auf Raspberry Pi.

## 📄 Lizenz

Apache-2.0 license - siehe LICENSE Datei


## 👤 Autor

Mahboob - Embedded Software Developer
