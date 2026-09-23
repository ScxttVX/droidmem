#ifndef __PROC_MEM_HPP__
#define __PROC_MEM_HPP__

/*
 * proc_mem.hpp — Leitor/Escritor de memória via /proc/pid/mem
 *
 * Não precisa de módulo kernel. Funciona em qualquer Android com root.
 * Usa /proc/pid/mem (disponível desde Linux 3.2) para acesso direto
 * à memória do processo, e /proc/pid/maps para busca de base de módulos.
 *
 * Créditos:
 *   - rogxo: Autor original da base kernel_hack
 *   - ScxttVX: Implementação standalone /proc/pid/mem
 */

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/uio.h>

class c_proc_mem {
private:
    pid_t pid;
    bool attached;

    uintptr_t find_module_base(const char *module_name) {
        char path[64];
        char line[512];
        FILE *fp;
        uintptr_t base = 0;

        snprintf(path, sizeof(path), "/proc/%d/maps", pid);
        fp = fopen(path, "r");
        if (!fp) {
            printf("[-] Cannot open %s: %s\n", path, strerror(errno));
            return 0;
        }

        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name)) {
                uintptr_t start = 0;
                if (sscanf(line, "%lx-", &start) == 1) {
                    base = start;
                    break;
                }
            }
        }

        fclose(fp);
        return base;
    }

public:
    c_proc_mem() : pid(0), attached(false) {}
    ~c_proc_mem() { detach(); }

    bool attach(pid_t target_pid) {
        this->pid = target_pid;

        if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == 0) {
            waitpid(pid, NULL, 0);
            attached = true;
            printf("[+] ptrace attached to PID %d\n", pid);
            return true;
        }

        printf("[!] ptrace attach failed (errno=%d): %s\n", errno, strerror(errno));
        printf("[*] Trying /proc/pid/mem directly (may need root)...\n");

        attached = false;
        return true;
    }

    void detach() {
        if (attached && pid > 0) {
            ptrace(PTRACE_DETACH, pid, NULL, NULL);
            attached = false;
        }
    }

    bool read_mem(uintptr_t addr, void *buffer, size_t size) {
        char path[64];
        int fd;
        ssize_t bytes_read;

        snprintf(path, sizeof(path), "/proc/%d/mem", pid);
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            printf("[-] open /proc/%d/mem failed: %s\n", pid, strerror(errno));
            return false;
        }

        if (lseek(fd, (off_t)addr, SEEK_SET) == (off_t)-1) {
            printf("[-] lseek to 0x%lx failed: %s\n", (unsigned long)addr, strerror(errno));
            close(fd);
            return false;
        }

        bytes_read = ::read(fd, buffer, size);
        close(fd);

        if (bytes_read != (ssize_t)size) {
            printf("[-] read at 0x%lx: got %zd bytes, expected %zu\n",
                   (unsigned long)addr, bytes_read, size);
            return false;
        }

        return true;
    }

    bool write_mem(uintptr_t addr, const void *buffer, size_t size) {
        char path[64];
        int fd;
        ssize_t bytes_written;

        snprintf(path, sizeof(path), "/proc/%d/mem", pid);
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            printf("[-] open /proc/%d/mem for write failed: %s\n", pid, strerror(errno));
            return false;
        }

        if (lseek(fd, (off_t)addr, SEEK_SET) == (off_t)-1) {
            printf("[-] lseek to 0x%lx failed: %s\n", (unsigned long)addr, strerror(errno));
            close(fd);
            return false;
        }

        bytes_written = ::write(fd, buffer, size);
        close(fd);

        if (bytes_written != (ssize_t)size) {
            printf("[-] write at 0x%lx: wrote %zd bytes, expected %zu\n",
                   (unsigned long)addr, bytes_written, size);
            return false;
        }

        return true;
    }

    template <typename T>
    T read(uintptr_t addr) {
        T res{};
        read_mem(addr, &res, sizeof(T));
        return res;
    }

    template <typename T>
    bool write(uintptr_t addr, T value) {
        return write_mem(addr, &value, sizeof(T));
    }

    uintptr_t get_module_base(const char *name) {
        return find_module_base(name);
    }
};

#endif /* __PROC_MEM_HPP__ */
