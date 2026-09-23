// SPDX-License-Identifier: GPL-2.0
/*
 * memory.c — Leitura/Escrita de memória de processos via page table walking
 *
 * Créditos:
 *   - rogxo: Autor original da base com page table walking
 *   - ScxttVX: Correções de bugs, multi-página, multi-kernel
 *
 * Correções aplicadas:
 *  - Use-after-free em read_process_memory / write_process_memory
 *  - Suporte a multi-página com cross-boundary
 *  - Compatibilidade com Kernel 3.x / 4.x / 5.x / 6.x
 *  - Contagem de referências correta (put_task_struct, put_pid, mmput)
 */

#include "memory.h"
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/highmem.h>

#include <asm/page.h>
#include <asm/pgtable.h>

extern struct mm_struct *get_task_mm(struct task_struct *task);

/* ──────────────────────────────────────────────
 * Compatibilidade de versões do kernel
 * ────────────────────────────────────────────── */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
#define PTE_OFFSET_MAP  1
#else
#define PTE_OFFSET_MAP  0
#endif

#ifndef ioremap_cache
#define ioremap_cache   ioremap
#endif

/* ──────────────────────────────────────────────
 * translate_linear_address — percorrer page tables
 * ────────────────────────────────────────────── */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61)
phys_addr_t translate_linear_address(struct mm_struct *mm, uintptr_t va)
{
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    phys_addr_t page_addr;
    uintptr_t page_offset;

    pgd = pgd_offset(mm, va);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        return 0;

    p4d = p4d_offset(pgd, va);
    if (p4d_none(*p4d) || p4d_bad(*p4d))
        return 0;

    pud = pud_offset(p4d, va);
    if (pud_none(*pud) || pud_bad(*pud))
        return 0;

    pmd = pmd_offset(pud, va);
    if (pmd_none(*pmd))
        return 0;

#if PTE_OFFSET_MAP
    pte = pte_offset_map(pmd, va);
    if (!pte)
        return 0;
    if (pte_none(*pte) || !pte_present(*pte)) {
        pte_unmap(pte);
        return 0;
    }
    page_addr = (phys_addr_t)(pte_pfn(*pte) << PAGE_SHIFT);
    page_offset = va & (PAGE_SIZE - 1);
    pte_unmap(pte);
    return page_addr + page_offset;
#else
    pte = pte_offset_kernel(pmd, va);
    if (pte_none(*pte) || !pte_present(*pte))
        return 0;
    page_addr = (phys_addr_t)(pte_pfn(*pte) << PAGE_SHIFT);
    page_offset = va & (PAGE_SIZE - 1);
    return page_addr + page_offset;
#endif
}
#else
phys_addr_t translate_linear_address(struct mm_struct *mm, uintptr_t va)
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    phys_addr_t page_addr;
    uintptr_t page_offset;

    pgd = pgd_offset(mm, va);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        return 0;

    pud = pud_offset(pgd, va);
    if (pud_none(*pud) || pud_bad(*pud))
        return 0;

    pmd = pmd_offset(pud, va);
    if (pmd_none(*pmd))
        return 0;

    pte = pte_offset_kernel(pmd, va);
    if (pte_none(*pte) || !pte_present(*pte))
        return 0;

    page_addr = (phys_addr_t)(pte_pfn(*pte) << PAGE_SHIFT);
    page_offset = va & (PAGE_SIZE - 1);
    return page_addr + page_offset;
}
#endif

#ifndef ARCH_HAS_VALID_PHYS_ADDR_RANGE
static inline int valid_phys_addr_range(phys_addr_t addr, size_t count)
{
    return addr + count <= __pa(high_memory);
}
#endif

static inline unsigned long size_inside_page(unsigned long start, unsigned long size)
{
    unsigned long sz;
    sz = PAGE_SIZE - (start & (PAGE_SIZE - 1));
    return min(sz, size);
}

bool read_physical_address(phys_addr_t pa, void __user *buffer, size_t size)
{
    void *mapped;
    unsigned long sz;
    char __user *buf = buffer;
    size_t remaining;

    if (!pfn_valid(__phys_to_pfn(pa)))
        return false;

    if (!valid_phys_addr_range(pa, size))
        return false;

    while (size > 0) {
        sz = size_inside_page(pa, size);

        mapped = ioremap_cache(pa, sz);
        if (!mapped)
            return false;

        remaining = copy_to_user(buf, mapped, sz);
        iounmap(mapped);

        if (remaining)
            return false;

        buf += sz;
        pa += sz;
        size -= sz;
    }

    return true;
}

bool write_physical_address(phys_addr_t pa, void __user *buffer, size_t size)
{
    void *mapped;
    unsigned long sz;
    char __user *buf = buffer;
    size_t remaining;

    if (!pfn_valid(__phys_to_pfn(pa)))
        return false;

    if (!valid_phys_addr_range(pa, size))
        return false;

    while (size > 0) {
        sz = size_inside_page(pa, size);

        mapped = ioremap_cache(pa, sz);
        if (!mapped)
            return false;

        remaining = copy_from_user(mapped, buf, sz);
        iounmap(mapped);

        if (remaining)
            return false;

        buf += sz;
        pa += sz;
        size -= sz;
    }

    return true;
}

bool read_process_memory(pid_t pid, uintptr_t addr,
                         void __user *buffer, size_t size)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct pid *pid_struct;
    phys_addr_t pa;
    bool ret = false;

    pid_struct = find_get_pid(pid);
    if (!pid_struct)
        return false;

    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task)
        goto put_pid;

    mm = get_task_mm(task);
    put_task_struct(task);
    if (!mm)
        goto put_pid;

    pa = translate_linear_address(mm, addr);
    if (pa)
        ret = read_physical_address(pa, buffer, size);

    mmput(mm);

put_pid:
    put_pid(pid_struct);
    return ret;
}

bool write_process_memory(pid_t pid, uintptr_t addr,
                          void __user *buffer, size_t size)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct pid *pid_struct;
    phys_addr_t pa;
    bool ret = false;

    pid_struct = find_get_pid(pid);
    if (!pid_struct)
        return false;

    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task)
        goto put_pid;

    mm = get_task_mm(task);
    put_task_struct(task);
    if (!mm)
        goto put_pid;

    pa = translate_linear_address(mm, addr);
    if (pa)
        ret = write_physical_address(pa, buffer, size);

    mmput(mm);

put_pid:
    put_pid(pid_struct);
    return ret;
}
