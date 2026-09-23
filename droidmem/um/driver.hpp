#ifndef __DRIVER_HPP__
#define __DRIVER_HPP__

/*
 * driver.hpp — Driver unificado para leitura/escrita de memória
 *
 * Tenta /dev/droidmem (kernel module) primeiro.
 * Se não disponível, usa /proc/pid/mem (fallback automático).
 *
 * Créditos:
 *   - rogxo: Autor original da base kernel_hack com driver .ko
 *   - ScxttVX: Adição do fallback /proc/pid/mem, ptrace, auto-detect
 */

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define DEVICE_NAME "/dev/droidmem"

class c_driver {
private:
    int fd;
    pid_t pid;
    bool use_proc_mem;
    bool ptrace_attached;

public:
    c_driver() : fd(-1), pid(0), use_proc_mem(false), ptrace_attached(false) {
        fd = open(DEVICE_NAME, O_RDWR);
        if (fd == -1) {
            printf("[*] " DEVICE_NAME " não disponível, usando /proc/pid/mem\n");
            use_proc_mem = true;
        } else {
            printf("[+] Driver kernel aberto: fd=%d\n", fd);
        }
    }

    ~c_driver() {
        if (ptrace_attached && pid > 0)
            ptrace(PTRACE_DETACH, pid, NULL, NULL);
        if (fd >= 0)
            close(fd);
    }

    bool is_open() const { return fd >= 0 || use_proc_mem; }

    void initialize(pid_t target_pid) {
        this->pid = target_pid;

        if (use_proc_mem) {
            if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == 0) {
                waitpid(pid, NULL, 0);
                ptrace_attached = true;
                printf("[+] ptrace conectado ao PID %d\n", pid);
            } else {
                printf("[!] ptrace falhou (errno=%d), tentando acesso direto\n", errno);
            }
        }
    }

    bool init_key(const char *key) {
        if (use_proc_mem) return true;

        char buf[0x100] = {0};
        strncpy(buf, key, sizeof(buf) - 1);
        if (ioctl(fd, 0x800, buf) != 0) {
            printf("[-] init_key falhou\n");
            return false;
        }
        printf("[+] init_key OK\n");
        return true;
    }

    bool read_mem(uintptr_t addr, void *buffer, size_t size) {
        if (use_proc_mem)
            return proc_read(addr, buffer, size);

        typedef struct { pid_t pid; uintptr_t addr; void *buffer; size_t size; } CM;
        CM cm = { pid, addr, buffer, size };
        return ioctl(fd, 0x801, &cm) == 0;
    }

    bool write_mem(uintptr_t addr, const void *buffer, size_t size) {
        if (use_proc_mem)
            return proc_write(addr, buffer, size);

        typedef struct { pid_t pid; uintptr_t addr; void *buffer; size_t size; } CM;
        CM cm = { pid, addr, (void *)buffer, size };
        return ioctl(fd, 0x802, &cm) == 0;
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
        if (use_proc_mem)
            return proc_find_module(name);

        typedef struct { pid_t pid; char *name; uintptr_t base; } MB;
        MB mb;
        char buf[0x100] = {0};
        strncpy(buf, name, sizeof(buf) - 1);
        mb.pid = pid;
        mb.name = buf;
        mb.base = 0;
        if (ioctl(fd, 0x803, &mb) != 0) return 0;
        return mb.base;
    }

private:
    bool proc_read(uintptr_t addr, void *buffer, size_t size) {
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/mem", pid);
        int fd2 = ::open(path, O_RDONLY);
        if (fd2 < 0) return false;

        if (::lseek(fd2, (off_t)addr, SEEK_SET) == (off_t)-1) {
            ::close(fd2);
            return false;
        }
        ssize_t n = ::read(fd2, buffer, size);
        ::close(fd2);
        return n == (ssize_t)size;
    }

    bool proc_write(uintptr_t addr, const void *buffer, size_t size) {
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/mem", pid);
        int fd2 = ::open(path, O_WRONLY);
        if (fd2 < 0) return false;

        if (::lseek(fd2, (off_t)addr, SEEK_SET) == (off_t)-1) {
            ::close(fd2);
            return false;
        }
        ssize_t n = ::write(fd2, buffer, size);
        ::close(fd2);
        return n == (ssize_t)size;
    }

    uintptr_t proc_find_module(const char *name) {
        char path[64];
        char line[512];
        uintptr_t base = 0;

        snprintf(path, sizeof(path), "/proc/%d/maps", pid);
        FILE *fp = fopen(path, "r");
        if (!fp) return 0;

        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, name)) {
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
};

#endif /* __DRIVER_HPP__ */
