#include <mpi.h>
#include <stdlib.h>
#include "mpi_main.h"
#define USING_THREADS 1



// inicializa mpi e roda um função, passando o rank e size de comunicação
void mpi_with(void (*fn)(i32, i32),int argc, char* argv[])
{

	int process_count, rank, provided;
#if defined USING_THREADS && USING_THREADS > 0
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
#else
	MPI_Init(NULL, NULL);
#endif
	MPI_Comm_size(MPI_COMM_WORLD, &process_count);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	fn(rank, process_count);
	MPI_Finalize();
}



/*--------------------------MAIN--------------------------------------*/
int main(int argc, char* argv[])
{
	//printf("MAIN COMECOU\n");
	mpi_with(mpi_main,argc,argv);
}
