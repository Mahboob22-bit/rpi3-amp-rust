# Core 3 Loader

User-space Programm zum Laden und Starten der Bare-Metal App auf Core 3.

## Kompilieren
```bash
make
```

## Ausführen
```bash
# Binary muss im selben Verzeichnis liegen
sudo ./core3_loader_v2 kernel_core3.img
```

## Voraussetzungen

- Root-Rechte (für /dev/mem Zugriff)
- Core 3 muss offline sein (siehe Boot-Config)
- `kernel_core3.img` muss vorhanden sein

## Funktionsweise

1. Lädt `kernel_core3.img` in den RAM bei 0x80000
2. Sendet Wakeup-Signal an Core 3 via Mailbox
3. Core 3 startet und führt den Code aus
