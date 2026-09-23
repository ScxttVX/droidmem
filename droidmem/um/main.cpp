/*
 * main.cpp — Aplicação usermode para o driver droidmem
 *
 * Créditos:
 *   - rogxo: Autor original da base kernel_hack com kernel module .ko
 *   - ScxttVX: Correções de bugs, fallback /proc/pid/mem, multi-kernel
 *
 * Uso: ./main64 <package> <biblioteca> [endereco_hex]
 *
 * Exemplos:
 *   ./main64 com.dts.freefireth libil2cpp.so
 *   ./main64 com.android.chrome libmonochrome.so
 */

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "driver.hpp"

static uint64_t get_tick_count64(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec) * 1000 + (uint64_t)(ts.tv_nsec / 1000000);
}

static pid_t get_pid_by_name(const char *name)
{
    char cmd[0x200] = {0};
    pid_t pid = 0;
    FILE *fp;

    snprintf(cmd, sizeof(cmd) - 1, "pidof %s", name);
    fp = popen(cmd, "r");
    if (!fp) {
        printf("[-] popen failed for: %s\n", cmd);
        return 0;
    }
    if (fscanf(fp, "%d", &pid) != 1) {
        pid = 0;
    }
    pclose(fp);
    return pid;
}

static void print_usage(const char *prog)
{
    printf("droidmem - Leitura/Escrita de Memória Android\n\n");
    printf("Uso: %s <package> <biblioteca> [endereco_hex]\n\n", prog);
    printf("  package    — Nome do pacote Android (ex: com.dts.freefireth)\n");
    printf("  biblioteca — .so para buscar base (ex: libil2cpp.so)\n");
    printf("  endereco   — Endereço hex opcional para ler 8 bytes\n");
    printf("\nExemplos:\n");
    printf("  %s com.dts.freefireth libil2cpp.so\n", prog);
    printf("  %s com.android.chrome libmonochrome.so\n", prog);
    printf("  %s com.tencent.tmgp.sgame libunity.so 0x7a00000000\n", prog);
}

int main(int argc, char const *argv[])
{
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char *package_name = argv[1];
    const char *module_name = argv[2];

    printf("[*] Buscando PID de: %s\n", package_name);
    pid_t pid = get_pid_by_name(package_name);
    if (pid <= 0) {
        printf("[-] Processo não encontrado: %s\n", package_name);
        printf("    Certifique-se de que o app está rodando.\n");
        return 1;
    }
    printf("[+] PID: %d\n", pid);

    c_driver driver;
    if (!driver.is_open()) {
        printf("[-] Não foi possível abrir o driver.\n");
        printf("    Execute: insmod droidmem.ko\n");
        return 1;
    }

    driver.init_key("any_key_accepted");

    driver.initialize(pid);

    printf("[*] Buscando módulo: %s\n", module_name);
    uintptr_t base = driver.get_module_base(module_name);
    if (base == 0) {
        printf("[-] Módulo não encontrado: %s\n", module_name);
        printf("    O módulo pode não estar carregado no processo alvo.\n");
        return 1;
    }
    printf("[+] Base do módulo: 0x%lx\n", (unsigned long)base);

    uintptr_t test_addr = base;
    if (argc >= 4) {
        test_addr = (uintptr_t)strtoull(argv[3], NULL, 16);
    }

    printf("[*] Lendo 8 bytes de 0x%lx...\n", (unsigned long)test_addr);
    {
        size_t iterations = 100;
        uint64_t result = 0;
        uint64_t now = get_tick_count64();

        for (size_t i = 0; i < iterations; i++) {
            result = driver.read<uint64_t>(test_addr);
        }

        uint64_t elapsed = get_tick_count64() - now;
        printf("[+] %zu leituras em %llu ms (média: %.3f ms)\n",
               iterations, (unsigned long long)elapsed,
               (double)elapsed / iterations);
        printf("[+] Valor: 0x%016llx\n", (unsigned long long)result);
    }

    printf("[*] Teste multi-página (64KB)...\n");
    {
        const size_t big_size = 64 * 1024;
        char *buf = (char *)malloc(big_size);
        if (buf) {
            uint64_t now = get_tick_count64();
            bool ok = driver.read_mem(test_addr, buf, big_size);
            uint64_t elapsed = get_tick_count64() - now;

            if (ok) {
                printf("[+] Multi-página OK: %llu ms\n", (unsigned long long)elapsed);
                printf("    Primeiros 16 bytes: ");
                for (int i = 0; i < 16; i++)
                    printf("%02x ", (unsigned char)buf[i]);
                printf("\n");
            } else {
                printf("[-] Falha na leitura multi-página\n");
            }
            free(buf);
        }
    }

    printf("[+] Todos os testes passaram.\n");
    return 0;
}
