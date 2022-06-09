#include "mpi_main.h" // all typedefs for this file: msg, mutexes, threads, conditional var ...

#define _DEBUG 1
#define FILENAME 			"state.txt"
#define FILENAME_BASIC 	"basic_alg.txt"

// faz todos os debug printar nada
#define debug(sdtr) 
#define debugf(format, ...)
#define debug1(sdtr) 
#define debugf1(format, ...)

#if defined _DEBUG && _DEBUG > 1
	#undef debug1
	#undef debugf1
	#define debug1(str) fprintf (stdout, "(P%d) "str"\n", process_rank)
	#define debugf1(format, ...) fprintf (stdout, "(P%d) "format "\n", process_rank, __VA_ARGS__)
#endif

#if defined _DEBUG && _DEBUG > 0
	#undef debug
	#undef debugf
	#define debug(str) fprintf (stdout, "(P%d) "str"\n", process_rank)
	#define debugf(format, ...) fprintf (stdout, "(P%d) "format "\n", process_rank, __VA_ARGS__)
#endif

/*=====================================GLOBALS=================================*/
i32 	process_rank, finished;

// flags para o algoritimo de snapshot
bool snapshot_clock_ready_flag = false,snapshot_seen_marker, snapshot_finished,snapshot_running;
// estado para uso do snapshot
state_t 			state;

atomic_queue_t 	 receiver_queue, emitter_queue;

// informação sobre o processo
process_info 	self;


pmutex 	 receiver_mutex, emitter_mutex, clock_mutex, snapshot_clock_ready_mutex;

pcond_var 
	emitter_empty , emitter_full ,
	receiver_empty, receiver_full,
	snapshot_clock_ready;

/*=====================================GLOBALS=================================*/

/*>----------------------------------UTILS------------------------------*/
void pthread_wake_all() {
	pthread_cond_broadcast(&emitter_empty);
	pthread_cond_broadcast(&emitter_full);
	pthread_cond_broadcast(&receiver_full);
	pthread_cond_broadcast(&receiver_empty);
	pthread_mutex_unlock(&receiver_mutex);
	pthread_mutex_unlock(&emitter_mutex);
	pthread_mutex_unlock(&clock_mutex);
}

void utils_write(char* filename, char* content) {
	FILE* file;
	file = fopen(filename, "a");
	fputs(content,file);
	fclose(file);
}
void utils_clear_file(char* filename) {
	FILE* file;
	file = fopen(filename, "w");
	fclose(file);
}
/*<----------------------------------UTILS------------------------------*/


/*>----------------------------------PROCESS------------------------------*/
// Representa o processo de rank 0
void process0(i32 comm_sz) {
	// Inicializamos a memoria de vector clock
  
	Event		(0, 	&self.clock);
	Send		(0,1, &self.clock);
	Snapshot	(&self.clock);
	Receive	(0,1, &self.clock);
	Send		(0,2, &self.clock);
	Receive	(0,2, &self.clock);
	Send		(0,1, &self.clock);
	Event		(0, 	&self.clock);

	if(comm_sz > 3){
	Receive	(0,3, &self.clock);
	Send		(0,3, &self.clock);
	}
	debug1("ENDED");
}

// Representa o processo de rank 1
void process1(i32 comm_sz){
	Send		(1,0, &self.clock);
	Receive	(1,0, &self.clock);
	Receive	(1,0, &self.clock);
	debug1("ENDED");
}

// Representa o processo de rank 2
void process2(i32 comm_sz){
	Event		(2, 	&self.clock);
	Send		(2,0, &self.clock);
	Receive	(2,0, &self.clock);
	debug1("ENDED");
}

// Representa o processo de rank 3
void process3(i32 comm_sz){
	Event		(3, 	&self.clock);
	Event		(3, 	&self.clock);
	Event		(3, 	&self.clock);
	Send		(3,0, &self.clock);
	Receive	(3,0, &self.clock);
}
/*<----------------------------------PROCESS------------------------------*/



void Event(int my_rank, vector_clock_t* clock){
	
	pthread_mutex_lock(&clock_mutex);
   
	clock->event(clock);
	char clock_str[500];
	str(clock,clock_str);

	pthread_mutex_unlock(&clock_mutex);
	
	debug1("EVENT");
	char basic_alg_str[1000];
	sprintf(
		basic_alg_str,
		"(P%d) %-8s %-2.2s %-10s %s\n",
		process_rank, "Event", "|", "VectorClock:", clock_str);
	printf(basic_alg_str);
	
	utils_write(FILENAME_BASIC,basic_alg_str);
}

