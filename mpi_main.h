#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "dunder.h"
#include "common.h"
#include "vector_clock.h"

#define istype(obj,type) __builtin_types_compatible_p(typeof(obj), typeof(type))

#define TASKS_SIZE 8		// QNTD máxima que cabem em uma fila de msg
#include <time.h>
#include <stdbool.h>
#include <mpi.h>     
#include <pthread.h> 

#define MPI_TAG 1



typedef pthread_cond_t pcond_var;
typedef pthread_mutex_t pmutex;
typedef pthread_t pthread;

typedef struct _msg{
	i32 from, to;
	vector_clock_t clock;
	pmutex clock_mutex;
} msg;

typedef struct _process_info{
	i32 comm_sz,rank;
	vector_clock_t clock;
} process_info;


typedef struct _atomic {
	pmutex *mutex;
	pcond_var *wait, *signal;
} atomic;

#define ATOMIC_QTYPE msg* // Definir o tipo do valor da nossa fila
#include "atomic_queue.h"

typedef struct _state_t{
	vector_clock_t clock;
	atomic_queue_t* channels;
	// if channel_done[1] == true, then thats the final stat of the channel 
	bool* channel_done;
	i32 channels_sz;
} state_t;

// MAIN
void mpi_main(i32 rank, i32 comm_sz);


/*----------------------------------UTILS------------------------------*/
void pthread_wake_all();
void utils_write(char* filename, char* content);
/*----------------------------------UTILS------------------------------*/



/*----------------------------------PROCESS------------------------------*/
void Receive(int my_rank,int other_rank, vector_clock_t* clock);
void Send(int my_rank,int other_rank, vector_clock_t* clock);
void Event(int my_rank, vector_clock_t* clock);
void Snapshot(vector_clock_t* clock);
/*----------------------------------PROCESS------------------------------*/

/*----------------------------------QUEUE------------------------------*/
void add_msg(ATOMIC_QTYPE new_msg, atomic_queue_t* queue, atomic* atom);
ATOMIC_QTYPE get_msg(atomic_queue_t* queue, atomic* atom);
void free_msg(msg* m);
void clear_queue(atomic_queue_t* q);
/*----------------------------------QUEUE------------------------------*/


/*----------------------------------SNAPSHOT------------------------------*/
void snapshot_send_makers();
bool snapshot_isfinished();
void snapshot_reset();
void snapshot_clear_channels();
bool snapshot_ismarker(vector_clock_t *v);
void snapshot_tomarker(vector_clock_t *v);
/*----------------------------------SNAPSHOT------------------------------*/


/*----------------------------------THREAD------------------------------*/
void* receiver_thread(void* args);
void 	receiver_thread_snapshot(bool seen_marker, bool ismarker, msg* m);
void* emitter_thread(void* args);
/*----------------------------------THREAD------------------------------*/