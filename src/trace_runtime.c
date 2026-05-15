#include "trace_runtime.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#if !defined(__x86_64__)
#error "Este runtime didatico suporta apenas Linux x86_64."
#endif

static void fill_event_from_regs(pid_t pid,
                                 int entering,
                                 const struct user_regs_struct *regs,
                                 struct syscall_event *ev)
{
    /*
     * TODO Semana 4:
     *
     * Preencha struct syscall_event usando os registradores x86_64.
     *
     * Dicas:
     * - regs->orig_rax contem o numero da syscall.
     * - regs->rax contem o retorno, valido na saida.
     * - os seis argumentos ficam em rdi, rsi, rdx, r10, r8 e r9.
     * - ev->entering deve copiar o parametro entering.
     */
    memset(ev, 0, sizeof(*ev));
    ev->pid = pid;
    ev->entering = entering;
}

static pid_t launch_tracee(char *const argv[])
{
    pid_t pid = fork(); 

    if(pid<0){
        perror("Erro no fork");  // Erro no fork, retorna o erro (-1)
        return -1;
    }
    
    if(pid==0){
      // Se entrar no if, processo filho avisa o Kernel que será rastreado pelo processo PAI
        if(ptrace(PTRACE_TRACEME, 0, NULL, NULL)< 0){
            perror("Erro no ptrace");
            exit(1);
        }  

        raise(SIGSTOP); //Processo filho para a si mesmo antes de executar o processo alvo
        
        if(execvp(argv[0],argv)<0){ //subsititui o processo filho pelo programa alvo
            perror("Erro no execvp"); // houve erro no execvp
            exit(1);
        }
    }

    // retorna o pid do filho 
    return pid;
}

static int wait_for_initial_stop(pid_t child)
{
    int status;

    //Pai espera o filho mudar de estado, ou seja, para com o SIGSTOP
    if(waitpid(child, &status, 0)< 0){
        perror("Erro no waitpid");
        return -1;
    }

    // Aqui verifica se o filho parou como esperado pelo SIGSTOP
    if(WIFSTOPPED(status)){
        return 0; // DEu certto filho parou 
    }

    fprintf(stderr, "Erro, filho não parou como esperado\n"); // deu errado, o filho não parou (houve falha)
    return -1;
}

static int configure_trace_options(pid_t child)
{
    /*
     * TODO Semana 3:
     *
     * Configure PTRACE_O_TRACESYSGOOD com PTRACE_SETOPTIONS.
     * Isso ajuda a diferenciar paradas de syscall de outros sinais.
     */
    fprintf(stderr, "erro: TODO Semana 3: implementar configure_trace_options()\n");
    return -1;
}

static int resume_until_next_syscall(pid_t child, int signal_to_deliver)
{
    /*
     * TODO Semana 3:
     *
     * Use ptrace(PTRACE_SYSCALL, ...) para deixar o filho executar ate a
     * proxima entrada ou saida de syscall.
     *
     * signal_to_deliver deve ser repassado como quarto argumento do ptrace.
     */
    fprintf(stderr, "erro: TODO Semana 3: implementar resume_until_next_syscall()\n");
    return -1;
}

static int wait_for_syscall_stop(pid_t child, int *status)
{
    /*
     * TODO Semana 3:
     *
     * Espere o filho com waitpid().
     *
     * Retorne:
     *   1 se a parada foi uma parada de syscall;
     *   0 se o filho terminou normalmente ou por sinal;
     *  -1 em erro.
     *
     * Dicas:
     * - WIFEXITED e WIFSIGNALED indicam fim do processo.
     * - WIFSTOPPED indica que o processo parou.
     * - com PTRACE_O_TRACESYSGOOD, syscall-stops aparecem com bit 0x80.
     * - paradas SIGTRAP comuns nao devem ser entregues de volta ao filho.
     */
    fprintf(stderr, "erro: TODO Semana 3: implementar wait_for_syscall_stop()\n");
    return -1;
}

int trace_program(char *const argv[],
                  trace_observer_fn observer,
                  void *userdata)
{
    pid_t child;
    int status = 0;
    int entering = 1;

    if (argv == NULL || argv[0] == NULL) {
        fprintf(stderr, "erro: programa alvo ausente\n");
        return -1;
    }

    child = launch_tracee(argv);
    if (child < 0) {
        return -1;
    }

    if (wait_for_initial_stop(child) < 0) {
        return -1;
    }

    if (configure_trace_options(child) < 0) {
        return -1;
    }

    if (resume_until_next_syscall(child, 0) < 0) {
        return -1;
    }

    while (1) {
        struct user_regs_struct regs;
        struct syscall_event ev;
        int stop_kind;

        stop_kind = wait_for_syscall_stop(child, &status);
        if (stop_kind < 0) {
            return -1;
        }
        if (stop_kind == 0) {
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            }
            return 0;
        }

        /*
         * TODO Semana 4:
         *
         * Use PTRACE_GETREGS para preencher regs.
         * Depois chame fill_event_from_regs() e observer().
         */
        memset(&regs, 0, sizeof(regs));
        fill_event_from_regs(child, entering, &regs, &ev);
        if (observer != NULL) {
            observer(&ev, userdata);
        }

        entering = !entering;

        if (resume_until_next_syscall(child, 0) < 0) {
            return -1;
        }
    }
}
