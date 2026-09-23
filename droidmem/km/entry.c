// SPDX-License-Identifier: GPL-2.0
/*
 * entry.c — Ponto de entrada do módulo kernel
 *
 * Registra device misc /dev/droidmem e despacha comandos ioctl.
 *
 * Créditos:
 *   - rogxo: Autor original da base kernel_hack
 *   - ScxttVX: Correções de bugs, compat_ioctl, multi-kernel
 *
 * Correções aplicadas:
 *  - Adicionado compat_ioctl para userspace 32-bit em kernels 64-bit
 *  - Códigos de erro corretos em vez de -1
 */

#include <linux/module.h>
#include <linux/tty.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include "comm.h"
#include "memory.h"
#include "process.h"
#include "verify.h"

#define DEVICE_NAME "droidmem"

static int dispatch_open(struct inode *node, struct file *file)
{
    return 0;
}

static int dispatch_close(struct inode *node, struct file *file)
{
    return 0;
}

static long dispatch_ioctl(struct file *file, unsigned int cmd,
                           unsigned long arg)
{
    COPY_MEMORY cm;
    MODULE_BASE mb;
    char key[0x100] = {0};
    static bool is_verified = false;

    /* Primeira chamada: inicializar chave (auth desabilitada, sempre sucesso) */
    if (cmd == OP_INIT_KEY && !is_verified) {
        if (copy_from_user(key, (void __user *)arg, sizeof(key) - 1))
            return -EFAULT;
        is_verified = init_key(key, sizeof(key));
    }

    if (!is_verified)
        return -EPERM;

    switch (cmd) {
    case OP_READ_MEM:
        if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)))
            return -EFAULT;
        if (!read_process_memory(cm.pid, cm.addr, cm.buffer, cm.size))
            return -EIO;
        break;

    case OP_WRITE_MEM:
        if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)))
            return -EFAULT;
        if (!write_process_memory(cm.pid, cm.addr, cm.buffer, cm.size))
            return -EIO;
        break;

    case OP_MODULE_BASE:
        if (copy_from_user(&mb, (void __user *)arg, sizeof(mb)))
            return -EFAULT;
        mb.base = get_module_base(mb.pid, mb.name);
        if (copy_to_user((void __user *)arg, &mb, sizeof(mb)))
            return -EFAULT;
        break;

    default:
        return -ENOTTY;
    }

    return 0;
}

/*
 * compat_ioctl — tratar ioctls de userspace 32-bit em kernel 64-bit.
 */
#if IS_ENABLED(CONFIG_COMPAT)
static long dispatch_compat_ioctl(struct file *file, unsigned int cmd,
                                  unsigned long arg)
{
    return dispatch_ioctl(file, cmd, arg);
}
#endif

static const struct file_operations dispatch_functions = {
    .owner   = THIS_MODULE,
    .open    = dispatch_open,
    .release = dispatch_close,
    .unlocked_ioctl = dispatch_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
    .compat_ioctl   = dispatch_compat_ioctl,
#endif
};

static struct miscdevice misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &dispatch_functions,
};

static int __init driver_entry(void)
{
    int ret;
    pr_info("[droidmem] driver_entry\n");
    ret = misc_register(&misc);
    if (ret)
        pr_err("[droidmem] misc_register falhou: %d\n", ret);
    return ret;
}

static void __exit driver_unload(void)
{
    pr_info("[droidmem] driver_unload\n");
    misc_deregister(&misc);
}

module_init(driver_entry);
module_exit(driver_unload);

MODULE_DESCRIPTION("droidmem - Leitura/Escrita de Memória Linux");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rogxo (base) + ScxttVX (correções)");
