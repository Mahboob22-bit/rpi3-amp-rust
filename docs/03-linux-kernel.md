# Linux Kernel für AMP konfigurieren

## Ziel

Linux soll nur auf Core 0-2 laufen und Core 3 frei lassen.

## Boot-Konfiguration anpassen

### 1. cmdline.txt bearbeiten

Auf dem **Raspberry Pi**:
```bash
sudo nano /boot/firmware/cmdline.txt
```

**Am Ende der Zeile hinzufügen** (alles in EINER Zeile!):
```
maxcpus=3 isolcpus=3 mem=768M
```

**Beispiel-Datei:** [raspberry-pi/boot-config/cmdline.txt.example](../raspberry-pi/boot-config/cmdline.txt.example)

**Was bedeuten die Parameter?**

- `maxcpus=3`: Linux startet nur mit 3 CPUs (Core 0, 1, 2)
- `isolcpus=3`: Core 3 wird vom Scheduler isoliert (falls er doch online wäre)
- `mem=768M`: Limitiert Linux RAM auf 768MB (obere 256MB frei für Core 3)

### 2. Vollständige cmdline.txt
```bash
console=serial0,115200 console=tty1 root=PARTUUID=190c3914-02 rootfstype=ext4 fsck.repair=yes rootwait cfg80211.ieee80211_regdom=CH maxcpus=3 isolcpus=3 mem=768M
```

**Wichtig:** `mem=768M` verhindert Memory-Konflikte!
- RPi3 hat 1GB RAM (0x00000000 - 0x3FFFFFFF)
- Linux nutzt nur 0x00000000 - 0x2FFFFFFF (768MB)
- Core 3 bekommt 0x30000000 - 0x3EFFFFFF (240MB frei)
- Ab 0x3F000000 beginnen die Peripherals (GPIO, UART, etc.)

### 3. config.txt (bereits konfiguriert)

**Beispiel-Datei:** [raspberry-pi/boot-config/config.txt.example](../raspberry-pi/boot-config/config.txt.example)

Wichtig ist, dass UART aktiviert ist:
```ini
enable_uart=1
```

## Reboot und Verifizieren
```bash
sudo reboot
```

Nach dem Reboot:
```bash
# Sollte "3" anzeigen
nproc

# Sollte "0-2" anzeigen
cat /sys/devices/system/cpu/online

# Core 3 existiert, ist aber offline
ls -la /sys/devices/system/cpu/cpu3
```

**Erwartete Ausgabe:**
```
nproc: 3
online: 0-2
cpu3: (Verzeichnis existiert)
```

✅ **Erfolg!** Linux läuft jetzt nur auf 3 Cores. Core 3 ist reserviert für unsere Bare-Metal App.

## Nächster Schritt

➡️ [04-running.md](04-running.md) - Core 3 mit Bare-Metal Code starten
