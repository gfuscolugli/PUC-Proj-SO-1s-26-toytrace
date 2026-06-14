#include "student_api.h"

int student_pair_syscall(struct syscall_pairer *pairer,
                         const struct syscall_event *ev,
                         struct syscall_event *out)
{
    static struct syscall_event saved_entry;
    static int waiting_for_exit = 0;

    // Se estamos esperando uma saída e a syscall atual é a MESMA que guardamos
    if (waiting_for_exit && saved_entry.syscall_no == ev->syscall_no) {
        
        // --- AS LINHAS QUE FALTAVAM ESTÃO AQUI ---
        *out = saved_entry;
        out->ret = ev->ret;
        out->entering = 0;
        // ----------------------------------------

        waiting_for_exit = 0; // Zera o estado para a próxima
        return 1;
    }

    // Se não bateu (ou se não estávamos esperando nada), assumimos como nova ENTRADA.
    saved_entry = *ev;
    waiting_for_exit = 1;
    
    return 0;
}