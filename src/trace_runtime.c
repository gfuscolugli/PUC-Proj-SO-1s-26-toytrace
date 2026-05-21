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
    // confugura a opção PTRACE_O_TRACEYSGOOD para o filho rastreado, isso faz com que o Kernel (SO) marque as paradas de syscall e as diferencie. 
    if(ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACESYSGOOD)<0){
        perror("\nErro no PTRACE_SETOPTIONS");
        return -1;
    }

    return 0; // Sucesso, deu certo!
}

static int resume_until_next_syscall(pid_t child, int signal_to_deliver)
{
    //O PTRACE_SYSCALL é utilizado para avançar até a próxima I/O de syscall
    // o signal_to_deliver repassa o sinal para o filho 
    if(ptrace(PTRACE_SYSCALL, child, NULL, signal_to_deliver) < 0){
        perror("\nErro no PTRACE_SYSCALL");
        return -1;
    }
    
    return 0; // Sucesso, deu certo!
}

static int wait_for_syscall_stop(pid_t child, int *status)
{
    // verifica se o filho mudou de estado (parou, terminou, etc.)
    if(waitpid(child, status, 0)< 0){
        perror("\nErro no waitpid");
        return -1;
    }

    // Faz a verificação se o filho terminou normalmente (WIFEXITED) ou seja, chamou o exit ao final ...
    //... ou então se terminou por meio de um sinal (WIFSIGNALED), por exemplo o kill.
    if(WIFEXITED(*status) || WIFSIGNALED(*status)){
        return 0;
    }

    // aqui o filho parou e temos que descobri o por que
    if(WIFSTOPPED(*status)){
        //Usando o ptrace_o_tracesysgood as paradas de syscall vem com o bit 0x80 no sinal
        if(WSTOPSIG(*status) & 0x80){
            return 1; // de acordo com as regras de retorno, q foi uma parada de syscall.
            
            // aqui caso pare por algum sinal comum (SIGTRAP),  não repassa o sinal para o filho pOis não foi uma syscall.
        }
    }

    return 0; // significa que filho terminou normalmente ou por parada que não sejaa syscall stop.
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
