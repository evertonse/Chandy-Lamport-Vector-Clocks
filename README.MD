## Implementação do Algoritmo de Chandy-Lamport para sistema distribuido que se comunicam por MPI e as mensagens são vectorclocks. 
#### Essa implementação usa 3 threads por processo que se comunica:
* 1ª thread é o algoritmo básico do usuario
* 2ª thread é a o `emitter thread`  cujo trabalho é cosumir de uma fila de msg, e envia essa mensagem para outro processo usando MPI
* 2ª thread é a o `receiver thread` cujo trabalho é receber mensagens do MPI, se for um marcador do snapshot, envia para a fila de envios, se for uma msg para o algoritmo básica, então envia para a fila de msgs para  algortimo básico
* 
## Compile:
	mpicc -o main -Wall -I./ vector_clock.c atomic_queue.c mpi_main.c main.c -I./ -lpthread -lrt
## Run (expects 3 processes):
	mpiexec -n 3 ./main

Utilizando o modelo produtor / consumidor de uma fila de msg -> [from, to, vector_clock]

<img src="/img/produtor_cosumidor_mpi.png" width="720">

Implementado esse relogio vetorial para 3 processos:

<img src="/img/process_clock_events.png" width="720">

