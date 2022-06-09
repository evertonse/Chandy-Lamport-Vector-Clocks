#ifndef DUNDER

#define DUNDER ;
#define print(obj) 			(obj)->__print__((obj))
#define foreach(obj,fn) 	(obj)->__foreach__((obj), (fn))
#define len(obj) 				(obj)->__len__((obj))
#define str(obj,buffer) 	(obj)->__str__((obj),(buffer))
#define del(obj) 				(obj)->__del__((obj))
#define copy(source, dest) (source)->__copy__((source), (dest))

#endif