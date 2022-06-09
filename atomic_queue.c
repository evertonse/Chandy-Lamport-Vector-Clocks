#include "atomic_queue.h"

atomic_queue_t atomic_queue() {
	atomic_queue_t q = {._head = NULL, ._tail = NULL};
	q._mutex = (pthread_mutex_t*)malloc(1*sizeof(pthread_mutex_t));
	pthread_mutex_init(q._mutex,NULL);
	q._size 			= 0;
	q.dequeue 		= Qdequeue;
	q.enqueue 		= Qenqueue;
	q.__foreach__ 	= Qforeach;
	q.__len__ 		= Qlen;
	q.__del__ 		= Q__del__;
	return q;
}

bool Qenqueue (atomic_queue_t *self, ATOMIC_QTYPE val) {
	pthread_mutex_lock(self->_mutex);

	node_t* n 	= malloc(sizeof(node_t));

	//printf("node:%p\n",(void*)n);

	if (n ==  NULL){
		pthread_mutex_unlock(self->_mutex);
		return false;
	}
	n->val 	= val;
	
		
	n->next 	= NULL;
	
	// if we have a last item, we append new node_t to tail
	if(self->_tail != NULL) {
		self->_tail->next = n;
	}
	// we update the new end of the line, tail is now the new node.
	self->_tail = n;

	// the queue could have been previously empty
	if(self->_head == NULL){
		// if thats the case we should make this node_t
		// the head as well as the tail, (we only have one item)
		self->_head = n;
	}
	self->_size++;
	
	pthread_mutex_unlock(self->_mutex);
	return true;
}

ATOMIC_QTYPE Qdequeue (atomic_queue_t *self){
	if (self->_head == NULL) {
		printf("Trying to dequeue an empty queue. self: %p, head: %p, tail: %p \n", (void*)self,(void*)self->_head,(void*)self->_tail);
		return (ATOMIC_QTYPE)NULL;
	}
	pthread_mutex_lock(self->_mutex);
	// pop the head of queue
	node_t* tmp = self->_head;
	// get the value of interest
	ATOMIC_QTYPE result = tmp->val;
	// set the new head to be the next in line
	self->_head = self->_head->next;
	// if there's no more items, we need to make sure the
	// tail is NULL as well. only because 
	if (self->_head == NULL){
		self->_tail = NULL;
	}
	free(tmp);
	self->_size--;
	pthread_mutex_unlock(self->_mutex);
	return result;
}

int Qlen (atomic_queue_t *self) {
	return self->_size;
}

void Qforeach(atomic_queue_t *self, void (*func)(ATOMIC_QTYPE)){

	if (self == NULL){
		return;
	}
	pthread_mutex_lock(self->_mutex);
	
	node_t *current = self->_head , *next;
	while (current != NULL){
		next = current->next;
		func(current->val);
		current = next;
	}
	pthread_mutex_unlock(self->_mutex);
}

void Q__del__(atomic_queue_t *self){

	if (self == NULL){
		return;
	}

	pthread_mutex_lock(self->_mutex);
	
	node_t *current = self->_head , *next;
	while (current != NULL){
		next = current->next;
		free(current);
		current = next;
	}
	free(self->_mutex);
	pthread_mutex_unlock(self->_mutex);
}

// static void __print__(ATOMIC_QTYPE item) {
// 	printf("queue ->: %p\n", item);
// }

// int main(int argc, char const *argv[])
// {
// 	_driver_queue_code1();
// 	return 0;
// }
