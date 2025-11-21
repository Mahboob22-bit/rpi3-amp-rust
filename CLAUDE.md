# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **Raspberry Pi 3 Model B Asymmetric Multiprocessing (AMP)** demonstration project that runs Linux on cores 0-2 and a bare-metal Rust application on core 3. The bare-metal app blinks the ACT LED (GPIO 47) to prove successful execution outside the Linux scheduler.

**Key Achievement**: Successfully running user-defined bare-metal code on a dedicated CPU core alongside a full Linux system, demonstrating real-time capable computing on commodity hardware.

## Project Status

**Current State**: Core 3 successfully loads bare-metal code but wakeup mechanism needs verification

**Completed**:
- Linux isolated to cores 0-2 via boot parameters
- Rust bare-metal LED blinker with proper ARM64 boot stub
- User-space core loader using ARM spin tables
- Memory layout resolved at 0x38000000 (896MB)

**Known Issues**:
- Core 3 may not be starting (needs `maxcpus=4` or SEV signal testing)
- UART output from Core 3 not implemented
- No shared memory communication yet

## Architecture & Memory Layout

### CPU Core Assignment
- **Cores 0-2**: Raspberry Pi OS (64-bit Linux)
- **Core 3**: Bare-metal Rust application (isolated)

### Memory Map (RPi3, 1GB RAM, gpu_mem=16)
```
0x00000000 - 0x00000FFF : ARM spin tables
  0xF0                  : Core 3 spin table address ← Loader writes here
0x00001000 - 0x3D5FFFFF : System RAM (~998MB)
  0x38000000            : *** Core 3 entry point ***
0x3D600000 - 0x3EFFFFFF : GPU memory (16MB)
0x3F000000 - 0x3FFFFFFF : Peripherals (GPIO, UART)
```

### Critical Address: 0x38000000

**Why this specific address?**
1. Above Linux usage (~731MB active)
2. Within System RAM map (no `mem=` restriction needed)
3. Below GPU memory and peripherals
4. Accessible via /dev/mem without bus errors

**Evolution**: Started at 0x80000 (conflicted with Linux kernel), moved through 0x3B000000 (GPU boundary), settled on 0x38000000.

## Key Components

### 1. Rust Bare-Metal App (`rust-baremetal/rpi3-core3/`)

**src/main.rs**:
- `_start()`: Naked assembly boot stub
  - Sets stack pointer to 0x38010000
  - Clears BSS
  - Disables interrupts
  - Branches to `rust_main()`
- GPIO 47 (ACT LED) blinker via BCM2837 registers at 0x3F200000

**link.ld**:
```ld
. = 0x38000000;  /* MUST match loader address */
```

**Important**: Uses `naked_asm!` (Rust 1.88+) and `#[unsafe(naked)]` syntax.

### 2. Core Loader (`raspberry-pi/core-loader/core3_loader_v2.c`)

User-space program that:
1. Reads `kernel_core3.img`
2. Copies to 0x38000000 via `/dev/mem`
3. Writes entry point to spin table at 0xF0
4. Core 3 reads spin table and jumps to 0x38000000

### 3. Boot Configuration

**`/boot/firmware/cmdline.txt`**:
```
maxcpus=3 isolcpus=3
```
- **NO `mem=` parameter** - would break /dev/mem access to high RAM

**`/boot/firmware/config.txt`**:
```ini
enable_uart=1
gpu_mem=16    # Maximize ARM RAM to 998MB
```

## Build & Deployment

### On Development PC:

```bash
# Build Rust bare-metal app
cd rust-baremetal/rpi3-core3
./build.sh
# Output: kernel_core3.img (84 bytes)

# Deploy to RPi
./deploy.sh
```

### On Raspberry Pi:

```bash
# Compile loader (one-time)
cd raspberry-pi/core-loader
make

# Run
sudo ./core3_loader_v2 kernel_core3.img
# Watch for blinking ACT LED
```

## Boot Sequence

1. **Firmware**: Initializes Core 3 in spin loop polling address 0xF0
2. **Linux**: Boots on cores 0-2 (due to `maxcpus=3`)
3. **Core 3**: Remains in firmware spin loop
4. **Loader**: Writes 0x38000000 to address 0xF0
5. **Core 3**: Reads non-zero value, jumps to 0x38000000
6. **Bare-metal code**: Executes `_start()` → `rust_main()` → LED blinks

## Troubleshooting

### LED doesn't blink

**Check boot parameters**:
```bash
cat /proc/cmdline  # Should have: maxcpus=3 isolcpus=3
nproc              # Should show: 3
```

**Verify loader output**:
```
✓ Kernel successfully loaded at 0x38000000
Spin table entry at 0xf0 = 0x38000000
```

**Test with `maxcpus=4`**:
If Core 3 needs to be online first, edit cmdline.txt:
```
maxcpus=4 isolcpus=3
```
Then: `sudo reboot`

### Common Issues

**Bus Error at 0x38000000**:
- Check `/proc/iomem | grep "System RAM"` includes this address
- Ensure NO `mem=` parameter in cmdline.txt
- Verify `gpu_mem=16` in config.txt

**Core 3 doesn't exist**:
```bash
ls /sys/devices/system/cpu/cpu3  # Should exist with maxcpus=4
```

## Memory Address Changes

**If changing 0x38000000, update**:
1. `link.ld`: `. = 0x38000000;`
2. `core3_loader_v2.c`: `#define CORE3_ENTRY`
3. `main.rs`: Stack pointer in `_start()` (address + 64KB)

## Important Gotchas

1. **No MMU on Core 3**: All addresses are physical
2. **Cache coherency**: Loader uses `__sync_synchronize()` + `msync()`
3. **No std library**: Can't use println!, Vec, String on Core 3
4. **Stack setup critical**: `_start()` must set SP before calling Rust
5. **Naked functions**: Use `naked_asm!` (not `asm!`) in `#[unsafe(naked)]` functions

## Useful Commands

```bash
# On Raspberry Pi

# Verify core configuration
nproc
cat /sys/devices/system/cpu/online
cat /proc/cmdline

# Check memory layout
sudo cat /proc/iomem | grep -E "System RAM|memory"
free -h

# GPU memory split
vcgencmd get_mem arm
vcgencmd get_mem gpu

# Kernel messages
dmesg | tail -30

# On Development PC

# Inspect binary
hexdump -C target/aarch64-unknown-none/release/kernel_core3.img
rust-objdump -d target/aarch64-unknown-none/release/rpi3-core3

# Check size
ls -lh target/aarch64-unknown-none/release/kernel_core3.img  # Should be 84 bytes
```

## Extending the Project

### Add UART output:
- Configure mini UART (AUX_MU) at 0x3F215000
- Write bytes to AUX_MU_IO_REG (0x3F215040)
- Read via USB-UART adapter on GPIO 14/15

### Shared memory:
- Reserve region (e.g., 0x38010000 - 0x38020000)
- Use volatile reads/writes with barriers
- Linux accesses via /dev/mem mmap

### Performance testing:
- Use ARM generic timer (CNTPCT_EL0)
- Measure interrupt latency vs Linux
- Compare jitter under CPU load

## Resources

- [BCM2837 Peripherals](https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf) - GPIO registers
- [ARM Cortex-A53 Manual](https://developer.arm.com/documentation/ddi0500/) - CPU architecture
- [Rust Embedonomicon](https://docs.rust-embedded.org/embedonomicon/) - Bare-metal Rust guide
- [RPi Boot Sequence](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#raspberry-pi-boot-sequence)

---

**Last Updated**: 2025-01-21
**License**: Apache-2.0