void Send(int my_rank,int other_rank, vector_clock_t* clock) {

	pthread_mutex_lock(&clock_mutex);
	i32 size = len(clock);
	clock->send(clock);
	
	msg* m = malloc(sizeof(msg));
	m->clock 	= vector_clock(size,my_rank);
	copy(clock, &m->clock);
	pthread_mutex_unlock(&clock_mutex);

	m->from 	= my_rank;
	m->to 		= other_rank;
	atomic atom = {
		.mutex	= &emitter_mutex,
		.signal	= &emitter_empty, // signal is not empty
		.wait		= &emitter_full  // wait if full, cant add
	};
	// bota pro emissor
	add_msg(m,&emitter_queue,&atom);
	//ignore this MPI_Send(clock,size,MPI_INT32_T,other_rank,1,MPI_COMM_WORLD);
	
	char clock_str[500],basic_alg_str[1000];
	str(clock,clock_str);
	

	sprintf(basic_alg_str,
		"(P%d) %-8s %-2.2s %-10s %s\n",
		process_rank, "Send", "|", "VectorClock:", clock_str);
	printf(basic_alg_str);
		
	utils_write(FILENAME_BASIC,basic_alg_str);

	
}


void Receive(int my_rank,int other_rank, vector_clock_t* clock) {
	atomic atom = {
		.mutex	= &receiver_mutex,
		.signal	= &receiver_full, // signal is not full
		.wait		= &receiver_empty // wait if empty
	};

	// Pegamos a msg do receiver
	msg* m = get_msg(&receiver_queue, &atom);

/*
	Se messagem 'm' é um <marker>	: 
	Devemos sinalizar ao 'receiver_thread' que todos os 
	relogios já estão ATUALIZADOS para fazer o snapshot
*/
	if (snapshot_ismarker(&m->clock)) {
		debug("<Receive> got a <marker> and its signaling 'snapshot_clock_ready'");
		//snapshot_clock_ready_flag = true;
		//pthread_cond_signal(&snapshot_clock_ready);
		
		// se recebemos um marker, isso quer dizer que é aprimeira vez
		// então devemos puxar mais outra msg e ignorar essa;
		//Receive(my_rank,other_rank, clock);
		return;
	}

	pthread_mutex_lock(&clock_mutex);
	clock->receive(clock, &m->clock);
	pthread_mutex_unlock(&clock_mutex);
	
	
	del(&m->clock); 	// chama a limpeza do vector_clock
	free(m); 			// libera memória da msg que fora alocada em outro lugar  
	
	
	char clock_str[500],basic_alg_str[1000];
	str(clock,clock_str); // create a str of clock from buffer clock_str;
	
	sprintf(
		basic_alg_str,
		"(P%d) %-8s %-2.2s %-10s %s\n",
		self.rank, "Receive", "|", "VectorClock:", clock_str);
	printf(basic_alg_str);

	utils_write(FILENAME_BASIC,basic_alg_str);

}

void Snapshot(vector_clock_t* clock) {
	// Save proprio estado.
	pthread_mutex_lock(&clock_mutex);
	copy(clock,&state.clock);


	// flag que um <marker> foi visto
	snapshot_seen_marker = true;
	snapshot_finished 	= false;
	snapshot_running 		= true;
	// marcamos que o canal 
	state.channel_done[self.rank] = true;

	snapshot_send_makers();
	
	char clock_str[500],basic_alg_str[1000];
	str(clock,clock_str); // create a str of clock from buffer clock_str;
	
	sprintf(
		basic_alg_str,
		"(P%d) %-8s %-2.2s %-10s %s\n",
		self.rank, "Snapshot", "|", "VectorClock:", clock_str);
	
	printf(basic_alg_str);
	utils_write(FILENAME_BASIC,basic_alg_str);
	pthread_mutex_unlock(&clock_mutex);
	
}


