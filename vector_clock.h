#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "common.h"
#include "dunder.h"
// Explicação das funções encotrada na implementação mpi_clock.c
typedef struct _vector_clock_t{
// public:	
	i32 rank;
	void (*receive)		(struct _vector_clock_t *self, struct _vector_clock_t* other);
	void (*send)			(struct _vector_clock_t *self);
	void (*event)			(struct _vector_clock_t *self);

// private:	
	i32* _buffer;
	i32 _size;
	pthread_mutex_t * _mutex;
	void	(*__print__)	(struct _vector_clock_t*);
	i32	(*__len__)		(struct _vector_clock_t*);
	void	(*__del__)		(struct _vector_clock_t*);
	void	(*__copy__)		(struct _vector_clock_t*, struct _vector_clock_t*);
	void	(*__str__)		(struct _vector_clock_t*, char*);
} vector_clock_t;

vector_clock_t vector_clock(i32 comm_sz, i32 rank);

void VC_receive	(vector_clock_t *self, vector_clock_t* other);
void VC_send		(vector_clock_t *self);
void VC_event		(vector_clock_t *self);
i32  VC_size		(vector_clock_t *self);

DUNDER int 	VC__len__	(vector_clock_t *self);
DUNDER void VC__print__	(vector_clock_t *self);
DUNDER void VC__del__	(vector_clock_t *self);
DUNDER void VC__copy__	(vector_clock_t *source, vector_clock_t *buffer);
DUNDER void VC__str__	(vector_clock_t *source, char *buffer);



/*	
	Vector Clocks = Noção de ordem
	
	Consider: vector_clock_t vc;

	THREE CASES from P0 to P3:
	(1) Evento Interno - não recebe e nem envia:
		P0:vc0[my_rank]++;

	(2) Send from P0->P1
		Sender:
		P0:vc0[my_rank]++;

	(3) Receive P1<-P0
		Receiver:
		P1:vc1[my_rank]++;
		for i in comm_sz:
			if vc0[i] > vc1[i]:
				vc1[i] = vc0[i]

*/
 