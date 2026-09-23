// SPDX-License-Identifier: GPL-2.0
/*
 * verify.c — Verificação de chave (desabilitada)
 *
 * Auth foi desabilitada: init_key() sempre retorna true.
 *
 * Créditos:
 *   - rogxo: Autor original da base kernel_hack
 *   - ScxttVX: Desabilitação da auth
 */

#include "verify.h"
#include <linux/kernel.h>

bool init_key(char __user *key, size_t len_key)
{
    (void)key;
    (void)len_key;
    return true;
}