/*>----------------------------------QUEUE------------------------------*/
ATOMIC_QTYPE get_msg(atomic_queue_t* queue, atomic* atom)
{
// begin atomic
	pthread_mutex_lock(atom->mutex);	
	while (len(queue) == 0)
	{
		if(finished) {
			pthread_mutex_unlock(atom->mutex);
			return NULL;
		}

		debug1("<get_msg> while (len(queue) == 0)");
		pthread_cond_wait(atom->wait,atom->mutex);
	}
	
	msg* m;
	// retira o primeiro elemento da fila
	m = queue->dequeue(queue);
	//debug1("MESSAGE FROM QUEUE WAS NULL");

	debugf1(
		"<get_msg> just FUCKING GOT IT msg [from:%d, to:%d, clock:%p, ] "
		"len(queue)=%d",
		m->from, m->to, m->clock._buffer,
		len(queue)
	);
		
	// signal que não está full, caso alguém tivesse perando isso
	pthread_cond_signal(atom->signal);
	
	pthread_mutex_unlock(atom->mutex);
// end
	return m;
}

void add_msg(ATOMIC_QTYPE new_msg, atomic_queue_t* queue, atomic* atom)
{
	// outras Threads não podem mecher na nossa fila
	pthread_mutex_lock(atom->mutex);	

	// se tiver no total permitido de tarefas trancamos a produção
	while (len(queue) == TASKS_SIZE)
	{
		if(finished){
			pthread_mutex_unlock(atom->mutex);
			return;
		}

		debug ("<add_msg> while (len(queue) == TASKS_SIZE)");
			pthread_cond_wait(atom->wait,atom->mutex);
	} 

	// adiciona nova tarefa à fila
	queue->enqueue(queue, new_msg); 
	debugf1(
		"<add_msg> just enqueue new msg [from:%d, to:%d, clock:%p, ] "
		"len(queue)=%d",
		new_msg->from, new_msg->to, new_msg->clock._buffer,
		len(queue)
	);

	pthread_mutex_unlock(atom->mutex);


	// signal que não está vazio mais
	pthread_cond_signal(atom->signal); 
}
/*<----------------------------------QUEUE------------------------------*/


/*>----------------------------------SNAPSHOT------------------------------*/

bool snapshot_ismarker(vector_clock_t *v) {
	return v->_buffer[0] == -1;
}

void snapshot_tomarker(vector_clock_t *v) {
	v->_buffer[0] = -1;
}

// Envia <maker> para todos os canais que estão abertos
void snapshot_send_makers() {
	
	snapshot_seen_marker = true;
	atomic atom = {
		.mutex	= &emitter_mutex,
		.signal	= &emitter_empty, // signal is not empty
		.wait		= &emitter_full  	// wait if full, cant add
	};

	// Para cada Processo envie um <marker>
	for (i32 i = 0; i < self.comm_sz; i++) {
		if (i == self.rank){
			continue;
		}
		
		// Memoria deve ser liberada pelo usuario final dessa msg;
		msg *m = (msg*)malloc(sizeof(msg)); 
		m->from = self.rank; m->to = i;

		m->clock = vector_clock(self.comm_sz,self.rank); 
		snapshot_tomarker(&m->clock); // transform to marker
		
		// adicionamos a fila para envio
		add_msg(m,&emitter_queue, &atom);
		debugf(
			"-snapshot- added <marker> from:%d to:%d ismaker:%s",
			m->from,m->to,snapshot_ismarker(&m->clock)?"true":"false");
	}
}

bool snapshot_isfinished() {
	// se ja foi confirmado antes que acabara
	// não calculamos novamente
	// checamos se todos os estados dos canais já foram encerrados
	for (i32 i = 0; i < state.channels_sz; i++) {
		if(state.channel_done[i] == false){
			debug1("-snapshot- is not finished");
			return false;
		}
	}

	debug("-snapshot- is finished");
	// printamos quantas msg tem em cada canal em cada processo
	for (i32 i = 0; i < state.channels_sz; i++) {
		
		
		debugf(
			"channel[%d<-%d] final state nº msg's:%d ",
			self.rank,i, len(&state.channels[i]));
	}

	snapshot_finished = true;
	return true;
}

void snapshot_reset() {
	snapshot_seen_marker = false;
	snapshot_finished 	= false;

	for (size_t i = 0; i < state.channels_sz; i++)
	{	
		state.channel_done[i] = false;
		debugf1(
			" snapshot_running:%s"
			" snapshot_seen_marker:%s"
			, snapshot_running?"true":"false",
			snapshot_seen_marker?"true":"false");
	}
	
}

