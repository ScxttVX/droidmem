#ifndef __VERIFY_H__
#define __VERIFY_H__

#include <linux/kernel.h>

bool init_key(char __user *key, size_t len_key);

#endif /* __VERIFY_H__ */
