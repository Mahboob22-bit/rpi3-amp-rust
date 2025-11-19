#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

// BCM2837 Local Peripherals (ARM-side)
#define ARM_LOCAL_BASE       0x40000000
#define CORE3_MAILBOX3_SET   (ARM_LOCAL_BASE + 0x8C)
#define CORE3_MAILBOX3_CLR   (ARM_LOCAL_BASE + 0xCC)

// Startadresse für Core 3
#define CORE3_ENTRY          0x00080000

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
    
    // 4. Mailbox für Core 3 wakeup
    printf("Mapping ARM local peripherals at 0x%x...\n", ARM_LOCAL_BASE);
    void* mailbox_page = mmap(NULL, 4096,
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED,
                             mem_fd,
                             ARM_LOCAL_BASE);
    
    if (mailbox_page == MAP_FAILED) {
        perror("mmap failed for mailbox");
        close(mem_fd);
        free(kernel_data);
        return 1;
    }
    
    volatile uint32_t* mailbox3 = (volatile uint32_t*)((uint8_t*)mailbox_page + 0x8C);
    
    printf("Sending wakeup to Core 3 with entry point 0x%x...\n", CORE3_ENTRY);
    
    // Startadresse in Mailbox schreiben
    *mailbox3 = CORE3_ENTRY;
    __sync_synchronize();
    
    printf("\n=== Core 3 wakeup signal sent! ===\n");
    printf("Watch the ACT LED (GPIO 47) - it should blink!\n");
    printf("\nPress Ctrl+C to exit (Core 3 keeps running)\n");
    
    // Cleanup
    munmap(mailbox_page, 4096);
    close(mem_fd);
    free(kernel_data);
    
    // Warten
    while(1) {
        sleep(1);
    }
    
    return 0;
}
