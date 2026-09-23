# 🔥 droidmem

**Leitura e escrita de memória de processos Android via `/proc/pid/mem` e kernel module**

Ferramenta de leitura/escrita de memória para processos Android, com suporte a kernel module (`.ko`) e modo usermode via `/proc/pid/mem`. Testado no emulador BlueStacks/MuMuPlayer com FreeFire (`libil2cpp.so`).

> ⚠️ **AVISO LEGAL**: Este projeto é exclusivamente para fins educacionais e de pesquisa. O uso para qualquer finalidade ilegal é de inteira responsabilidade do usuário. O autor não se responsabiliza pelo uso indevido desta ferramenta.

---

## 📋 Funcionalidades

- ✅ **Leitura de memória** de qualquer processo Android com root
- ✅ **Escrita de memória** em processos Android
- ✅ **Busca automática de módulos** via `/proc/pid/maps`
- ✅ **Fallback automático**: tenta kernel module primeiro, cai para `/proc/pid/mem`
- ✅ **Compatível** com Android 5.0+ (Linux 3.2+)
- ✅ **Funciona em emuladores** (BlueStacks, MuMuPlayer, etc.)
- ✅ **Funciona em dispositivos físicos** com root

---

## 🎯 Testado Com

| Aplicação | Biblioteca | Status |
|-----------|-----------|--------|
| FreeFire (`com.dts.freefireth`) | `libil2cpp.so` | ✅ Funcionando |
| Chrome (`com.android.chrome`) | `libmonochrome.so` | ✅ Funcionando |
| BlueStacks | Kernel 4.19.195 x86_64 | ✅ Funcionando |

---

## 🏗️ Estrutura do Projeto

```
droidmem/
├── README.md                    # Este arquivo
├── CONTRIBUTING.md              # Guia de contribuição
├── LICENSE                      # Licença MIT
├── .gitignore                   # Arquivos ignorados pelo Git
├── compilex64.ps1               # Compilação x86_64 (emulador)
├── compile.ps1                  # Compilação ARM64 (dispositivos 64-bit)
├── compile32.ps1                # Compilação ARM32 (dispositivos 32-bit)
├── compile_test_proc.ps1        # Compilação do test_proc
├── build.bat                    # Build completo
├── build_module.bat             # Build do kernel module
└── droidmem/
    ├── um/                      # Usermode
    │   ├── main.cpp             # Aplicação principal
    │   ├── driver.hpp           # Driver unificado (kernel + /proc/pid/mem)
    │   ├── proc_mem.hpp         # Leitura via /proc/pid/mem
    │   ├── test_proc.cpp        # Teste standalone
    │   ├── Makefile             # Makefile Linux
    │   └── main64               # Binário compilado (não commitar)
    └── km/                      # Kernel module
        ├── entry.c              # Ponto de entrada do módulo
        ├── memory.c             # Page table walking
        ├── process.c            # Busca de módulos
        ├── verify.c             # Verificação (auth desabilitada)
        ├── comm.h               # Structs compartilhadas
        ├── Makefile             # Build do módulo
        └── droidmem.ko          # Módulo compilado (não commitar)
```

---

## 🚀 Instalação

### Pré-requisitos

- **Android NDK** (r25+ recomendado)
- **ADB** configurado no PATH
- **Emulador** (BlueStacks/MuMuPlayer) ou **dispositivo físico** com root
- **WSL** ou **Linux** (para compilar o kernel module, opcional)

### Compilar o Usermode

```powershell
# x86_64 (emuladores)
.\compilex64.ps1

# ARM64 (dispositivos 64-bit)
.\compile.ps1

# ARM32 (dispositivos 32-bit)
.\compile32.ps1
```

### Compilar o Teste Standalone

```powershell
.\compile_test_proc.ps1
```

### Deploy no Emulador