void snapshot_clear_channels() {
	char process_state_buff[250],msg_buff[1000],clock_buff[50];
	
	str(&state.clock,clock_buff);
	sprintf( process_state_buff,"(P%d) state:%s\n",
			self.rank,clock_buff);
	utils_write(FILENAME, process_state_buff);

	for (i32 i = 0; i < state.channels_sz; i++)
	{	
		sprintf(
			msg_buff,
			"channel[%d<-%d] final state nº msg's:%d \n",
			self.rank,i, len(&state.channels[i]));
		
		if ( i != self.rank )
			utils_write(FILENAME, msg_buff);
		
		atomic_queue_t* q = &state.channels[i];
		for (i32 j = 0; j < len(q); j++)
		{
			msg* m = q->dequeue(q);
			
			char clock_buff[50];
			str(&m->clock,clock_buff);
			
			sprintf(
				msg_buff, "\tmsg[%d] from:%d to:%d clock:%s\n",
				j,m->from,m->to, clock_buff);
			utils_write(FILENAME, msg_buff);
			
			free_msg(m);
		}
		debugf1(
			"<snapshot_reset> cleared queue[%d],"
			" snapshot_running:%s"
			" snapshot_seen_marker:%s"
			,i, snapshot_running?"true":"false",
			snapshot_seen_marker?"true":"false");
	}
	
}
/*<----------------------------------SNAPSHOT------------------------------*/


/*>----------------------------------THREAD------------------------------*/
// Thread encarregada de enviar msg por MPI
void* emitter_thread(void* args)
{
	//i32 id = *(i32*)args;
	debug1("<emitter_thread> started");
	while (true && !finished)
	{

		//sleep(2); // simular delay
		
		atomic atom = {
			.mutex	=&emitter_mutex,
			.signal	=&emitter_full, 	// signal is not full anymore
			.wait		=&emitter_empty 	// wait if is get. [cant get if queue is empty]
		};
	
		debugf1( "<emmitter_thread> waiting to get msg, len(&emitter_queue) = %d\n",
			len(&emitter_queue));

		msg* new_msg = get_msg(&emitter_queue,&atom);
		// se retornar null, o programa acabou
		if (new_msg == NULL)
			break;
		
		debugf1(
			"<emmitter_thread> was able to get the msg, len(&emitter_queue) = %d\n [from:%d, to:%d, clock:%p, ] ",
			len(&emitter_queue),
			new_msg->from, new_msg->to, new_msg->clock._buffer);
		
		MPI_Send(
			new_msg->clock._buffer,  len(&new_msg->clock),MPI_INT32_T,
			new_msg->to, MPI_TAG, MPI_COMM_WORLD);
		
		debugf1( "<emmitter_thread>  send the msg, now len(&emitter_queue) =  = %d\n",
			len(&emitter_queue));
		
		del(&new_msg->clock); 	// destroy vector_clock
		free(new_msg);				// libera memória alocada para msg
	}
	debug1("<emmitter_thread>  exit");
	return NULL;
}


// Thread encarregadas de escudar todos os canais do MPI e botar msgs numa fila atomica
void* receiver_thread(void* args) /* type(args) = int[]*/
{

	i32 rank 	= ((i32*)args)[0];
	i32 comm_sz = ((i32*)args)[1];
	i32 has_message = false;
	debug1("<receiver_thread> started");

	srand(time(NULL)); 
	while(true/*theres a msg from MPI*/ && !finished)
	{
		sleep(rand() % 2); // criar delay artificial

		// vector_clock DEVE ter sua memoria liberada pelo usuário final
		MPI_Status stats;

		// Checka se tem mensagem, para evitar bloquear
		MPI_Iprobe(MPI_ANY_SOURCE,MPI_TAG,MPI_COMM_WORLD,&has_message,&stats);
		
		if ( has_message && !finished) {
			// init buffer para receber MPI receive
			vector_clock_t buffer_clock = vector_clock(comm_sz, rank);
			debugf1("<receiver_thread> created buffer_clock, len(&receiver_queue)=%d\n",
				len(&receiver_queue));

			// Receive de qualquer source
			MPI_Recv(
				buffer_clock._buffer,comm_sz,
				MPI_INT32_T,MPI_ANY_SOURCE,
				MPI_TAG,MPI_COMM_WORLD,
				&stats);

			// memoria deve ser liberada pelo usuario final dessa msg;
			msg *m = (msg*)malloc(sizeof(msg)); 
			m->clock = buffer_clock; m->from = stats.MPI_SOURCE; m->to = rank;
			
			// procedimentos do snapshot
			debugf1(
			"<receiver_thread> has_message:%d, finished:%d\n",
			has_message,finished);

			receiver_thread_snapshot(snapshot_seen_marker,snapshot_ismarker(&m->clock),m);
		}
	}
	debug1("<receiver_thread> exit");
	return NULL;
}


