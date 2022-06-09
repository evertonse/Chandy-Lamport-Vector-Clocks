## Compile:
	mpicc -o main -Wall -I./ vector_clock.c atomic_queue.c mpi_main.c main.c -I./ -lpthread -lrt
## Run (expects 3 processes):
	mpiexec -n 3 ./main

Utilizando o modelo produtor / consumidor de uma fila de msg -> [from, to, vector_clock]

<img src="/img/produtor_cosumidor_mpi.png" width="720">

Implementado esse relogio vetorial para 3 processos:

<img src="/img/process_clock_events.png" width="720">