```cmd
set ADB=C:\Users\VX\AppData\Local\Android\Sdk\platform-tools\adb.exe

:: Enviar binário
%ADB% push droidmem\um\main64 /data/local/tmp/
%ADB% shell chmod 755 /data/local/tmp/main64

:: Testar com FreeFire
%ADB% shell "su -c /data/local/tmp/main64 com.dts.freefireth libil2cpp.so"
```

---

## 📖 Uso

### Sintaxe

```bash
./main64 <package_name> <module_name> [endereco_hex]
```

### Exemplos

```bash
# FreeFire - ler libil2cpp.so
./main64 com.dts.freefireth libil2cpp.so

# Chrome - ler libmonochrome.so
./main64 com.android.chrome libmonochrome.so

# Endereço específico
./main64 com.dts.freefireth libil2cpp.so 0xa2c02000
```

### Output Esperado

```
[*] Buscando PID de: com.dts.freefireth
[+] PID: 7198
[*] /dev/droidmem não disponível, usando /proc/pid/mem
[+] ptrace conectado ao PID 7198
[*] Buscando módulo: libil2cpp.so
[+] Base do módulo: 0xa2c02000
[*] Lendo 8 bytes de 0xa2c02000...
[+] 100 leituras em 1 ms (média: 0.010 ms)
[+] Valor: 0x00010101464c457f
[*] Teste multi-página (64KB)...
[+] Multi-página OK: 0 ms
[+] Todos os testes passaram.
```

---

## ⚙️ Arquitetura

### Driver Unificado (`driver.hpp`)

O driver opera em dois modos:

1. **Kernel Module** (`/dev/droidmem`): Se o módulo `.ko` estiver carregado, usa ioctl para ler/escrever memória via page table walking no kernel.

2. **Usermode** (`/proc/pid/mem`): Fallback automático quando o módulo não está disponível. Usa ptrace + `/proc/pid/mem` para acesso direto à memória do processo.

### Fluxo de Execução

```
┌─────────────────────────────────────┐
│  1. Buscar PID por package name     │
│     (pidof via popen)               │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  2. Tentar abrir /dev/droidmem     │
│     → Se OK: usar kernel module     │
│     → Se ERRO: usar /proc/pid/mem   │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  3. ptrace attach ao processo       │
│     (necessário para /proc/pid/mem) │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  4. Buscar base do módulo           │
│     (parse /proc/pid/maps)          │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  5. Ler/escrever memória            │
│     (read/write via /proc/pid/mem)  │
└─────────────────────────────────────┘
```

### Kernel Module (`km/`)

O módulo do kernel implementa:

- **Page Table Walking**: tradução de endereços virtuais → físicos
  - Suporte a kernels 3.x/4.x (4 níveis de tabela)
  - Suporte a kernels 5.4+ (5 níveis com `p4d`)
  - Suporte a kernels 6.1+ (`pte_offset_map`)

- **`ioremap_cache`**: mapeamento de memória física para kernel space
- **`copy_to_user`/`copy_from_user`**: transferência kernel ↔ userspace
- **Referência correta**: `get_task_mm` → `put_task_struct` → `mmput` → `put_pid`

---

## 🐛 Correções Aplicadas

| Bug | Descrição | Status |
|-----|-----------|--------|
| **Use-after-free** | `get_task_mm` + `put_task_struct` com ordem incorreta | ✅ Corrigido |
| **Memory leak** | `mmput` e `put_pid` não chamados | ✅ Corrigido |
| **Multi-page** | Leitura não suportava boundaries de página | ✅ Corrigido |
| **Compatibilidade** | Kernel 3.x/4.x/5.x/6.x incompatível | ✅ Corrigido |
| **compat_ioctl** | 32-bit em 64-bit não funcionava | ✅ Corrigido |
| **Auth** | Autenticação desnecessária | ✅ Removida |

---

## 📊 Performance

| Método | Velocidade (100 reads) | Velocidade (64KB) |
|--------|----------------------|-------------------|
| `/proc/pid/mem` | ~1ms (0.010ms/read) | ~1ms |
| Kernel module | ~0.5ms | ~0.5ms |

