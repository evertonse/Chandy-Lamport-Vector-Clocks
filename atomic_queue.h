#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "dunder.h"
#pragma once

#ifndef ATOMIC_QTYPE 
#define ATOMIC_QTYPE void*
#endif


typedef struct _node_t {
	ATOMIC_QTYPE val;
	struct _node_t *next;
} node_t ;


typedef struct _atomic_queue_t {
	node_t *_head, *_tail;
	int _size;
	pthread_mutex_t * _mutex;
	
	bool (*enqueue)(struct _atomic_queue_t *self, ATOMIC_QTYPE val);
	ATOMIC_QTYPE(*dequeue)(struct _atomic_queue_t *self);
	
	//# Dunders
	void 	(*__foreach__)(struct _atomic_queue_t *self, void (*func)(ATOMIC_QTYPE));
	int 	(*__len__)(struct _atomic_queue_t *self);
	void 	(*__del__)(struct _atomic_queue_t *self);
	
	//void (*foreach)(struct _queue *self, void (*func)(ATOMIC_QTYPE));
} atomic_queue_t;

// Constructor

void 				Qforeach	(atomic_queue_t *self, void (*func)(ATOMIC_QTYPE));
ATOMIC_QTYPE 	Qdequeue (atomic_queue_t *self);
bool 			 	Qenqueue (atomic_queue_t *self, ATOMIC_QTYPE val);

int 	Qlen 		(atomic_queue_t *self);
void 	Q__del__ (atomic_queue_t *self);

extern atomic_queue_t 	atomic_queue();