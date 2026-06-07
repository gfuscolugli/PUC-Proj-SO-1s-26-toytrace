#include "student_api.h"
#include "syscall_names.h"
#include <stdio.h>

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
    /*
     * TODO Semana 5:
     *
     * Primeiro, formate uma syscall completa em uma linha simples.
     *
     * Depois, adicione casos especiais para:
     * read(fd, buf, count)
     * write(fd, buf, count)
     * openat(dirfd, "path", flags, mode)
     * execve("path", ...)
     * exit_group(status)
     *
     * Para caminhos do processo monitorado, use read_child_string().
     * Se a leitura falhar, imprima "<ilegivel>".
     */
    snprintf(buf, bufsz, "%s(%#lx, %#lx, %#lx, %#lx, %#lx, %#lx) = %ld",
             syscall_name(ev->syscall_no),
             ev->args[0],
             ev->args[1],
             ev->args[2],
             ev->args[3],
             ev->args[4],
             ev->args[5],
             ev->ret);
}