---

## 🔧 Kernel Module (Opcional)

O kernel module é **opcional** — o usermode via `/proc/pid/mem` já funciona perfeitamente.

Se quiser compilar o `.ko` para o emulador:

```bash
# Clonar kernel AOSP 4.19
git clone --depth=1 --branch android-4.19-common \
    https://android.googlesource.com/kernel/common kernel_src

# Compilar módulo
make -C kernel_src M=$(pwd)/droidmem/km ARCH=x86 modules

# Deploy
adb push droidmem/km/droidmem.ko /data/local/tmp/
adb shell su -c "insmod /data/local/tmp/droidmem.ko"
adb shell su -c "chmod 666 /dev/droidmem"
```

---

## 📝 Notas

- **Root necessário**: O dispositivo/emulador precisa ter root (Magisk, SuperSU, etc.)
- **ptrace**: Se ptrace falhar, `/proc/pid/mem` ainda funciona como root
- **Emuladores**: Detecta automaticamente `goldfish`, `qemu_pipe`, etc.
- **Anti-cheat**: Ferramentas como ACE detectam facilmente este tipo de acesso

---

## 📄 Licença

Este projeto está licenciado sob a MIT License - veja o arquivo [LICENSE](LICENSE) para detalhes.

---

## 🛡️ Contornando o ACE (Anti-Cheat)

> ⚠️ **AVISO**: Esta seção é exclusivamente para fins educacionais e de pesquisa em segurança. O uso para trapaça em jogos é proibido pelos termos de serviço.

O ACE (Anti-Cheat) do FreeFire detecta múltiplos vetores de manipulação de memória. Para contornar, são necessárias modificações em múltiplas camadas:

### Vetores de Detecção do ACE

| Vetor | Como o ACE detecta |
|-------|-------------------|
| **ptrace** | Monitora `PTRACE_ATTACH` — verifica se processo está em estado `traced` |
| **/proc/pid/mem** | Detecta abertura de `/proc/<pid>/mem` por processo externo |
| **/proc/pid/maps** | Monitora leitura do maps por outros processos |
| **Root detection** | Verifica presença de Magisk, SuperSU, su binary |
| **Emulator detection** | Detecta BlueStacks/MuMuPlayer por arquivos e properties |
| **Integrity checks** | Compara hashes de memória em runtime |
| **eBPF/kprobe** | ACE pode usar eBPF para monitorar syscalls |

### Abordagens de Contorno (Nível Educacional)

#### 1. Kernel Module Level (Mais Eficaz)

```
┌─────────────────────────────────────────────┐
│  Kernel Module (.ko)                        │
│  ├─ Ocultar módulo de /proc/modules         │
│  ├─ Hook de syscalls (open, read, write)    │
│  ├─ Interceptação de ptrace                 │
│  └─ Proteção contra integrity checks        │
└─────────────────────────────────────────────┘
```

**Técnicas:**
- **Module hiding**: Remover módulo da lista de módulos carregados
- **Syscall hooking**: Interceptação de `__x64_sys_openat`, `__x64_sys_read`
- **ptrace spoofing**: Retornar estado falso quando verificado
- **Anti-debug**: Detectar e bloquear ferramentas de depuração

#### 2. Usermode Level (Mais Fácil)

```
┌─────────────────────────────────────────────┐
│  Usermode                                    │
│  ├─ Binder hook (comunicação inter-processo)│
│  ├─ PLT/GOT hooking                         │
│  ├─ Inline hooking                          │
│  └─ Dlopen/dlsym interception               │
└─────────────────────────────────────────────┘
```

**Técnicas:**
- **Binder hook**: Interceptação de chamadas IPC do Android
- **PLT hook**: Sobrescrever tabela de procedimentos
- **GOT hook**: Modificar Global Offset Table
- **Memory mapping**: Usar `mmap` com permissões específicas

