#include "student_api.h"
#include "syscall_names.h"
#include "trace_helpers.h" 
#include <stdio.h>
#include <sys/syscall.h>   

void student_debug_raw_event(const struct syscall_event *ev,
                             char *buf,
                             size_t bufsz)
{
    if (ev->entering) 
    {
        // Na entrada, mostramos o nome da syscall e os 6 registradores de argumentos
        snprintf(buf, bufsz, "pid=%d %s entrada (args: %llu, %llu, %llu, %llu, %llu, %llu)",
                 ev->pid, 
                 syscall_name(ev->syscall_no),
                (unsigned long long)ev->args[0], (unsigned long long)ev->args[1],
                (unsigned long long)ev->args[2], (unsigned long long)ev->args[3],
                (unsigned long long)ev->args[4], (unsigned long long)ev->args[5]);
    } 
    else 
    {
        // Na saída, mostramos o nome da syscall e o valor de retorno contido no rax
        snprintf(buf, bufsz, "pid=%d %s saida (retorno: %lld)",
                ev->pid, 
                syscall_name(ev->syscall_no),
                (long long)ev->ret); 
    }
}

void student_format_event(const struct syscall_event *ev,
                          char *buf,
                          size_t bufsz)
{
    // Inicializando com zeros para evitar lixo de memoria
    char path_buf[256] = {0};

    // Prevenção de crash caso a syscall seja desconhecida
    const char *sys_name = syscall_name(ev->syscall_no);
    if (!sys_name) sys_name = "unknown";

    switch (ev->syscall_no) {
        
        // --- casos especiais ---

        case SYS_read:
            snprintf(buf, bufsz, "read(%d, 0x%lx, %zu) = %ld",
                     (int)ev->args[0], ev->args[1], (size_t)ev->args[2], ev->ret);
            break;

        case SYS_write:
            snprintf(buf, bufsz, "write(%d, 0x%lx, %zu) = %ld",
                     (int)ev->args[0], ev->args[1], (size_t)ev->args[2], ev->ret);
            break;

        case SYS_execve:
            // execve: pathname esta no primeiro argumento (args[0])
            if (read_child_string(ev->pid, ev->args[0], path_buf, sizeof(path_buf)) < 0) {
                snprintf(path_buf, sizeof(path_buf), "<ilegivel>");
            }
            snprintf(buf, bufsz, "execve(\"%s\", ...) = %ld",
                     path_buf, ev->ret);
            break;

        case SYS_openat:
            // openat: pathname esta no segundo argumento (args[1])
            if (read_child_string(ev->pid, ev->args[1], path_buf, sizeof(path_buf)) < 0) {
                snprintf(path_buf, sizeof(path_buf), "<ilegivel>");
            }
            snprintf(buf, bufsz, "openat(%d, \"%s\", 0x%x, 0x%x) = %ld",
                     (int)ev->args[0], path_buf, (unsigned int)ev->args[2], (unsigned int)ev->args[3], ev->ret);
            break;

        case SYS_exit_group:
            snprintf(buf, bufsz, "exit_group(%d) = %ld",
                     (int)ev->args[0], ev->ret);
            break;

        default:
            // Voltando para %lx para que endereços de memoria (ponteiros) sejam impressos corretamente
            snprintf(buf, bufsz, "%s(%#lx, %#lx, %#lx, %#lx, %#lx, %#lx) = %ld",
                     sys_name,
                     ev->args[0], 
                     ev->args[1], 
                     ev->args[2], 
                     ev->args[3], 
                     ev->args[4], 
                     ev->args[5], 
                     ev->ret);
            break;
    }
}