// Função que sincroniza
void receiver_thread_snapshot(bool seen_marker, bool ismarker, msg* m) {
/* 
	Possibilidades snapshot:
		i)  	<marker> já foi visto  		&& <msg> is NOT <marker>
			- salve relogio
			- marque o canal que recebeu como finalizado
			- mande <marker> a todos
		ii)  	<marker> já foi visto		&& <msg> is <marker>
		iii) 	<marker> NUNCA foi visto 	&& <msg> is <marker>
		iv)  	<marker> NUNCA foi visto  	&& <msg> is NOT <marker>
		 
*/
	atomic atom = {
		.mutex	= &receiver_mutex,
		.signal	= &receiver_empty, 	// signal is not empty
		.wait		= &receiver_full 		// wait if full
	};

	
	debugf1("<receiver_thread_snapshot> have already seen marker = %s", seen_marker?"true":"false" );
	debugf1("<receiver_thread_snapshot> msg is %s <marker>",ismarker?"":"not");
	debugf1("<receiver_thread_snapshot> channel is %s done", state.channel_done[m->from]?"":"not");

	// CASO i)
	// Se canal já esta fechado essa msg não faz parte do snapshot 
	if (seen_marker && ismarker == false && state.channel_done[m->from] == false) 
	{	
		// Se a mensagem NÂO é um <marker> e o canal ainda estiver aberto
		// então sim, adicionamos essa mensagem ao estado do snapshot desse canal
		msg *msg_channel = (msg*)malloc(sizeof(msg)); 
		msg_channel->from = m->from;
		msg_channel->to 	= m->to;
		msg_channel->clock = vector_clock(self.comm_sz,self.rank);

		debug1("<receiver_thread_snapshot> about do copy");
		copy(&m->clock, &msg_channel->clock);
			
			
		// adiciona a copia ao canal
		debugf("<receiver_thread_snapshot> enqueing msg as state in channel[%d]", m->from);
		state.channels[msg_channel->from].enqueue(&state.channels[msg_channel->from],msg_channel);
		add_msg(m,&receiver_queue,&atom);
	}
	// CASO ii)
	else if (seen_marker && ismarker) 
	{
		// Paramos de gravar nesse canal
		state.channel_done[m->from] = true;
		debugf1(
			"<receiver_thread_snapshot> it's a <marker> set channel_done[%d]:%s",
		 	m->from, state.channel_done[m->from]?"true":"false");
		free_msg(m);
	}
	// CASO iii)
	else if(seen_marker == false && ismarker){
		// add uma msg que é um <marker> para receber confirmação que 
		// algoritimo basico já processou até todos os relogio antes de 
		// fazermos o snapshot
		snapshot_running = true;
	
		
		// Salve proprio estado.
		copy(&self.clock,&state.clock);
		debug("<receiver_thread_snapshot> has saved it's own state, 'snapshot_clock_ready' was signaled ");
		
		// flag que um <marker> foi visto
		snapshot_seen_marker = true;

		// marcamos que o nosso canal e o que recebemos está concluido 
		state.channel_done[self.rank] = true;
		state.channel_done[m->from] = true;

		// enviamos <marker> para todos os outros canais
		snapshot_send_makers();
		
	}
	// CASO iv)
	else {
		add_msg(m,&receiver_queue,&atom);
	}
	// Chegamos se o snapshot terminou localmente
	if (snapshot_running && snapshot_isfinished()) {
		debug1("snapshot_running && snapshot_isfinished()");
		// se sim resetamos as variaveis do alg.
		snapshot_running = false;
		snapshot_seen_marker = false;
		snapshot_reset();
		snapshot_clear_channels();
	}

}
/*<----------------------------------THREAD------------------------------*/



