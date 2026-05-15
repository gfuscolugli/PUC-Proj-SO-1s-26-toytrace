# Mapa Mental - Semana 1

* **Onde o programa começa:** No arquivo `src/main.c`.
* **Onde o processo alvo é criado:** No arquivo `src/trace_runtime.c`, na função `launch_tracee()`.
* **Onde o runtime chama o callback:** No `trace_runtime.c`, que repassa os eventos para a lógica em `src/student/`.
* **Arquivos a se modificar:** A princípio `src/trace_runtime.c` e os arquivos dentro de `src/student/`.
* **Primeiro TODO ao executar o scaffold:** erro: TODO Semana 2: implementar launch_tracee()
* **Principais duvidas:** Entender como o "ptrace" consegue pausar o processo filho antes do execpv.
