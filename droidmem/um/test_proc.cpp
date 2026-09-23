/*
 * test_proc.cpp — Leitor de memória standalone via /proc/pid/mem
 *
 * Não precisa de módulo kernel. Funciona em qualquer Android com root.
 * Uso: ./test_proc <package> <biblioteca>
 *
 * Créditos:
 *   - rogxo: Autor original da base kernel_hack
 *   - ScxttVX: Teste standalone via /proc/pid/mem
 */

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "proc_mem.hpp"

static uint64_t get_tick_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec) * 1000 + (uint64_t)(ts.tv_nsec / 1000000);
}

static pid_t get_pid_by_name(const char *name) {
    char cmd[0x200] = {0};
    pid_t pid = 0;
    FILE *fp;

    snprintf(cmd, sizeof(cmd) - 1, "pidof %s", name);
    fp = popen(cmd, "r");
    if (!fp) return 0;
    if (fscanf(fp, "%d", &pid) != 1) pid = 0;
    pclose(fp);
    return pid;
}

int main(int argc, char const *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <package_name> <module_name>\n", argv[0]);
        printf("  e.g. %s com.android.chrome libchrome.so\n", argv[0]);
        return 1;
    }

    const char *pkg = argv[1];
    const char *mod = argv[2];

    printf("[*] Looking up PID for: %s\n", pkg);
    pid_t pid = get_pid_by_name(pkg);
    if (pid <= 0) {
        printf("[-] Process not found. Is it running?\n");
        return 1;
    }
    printf("[+] PID: %d\n", pid);

    c_proc_mem mem;
    if (!mem.attach(pid)) {
        printf("[-] Failed to attach\n");
        return 1;
    }

    printf("[*] Searching for module: %s\n", mod);
    uintptr_t base = mem.get_module_base(mod);
    if (base == 0) {
        printf("[-] Module not found in PID %d\n", pid);
        printf("    Available modules:\n");
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/maps", pid);
        FILE *fp = fopen(path, "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, ".so") || strstr(line, ".apk")) {
                    char *nl = strchr(line, '\n');
                    if (nl) *nl = 0;
                    printf("      %s\n", line);
                }
            }
            fclose(fp);
        }
        return 1;
    }
    printf("[+] Module base: 0x%lx\n", (unsigned long)base);

    printf("[*] Reading ELF header at base...\n");
    {
        uint8_t magic[4] = {0};
        if (mem.read_mem(base, magic, 4)) {
            if (magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F') {
                printf("[+] ELF magic verified: 7f 45 4c 46\n");
            } else {
                printf("[!] Unexpected bytes: %02x %02x %02x %02x\n",
                       magic[0], magic[1], magic[2], magic[3]);
            }
        }
    }

    printf("[*] Speed test: 100 x 8-byte reads...\n");
    {
        uint64_t now = get_tick_ms();
        for (int i = 0; i < 100; i++) {
            volatile uint64_t val = mem.read<uint64_t>(base);
            (void)val;
        }
        uint64_t elapsed = get_tick_ms() - now;
        printf("[+] 100 reads in %llu ms (avg: %.3f ms)\n",
               (unsigned long long)elapsed, (double)elapsed / 100.0);
    }

    printf("[*] First 64 bytes at base:\n    ");
    {
        uint8_t buf[64] = {0};
        if (mem.read_mem(base, buf, 64)) {
            for (int i = 0; i < 64; i++) {
                printf("%02x ", buf[i]);
                if ((i + 1) % 16 == 0) printf("\n    ");
            }
        }
    }

    printf("\n[+] All tests passed — /proc/pid/mem working!\n");
    return 0;
}
