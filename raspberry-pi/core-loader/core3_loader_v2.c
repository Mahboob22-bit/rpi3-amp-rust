#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

// Memory Layout für RPi3 mit 1GB RAM (NO mem= parameter):
// 0x00000000 - 0x00000FFF : Spin tables (ARM stub)
// 0x00001000 - 0x3D5FFFFF : System RAM visible to Linux (~998MB with gpu_mem=16)
// 0x3D600000 - 0x3EFFFFFF : GPU memory (16MB with gpu_mem=16)
// 0x3F000000 - 0x3FFFFFFF : Peripherals
//
// Strategy: Use high address within System RAM but beyond Linux usage
// Linux uses ~731MB, so anything above ~0x2E000000 is relatively safe
// We use 0x38000000 (896MB) - high enough to avoid Linux, low enough to be in RAM map

// ARM64 Spin Table Addresses (from armstub8.S)
#define SPIN_TABLE_BASE      0x000000D8
#define CORE0_SPIN_ADDR      (SPIN_TABLE_BASE + 0x00)  // 0xD8
#define CORE1_SPIN_ADDR      (SPIN_TABLE_BASE + 0x08)  // 0xE0
#define CORE2_SPIN_ADDR      (SPIN_TABLE_BASE + 0x10)  // 0xE8
#define CORE3_SPIN_ADDR      (SPIN_TABLE_BASE + 0x18)  // 0xF0

// Startadresse für Core 3 - 896MB (within System RAM map)
// WARNING: Might conflict with Linux if it allocates high memory!
#define CORE3_ENTRY          0x38000000

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <kernel_core3.img>\n", argv[0]);
        return 1;
    }
    
    printf("=== Raspberry Pi 3 Core 3 Bare-Metal Loader v2 ===\n");
    
    // 1. Binary einlesen
    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        perror("Cannot open kernel image");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    if (fsize < 0) {
        perror("Cannot determine file size");
        fclose(f);
        return 1;
    }
    size_t size = (size_t)fsize;
    fseek(f, 0, SEEK_SET);
    
    printf("Kernel image size: %zu bytes\n", size);
    
    uint8_t* kernel_data = malloc(size);
    if (!kernel_data) {
        perror("Cannot allocate memory");
        fclose(f);
        return 1;
    }
    
    if (fread(kernel_data, 1, size, f) != size) {
        perror("Cannot read kernel image");
        free(kernel_data);
        fclose(f);
        return 1;
    }
    fclose(f);
    
    // Debug: Erste Bytes anzeigen
    printf("First instruction: %02x %02x %02x %02x\n",
           kernel_data[0], kernel_data[1], kernel_data[2], kernel_data[3]);
    
    // 2. /dev/mem öffnen
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("Cannot open /dev/mem - need root!");
        free(kernel_data);
        return 1;
    }
    
    // 3. Kernel-Code in Memory kopieren (0x80000)
    printf("Mapping physical memory at 0x%x...\n", CORE3_ENTRY);
    void* target_mem = mmap(NULL, 4096, 
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED, 
                           mem_fd, 
                           CORE3_ENTRY);
    
    if (target_mem == MAP_FAILED) {
        perror("mmap failed for target memory");
        close(mem_fd);
        free(kernel_data);
        return 1;
    }
    
    printf("Copying kernel to 0x%x...\n", CORE3_ENTRY);
    memcpy(target_mem, kernel_data, size);
    
    // Cache synchronisieren
    __sync_synchronize();
    msync(target_mem, 4096, MS_SYNC);
    
    // Verify
    printf("Verifying copy...\n");
    if (memcmp(target_mem, kernel_data, size) == 0) {
        printf("✓ Kernel successfully loaded at 0x%x\n", CORE3_ENTRY);
    } else {
        printf("✗ Verification failed!\n");
    }
    
    munmap(target_mem, 4096);

    // 4. Spin-Table für Core 3 wakeup (ARM64 Standard-Methode)
    printf("Mapping spin table at 0x%lx...\n", CORE3_SPIN_ADDR);
    void* spin_page = mmap(NULL, 4096,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          mem_fd,
                          0x0);  // Map page 0 (contains spin tables)

    if (spin_page == MAP_FAILED) {
        perror("mmap failed for spin table");
        close(mem_fd);
        free(kernel_data);
        return 1;
    }

    // Core 3 spin table entry (64-bit entry point address)
    volatile uint64_t* core3_spin = (volatile uint64_t*)((uint8_t*)spin_page + CORE3_SPIN_ADDR);

    printf("Current spin table value at 0x%lx: 0x%lx\n", CORE3_SPIN_ADDR, *core3_spin);
    printf("Writing entry point 0x%x to Core 3 spin table...\n", CORE3_ENTRY);

    // Wichtig: Data cache flush BEVOR wir die spin table schreiben
    __sync_synchronize();

    // Startadresse in Spin-Table schreiben (64-bit!)
    *core3_spin = (uint64_t)CORE3_ENTRY;

    // Cache coherency sicherstellen
    __sync_synchronize();
    msync(spin_page, 4096, MS_SYNC);

    // SEV (Send Event) würde hier Core 3 aus WFE wecken, aber das geht aus Userspace nicht
    // Core 3 muss die spin table pollen (was armstub8 normalerweise macht)

    printf("\n=== Core 3 wakeup signal sent via spin table! ===\n");
    printf("Spin table entry at 0x%lx = 0x%lx\n", CORE3_SPIN_ADDR, *core3_spin);
    printf("Watch the ACT LED (GPIO 47) - it should blink!\n");
    printf("\nPress Ctrl+C to exit (Core 3 keeps running)\n");

    // Cleanup
    munmap(spin_page, 4096);
    close(mem_fd);
    free(kernel_data);
    
    // Warten
    while(1) {
        sleep(1);
    }
    
    return 0;
}