void free_msg(msg* m) {
	del(&m->clock);
	free(m);
}

void clear_queue(atomic_queue_t* q) {
	for (size_t i = 0; i < len(q); i++)
	{
		msg* m = q->dequeue(q);
		free_msg(m);
	}
}


/*=====================================THREAD FUNCTIONS=================================*/

/*-------------------------------MAIN-------------------------------------*/
// Função principal do mpi para ser passada para mpi_with
void mpi_main(i32 rank, i32 comm_sz)
{
	process_rank = rank;
	finished = 0;
	utils_clear_file(FILENAME);
	utils_clear_file(FILENAME_BASIC);
	pthread_mutex_init(&clock_mutex,NULL);
	// dynamic init das variaveis de controle
	// RECEIVER
	pthread_mutex_init(&receiver_mutex,NULL);
	pthread_cond_init (&receiver_empty,NULL);
	pthread_cond_init (&receiver_full,NULL);
	receiver_queue = atomic_queue(); 
	
	debugf1("<main> len(&receiver_queue)=%d",len(&receiver_queue));
	
	// EMITTER
	pthread_mutex_init(&emitter_mutex,NULL);
	pthread_cond_init (&emitter_empty,NULL);
	pthread_cond_init (&emitter_full,NULL);
	emitter_queue = atomic_queue(); 
	debugf1("<main> len(&emitter_queue)=%d",len(&emitter_queue));

	// SNAPSHOT
	pthread_mutex_init(&snapshot_clock_ready_mutex,NULL);
	pthread_cond_init (&snapshot_clock_ready,NULL);
	// criamos um clock que faz parte do estado a ser salvo pelo snapshot
	state.clock = vector_clock(comm_sz,process_rank);
	// criamos uma fila para cada canal de entrada
	state.channels 	= malloc(comm_sz*sizeof(atomic_queue_t));
	state.channels_sz 	= comm_sz;
	for (size_t i = 0; i < state.channels_sz; i++) {
		state.channels[i] = atomic_queue();
	}
	
	// criamos um array de flags para indicar se o channel terminou
	state.channel_done = malloc(comm_sz*sizeof(bool));
	memset(state.channel_done,false,comm_sz*sizeof(bool));
	
	// info sobre este processo
 	self.clock 		= vector_clock(comm_sz,process_rank);
	self.comm_sz	= comm_sz;
	self.rank 		= rank;

	// RECEIVER thread init, será produtora da fila
	pthread receiver;
	i32 receiver_args[2] = {rank,comm_sz}, fail;
	fail = pthread_create(
			&receiver,
			0,receiver_thread,
			receiver_args);
	if (fail) 
		perror("receiver failed to init");

	// EMITTER thread init, será consumidora da fila
	pthread emitter;
	fail = pthread_create(
			&emitter,
			0,emitter_thread,
			NULL);
	if (fail) 
		perror("emitter failed to init");
	

	if (rank == 0)
		process0(comm_sz);
	else if (rank == 1)  
		process1(comm_sz);
	else if (rank == 2)
		process2(comm_sz);
	else if (rank == 3)
		process3(comm_sz);
	
	del(&self.clock);
	//finished = 1;

	pthread_wake_all();
	
	pthread_join(receiver,NULL);
	debug1("<main> joined receiver ");
	pthread_join(emitter,NULL);
	debug1("<main> joined emitter ");

	debugf1("<main> has returned from <process%d> ",process_rank);
	// RECEIVER
	pthread_mutex_destroy(&receiver_mutex);
	debug1("<main> destroyed receiver_mutex");
	pthread_cond_destroy (&receiver_empty);
	debug1("<main> destroyed receiver_empty");
	pthread_cond_destroy (&receiver_full);
	debug1("<main> destroyed receiver_full");
	
	// EMITTER
	pthread_mutex_destroy(&emitter_mutex);
	debug1("<main> destroyed emitter_mutex");
	pthread_cond_destroy (&emitter_empty);
	debug1("<main> destroyed emitter_empty");
	pthread_cond_destroy (&emitter_full);
	debug1("<main> destroyed emitter_full");

	//SNAPSHOT
	pthread_cond_destroy (&snapshot_clock_ready);
	pthread_mutex_destroy (&snapshot_clock_ready_mutex);

	debug1("<main> exit");
	return;
}
/*-------------------------------MAIN-------------------------------------*/
