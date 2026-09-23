#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <linux/kernel.h>

uintptr_t get_module_base(pid_t pid, char __user *name);

#endif /* __PROCESS_H__ */
