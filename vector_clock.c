#include "vector_clock.h"
#include <stdio.h>

vector_clock_t vector_clock(i32 comm_sz, i32 rank) {
	vector_clock_t new_vc;
	new_vc.rank 	= rank;
	new_vc.event 	= VC_event;
	new_vc.send 	= VC_send;
	new_vc.receive = VC_receive;
	
	
	new_vc._buffer = (i32*)calloc(comm_sz, sizeof(i32));
	new_vc._size 	= comm_sz;
	new_vc._mutex = (pthread_mutex_t*)malloc(1*sizeof(pthread_mutex_t));
	pthread_mutex_init(new_vc._mutex,NULL);


	new_vc.__len__   	= VC__len__;
	new_vc.__print__ 	= VC__print__;
	new_vc.__copy__ 	= VC__copy__;
	new_vc.__del__ 	= VC__del__;
	new_vc.__str__ 	= VC__str__;
	
	return new_vc;
}

int VC__len__(vector_clock_t *self) {
	return self->_size;
}

void VC__print__(vector_clock_t *self) {
	i32 n = len(self);
	printf("[");
	for (size_t i = 0; i < n-1; i++) {
		printf("%d, ",self->_buffer[i]);
	}
	printf("%d]\n",self->_buffer[n-1]);
}

void VC__copy__(vector_clock_t *source, vector_clock_t *buffer) {
	pthread_mutex_lock(source->_mutex);

	vector_clock_t *self = source;
	buffer->rank 	 = self->rank;
	buffer->event 	 = self->event;
	buffer->send 	 = self->send;
	buffer->receive = self->receive;
	
	buffer->_size 	 = self->_size;
	for (size_t i = 0; i < len(self); i++)
		buffer->_buffer[i] = self->_buffer[i]; 
	
	
	buffer->__len__   = self->__len__;
	buffer->__print__ = self->__print__;
	buffer->__del__ 	= self->__del__;
	buffer->__copy__ 	= self->__copy__;
	buffer->__str__ 	= self->__str__;

	pthread_mutex_unlock(source->_mutex);
}

void VC_receive( vector_clock_t* self,vector_clock_t *other) {
	pthread_mutex_lock(self->_mutex);
	i32 rank = self->rank;
	i32 comm_sz = len(self); // pegamos o comm_size de vc (assumisse que foi inicializado com VC_init)
	i32 *my_vc = self->_buffer, *other_vc = other->_buffer;
	for (i32 i = 0; i < comm_sz; i++) {
		// Se algum valor do outro clock for maior que o nosso
		// atulizamos
		if (other_vc[i] > my_vc[i])
			my_vc[i] = other_vc[i];
	}
	// ao final atulizamos nosso proprio clock
	my_vc[rank]++;
	pthread_mutex_unlock(self->_mutex);
}

void VC_send(vector_clock_t *self) {
	pthread_mutex_lock(self->_mutex);
	// Para send, incrementamos o nosso valor
	i32 *my_vc = self->_buffer;
	i32 rank = self->rank;
	my_vc[rank]++;
	pthread_mutex_unlock(self->_mutex);
}

void VC_event(vector_clock_t *self) {
	pthread_mutex_lock(self->_mutex);
	i32 *my_vc = self->_buffer;
	i32 rank = self->rank;
	// evento interno apenas incrementamos
	my_vc[rank]++;
	pthread_mutex_unlock(self->_mutex);
}

// deleta a memoria alocada
void VC__del__(vector_clock_t *self) {
	pthread_mutex_destroy(self->_mutex); 
	free(self->_buffer);
}

// expects a bufer with len(self)*2  + 2 of char slots;
void VC__str__(vector_clock_t *self, char* buffer) {
	pthread_mutex_lock(self->_mutex);
	buffer[0] = '[';
	int i = 0,j = 1;
	for (i=0; i < len(self)-1; i++)
   	j += sprintf(&buffer[j], "%d, ", self->_buffer[i]);
   j += sprintf(&buffer[j], "%d", self->_buffer[len(self)-1]);
	buffer[j] = ']';
	buffer[j+1] = '\0';
	pthread_mutex_unlock(self->_mutex);
}

// testing the code
// static void 
// _driver_vectorclock_code() {
// 	vector_clock_t vc = vector_clock(20,2);
// 	vc.event(&vc);
// 	printf("%d\n", vc.rank);
// 	print(&vc);
// 	vc.send(&vc);

// 	vector_clock_t other = vector_clock(20,5);
// 	other.send(&other);
// 	print(&other);
	
// 	vc.receive(&vc,&other);
// 	print(&other);
// 	print(&vc);

// 	vector_clock_t copy_other  = vector_clock(20,6);
// 	//char name[200000];
// 	copy(&vc,&copy_other);
// 	//copy(&vc, &copy_other);
// 	printf("copy other: "); print(&copy_other);
// 	char buf[1000];
// 	str(&copy_other,buf);
// 	puts(buf);
//}


// int main() {
// 	_driver_vectorclock_code();
// }