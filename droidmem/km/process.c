// SPDX-License-Identifier: GPL-2.0
/*
 * process.c — Busca de endereço base do módulo via iteração VMA
 *
 * Créditos:
 *   - rogxo: Autor original da base kernel_hack
 *   - ScxttVX: Correções de bugs, memory leaks, compatibilidade Kernel 6.x
 *
 * Correções aplicadas:
 *  - Use-after-free: mmput() era chamado antes da iteração mm->mmap
 *  - Memory leaks: put_task_struct() e put_pid() nunca eram chamados
 *  - Compatibilidade Kernel 6.x: vm_next substituído por for_each_vma()
 */

#include "process.h"
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/fs.h>

#define ARC_PATH_MAX 256

extern struct mm_struct *get_task_mm(struct task_struct *task);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61)
extern void mmput(struct mm_struct *);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
#define USE_VMA_ITER 1
#include <linux/mm_types.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
#include <linux/maple_tree.h>
#endif
#else
#define USE_VMA_ITER 0
#endif

uintptr_t get_module_base(pid_t pid, char __user *uname)
{
    struct pid *pid_struct;
    struct task_struct *task;
    struct mm_struct *mm;
    uintptr_t base = 0;
    char name[ARC_PATH_MAX];

    if (!uname)
        return 0;

    if (strncpy_from_user(name, uname, ARC_PATH_MAX - 1) < 0)
        return 0;
    name[ARC_PATH_MAX - 1] = '\0';

    pid_struct = find_get_pid(pid);
    if (!pid_struct)
        return 0;

    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task)
        goto put_pid;

    mm = get_task_mm(task);
    put_task_struct(task);
    if (!mm)
        goto put_pid;

#if USE_VMA_ITER
    {
        VMA_ITERATOR(vmi, mm, 0);
        struct vm_area_struct *vma;

        for_each_vma(vmi, vma) {
            if (vma->vm_file) {
                char buf[ARC_PATH_MAX];
                char *path_nm;

                path_nm = file_path(vma->vm_file, buf, ARC_PATH_MAX - 1);
                if (!IS_ERR(path_nm)) {
                    if (!strcmp(kbasename(path_nm), name)) {
                        base = vma->vm_start;
                        break;
                    }
                }
            }
        }
    }
#else
    {
        struct vm_area_struct *vma;

        for (vma = mm->mmap; vma; vma = vma->vm_next) {
            if (vma->vm_file) {
                char buf[ARC_PATH_MAX];
                char *path_nm;

                path_nm = file_path(vma->vm_file, buf, ARC_PATH_MAX - 1);
                if (!IS_ERR(path_nm)) {
                    if (!strcmp(kbasename(path_nm), name)) {
                        base = vma->vm_start;
                        break;
                    }
                }
            }
        }
    }
#endif

    mmput(mm);

put_pid:
    put_pid(pid_struct);
    return base;
}
