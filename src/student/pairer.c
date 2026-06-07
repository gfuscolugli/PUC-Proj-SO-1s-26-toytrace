#include "student_api.h"

int student_pair_syscall(struct syscall_pairer *pairer,
                         const struct syscall_event *ev,
                         struct syscall_event *out)
{
    // Usamos variáveis estáticas para guardar o estado (a entrada) 
    // entre as chamadas da função, já que estamos assumindo 1 processo alvo.
    static struct syscall_event saved_entry;
    static int waiting_for_exit = 0;

    if (ev->entering == 1) { // 1. Parada de ENTRADA
        
        // Se já estávamos esperando uma saída e chegou outra entrada, deu erro na sequência
        if (waiting_for_exit) {
            return -1;
        }
        
        // Salva as informações de entrada (que contêm os argumentos)
        saved_entry = *ev;
        waiting_for_exit = 1;
        
        // Retorna 0 porque ainda falta a saída para completar a syscall
        return 0;
        
    } else { // 2. Parada de SAÍDA (ev->entering == 0)
        
        // Se chegou uma saída mas não tínhamos guardado nenhuma entrada, erro na sequência
        if (!waiting_for_exit) {
            return -1;
        }
        
        // Se a saída for de uma syscall diferente da entrada, erro na sequência
        if (saved_entry.syscall_no != ev->syscall_no) {
            return -1;
        }

        // SUCESSO! Vamos juntar as duas metades no ponteiro 'out'
        *out = saved_entry;                 // Copia a base (com o pid, número e argumentos)
        out->ret = ev->ret;
        out->entering = 0;                  // (Opcional) padronizamos como finalizada

        // Limpa o estado avisando que estamos prontos para a próxima syscall
        waiting_for_exit = 0;

        // Retorna 1 avisando ao programa principal que o 'out' tem uma syscall completa
        return 1;
    }
}