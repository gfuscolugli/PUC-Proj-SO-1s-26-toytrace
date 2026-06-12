#include "student_api.h"
#include <sys/syscall.h>  

int student_pair_syscall(struct syscall_pairer *pairer,
                         const struct syscall_event *ev,
                         struct syscall_event *out)
{
    //  guarda o estado entre as chamadas
    static struct syscall_event saved_entry;
    static int waiting_for_exit = 0;

    if (ev->entering == 1) { // 1. Parada de ENTRADA
        
        // RECUPERAÇÃO DE ESTADO:
        saved_entry = *ev;
        waiting_for_exit = 1;
        
        return 0;
        
    } else { // 2. Parada de SAÍDA (ev->entering == 0)
        
        // Ignora saídas órfãs sem quebrar o loop
        if (!waiting_for_exit) {
            return 0;
        }
        
        // Se a saída for de uma syscall diferente da entrada, desincronizou.
        // Resetamos o estado e ignoramos, sem abortar o tracer.
        if (saved_entry.syscall_no != ev->syscall_no) {
            waiting_for_exit = 0;
            return 0;
        }

        // SUCESSO! Vamos juntar as duas metades no ponteiro 'out'
        *out = saved_entry;
        out->ret = ev->ret;
        out->entering = 0;
        
        // Limpa o estado 
        waiting_for_exit = 0;
        
        return 1;
    }
}