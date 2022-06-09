#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include "dunder.h"

#pragma once

typedef volatile uint32_t vu32;
typedef float	f32;
typedef double f64;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8 ;


typedef int64_t  i64;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8 ;

typedef pthread_cond_t pcond_var;
typedef pthread_mutex_t pmutex;
typedef pthread_t pthread;

/*
	Threads consumidora (Ci) e produtoras (Pi) | i = 1,2,...,n
	n = numero de taredas
	enquanto n = 0, todas Ci wait(cv,mutex)
	quando n = 1, uma das possivel Ci vão executar essa tarefa, apenas um Ci


	cada thread Ci vai ter esse code:
	while n == 0:
		wait
		quando n != 0, saimos do loop
	Ci.get_tarefa()

	Se a fila de tarefas ficar cheia da Pi deve esperar tbm
	no caso da taxa de produção >> taxa de realização da tarefa
	Tarefas = [ o o o o o o o ... ]

	cada thread Pi :
	while n == 10:
		wait(codição que n seja diferente de 10, mutex)
		quando n != 10, saimos dessa porra
	Pi.faça_nova_tarefa()

	>  Quando Ci.get_tarefa, deve dar um signal para a condição do n, deve dizer "OLHA n = n - 1, okay?""

	>  Quando Pi faz nova tarefa, deve sar um signal para condição e dizer "OLHA temos nova tarefa, beleza?
		Isso evita caso alguma thread esperando, pode sair do loop e pegar um tarefa com segurança!


	bloqueia e desbloqueio cruzado
*/

