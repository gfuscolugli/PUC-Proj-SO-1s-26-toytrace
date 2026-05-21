# Semana 3 – Loop de Tracing

## O que foi implementado
Nesta semana, estruturamos o loop principal do `toytrace`. Ele utiliza três funções em conjunto para permitir que o processo pai consiga acompanhar cada chamada de sistema (syscall) executada pelo processo filho.

### configure_trace_options
Roda apenas uma vez, logo após o filho ser pausado com o `SIGSTOP` inicial. Usamos o `PTRACE_SETOPTIONS` para ativar a flag `PTRACE_O_TRACESYSGOOD`. O objetivo disso é pedir ao kernel que marque as paradas de syscall com o bit `0x80` no sinal enviado. Isso nos permite separar de forma clara o que é uma syscall do que é um sinal de parada comum.

### resume_until_next_syscall
Utiliza o `ptrace(PTRACE_SYSCALL, ...)` para mandar o filho continuar a execução, mas com uma pausa programada: ele vai parar assim que tentar entrar ou sair de uma syscall. O parâmetro `signal_to_deliver` é passado no quarto argumento para garantir que sinais reais pendentes sejam entregues ao processo.

### wait_for_syscall_stop
Bloqueia a execução do pai usando `waitpid` até que o filho mude de estado. O tratamento do status ficou assim:
* **Retorna 1:** Quando é uma syscall legítima (identificamos isso isolando o bit `0x80` no status recebido).
* **Retorna 0:** Quando o processo não está mais em uma syscall. Pode ser porque ele encerrou normalmente (`WIFEXITED`), foi morto por um sinal (`WIFSIGNALED`), ou parou por um sinal comum que não nos interessa no momento.
* **Retorna -1:** Caso ocorra alguma falha no `waitpid` ou erro de consistência.

---

## O Fluxo do Loop
Após rodar o `configure_trace_options(child)` para preparar a rastreabilidade, o fluxo principal fica iterando da seguinte forma:

```text
loop:
    resume_until_next_syscall(child, sinal)
    r = wait_for_syscall_stop(child, &status)

    se r == 1  → É uma parada de syscall: prosseguir para leitura dos registradores (Semana 4).
    se r == 0  → Qualquer outra parada (fim da execução ou sinal comum): tratar ou sair do loop.
    se r == -1 → Falha no processo: abortar execução.

## Decisões de Implementação
Para deixar o projeto mais limpo e seguro, tomamos algumas decisões durante a escrita do código:

* **Early Return no fluxo de checagem:** Evitamos aninhamentos longos de `if/else` na função `wait_for_syscall_stop`. O código testa os cenários e, se o bit `0x80` não estiver presente, ele avança naturalmente e retorna `0` no final da função. Fica muito mais legível.
* **Máscara de Bits:** Em vez de comparar o sinal recebido usando uma igualdade engessada, utilizamos a expressão `WSTOPSIG(*status) & 0x80`. É uma abordagem idiomática e mais segura, pois foca apenas na presença do bit injetado pelo kernel, ignorando outros status secundários que poderiam quebrar a verificação.
* **Evitando o NULL no ptrace:** Nas chamadas em que o terceiro argumento não é utilizado (`PTRACE_SETOPTIONS` e `PTRACE_SYSCALL`), decidimos passar o valor numérico `0` em vez de `NULL`. Como a função espera um valor numérico de endereço e não um ponteiro, isso evita warnings de incompatibilidade de tipo no compilador C.

---

## Observação sobre Entrada e Saída
Um ponto fundamental compreendido nesta semana é que o `PTRACE_SYSCALL` pausa o processo filho em **dois momentos distintos** para a mesma chamada de sistema:
1. **Entrada:** Antes de o kernel executar a ação de fato. Os argumentos da chamada ainda estão nos registradores padrão (rdi, rsi, etc.).
2. **Saída:** Logo após o kernel finalizar o trabalho, com o valor de retorno disponível no registrador `rax`.

Isso significa que o nosso loop vai executar duas vezes para cada syscall identificada. O desafio de cruzar os dados de entrada com os de saída ficará para as próximas semanas.

---

## Dúvidas Esclarecidas

* **Repasse de sinais comuns:** Se o `wait_for_syscall_stop` retornar `0` por causa de um sinal comum recebido, o controle disso não precisa ser feito nas nossas três funções. O próprio laço do `main.c` do projeto recebe esse status e encaminha o sinal retornado na próxima chamada do `resume_until_next_syscall`.
* **SIGTRAP sem o bit 0x80:** É um cenário possível no mundo real (como ao atingir um breakpoint de código `int3` ou rodar uma instrução passo a passo). Embora não ocorra no programa alvo de teste (`hello_write`), nossa implementação com a máscara de bits isola perfeitamente esses casos, impedindo que sejam classificados como syscalls.
* **Múltiplas paradas na mesma syscall:** Geralmente teremos as duas paradas (entrada/saída). Mas, caso o kernel precise interromper uma syscall para tratar um sinal urgente e usar a flag `SA_RESTART` para tentar executá-la de novo, isso gerará novas paradas no rastreador. A lógica estrutural que criamos suporta esse comportamento normalmente.

---

## Resultado da Integração
Com o código ajustado, a compilação utilizando `make` ocorreu sem problemas. Ao executar o teste padrão:

```bash
./toytrace trace -- ./tests/targets/hello_write