#### 3. Hypervisor Level (Mais Avançado)

```
┌─────────────────────────────────────────────┐
│  Hypervisor (KVM/Xen)                       │
│  ├─ VM-level memory isolation               │
│  ├─ Hardware breakpoints                    │
│  ├─ Page fault handling                     │
│  └─ Transparent to guest OS                 │
└─────────────────────────────────────────────┘
```

### Exemplo: Ocultar Módulo do Kernel

```c
// droidmem/km/hide.c (conceitual)
#include <linux/module.h>
#include <linux/list.h>

// Remover da lista de módulos
static void hide_module(void) {
    list_del_init(&THIS_MODULE->list);
    kobject_del(&THIS_MODULE->mkobj.kobj);
}

// Restaurar visibility
static void show_module(void) {
    list_add(&THIS_MODULE->list, __this_module.prev);
    kobject_add(&THIS_MODULE->mkobj.kobj,
                &THIS_MODULE->mkobj.kobj.parent,
                KBUILD_MODNAME);
}
```

### Exemplo: Hook de Syscall (Conceitual)

```c
// droidmem/km/syscall_hook.c (conceitual)
#include <linux/kprobes.h>

// Interceptação de openat para ocultar arquivos
static int handler_openat(struct kprobe *p, struct pt_regs *regs) {
    char __user *filename = (char __user *)regs->di;

    // Verificar se está acessando /proc/modules
    if (strstr(filename, "/proc/modules")) {
        // Retornar erro ou dados modificados
        return -ENOENT;
    }
    return 0;
}
```

### Ferramentas Úteis para Pesquisa

| Ferramenta | Propósito |
|-----------|-----------|
| **frida** | Dynamic instrumentation framework |
| **Il2CppDumper** | Dump de libil2cpp.so |
| **Ghidra** | Engenharia reversa |
| **strace** | Rastreamento de syscalls |
| **ltrace** | Rastreamento de chamadas de biblioteca |
| **procmap** | Análise de memória de processos |

### Limitações desta Abordagem

| Método | Detectado pelo ACE? | Dificuldade |
|--------|-------------------|-------------|
| `/proc/pid/mem` direto | ✅ Sim, facilmente | Fácil |
| ptrace básico | ✅ Sim | Fácil |
| Kernel module simples | ✅ Sim | Médio |
| Module hiding | ⚠️ Parcialmente | Médio |
| Syscall hooking | ⚠️ Parcialmente | Difícil |
| Hypervisor | ❌ Difícil de detectar | Muito Difícil |

### Nota Importante

O ACE está em constante evolução. Qualquer técnica de contorno pode ser detectada em versões futuras. Este projeto é para **pesquisa educacional** — não garantimos funcionamento contínuo contra atualizações do anti-cheat.

---

## 📊 Status do Projeto

| Componente | Status |
|------------|--------|
| Usermode driver | ✅ Funcional |
| Auto-detect (kernel → /proc/pid/mem) | ✅ Funcional |
| FreeFire (libil2cpp.so) | ✅ Testado |
| Chrome (libmonochrome.so) | ✅ Testado |
| Read/Write memória | ✅ Suportado |
| Velocidade | ✅ ~10μs por read |
| Android 5.0+ | ✅ Compatível |
| Emuladores | ✅ BlueStacks/MuMuPlayer |
| Dispositivos físicos | ✅ Com root |

---

## 🙏 Créditos

- **rogxo** ([@rogxo](https://github.com/rogxo)) — Autor original da base kernel_hack com suporte a kernel module `.ko` e page table walking
- **ScxttVX** ([@ScxttVX](https://github.com/ScxttVX)) — Correções de bugs (use-after-free, memory leaks), adição do fallback `/proc/pid/mem`, compatibilidade multi-kernel (3.x a 6.x), organização do projeto e testes no emulador BlueStacks com FreeFire
