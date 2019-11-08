#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "rthread.h"
#define MIDDLE 0
#define HIGH 1
#define NLANES 7
#define NMIDDLE 7
#define NHIGH 7
#define NEXPERIMENTS 5


struct pool {
	rthread_lock_t lock;
	//make an array of structs
	struct {
		int type; //whether this type is middle or high 
		rthread_cv_t cv; //each swimmer gets their own conditional variable
	} swimmers[NMIDDLE + NHIGH];
	int nHighEntered, nMiddleEntered;
	int nMiddleWaiting, nHighWaiting;
	int front_index;
	int back_index;
};

void pool_init(struct pool *pool){
	memset(pool, 0, sizeof(*pool));
	rthread_lock_init(&pool->lock);
	// initialize your monitor variables here
	//initalize each conditional variable in struct array
	for( int i = 0; i < NMIDDLE + NHIGH; i++){
		rthread_cv_init(&pool->swimmers[i].cv, &pool->lock);
		pool->swimmers[i].type = -1;
	}
	pool->front_index = 0; 
	pool->back_index = 0;
	pool->nHighEntered = pool->nMiddleEntered = 0;
	pool->nMiddleWaiting = pool->nHighWaiting = 0;
}

int default_index = NMIDDLE*600 + NHIGH*600;
void pool_enter(struct pool *pool, int level){
	int index = default_index;
	rthread_with(&pool->lock) {
		if (level == 0){ //corressponds to middle school
			pool->swimmers[pool->front_index % 14].type = 0;
			while(!((pool->nHighEntered == 0 && pool->nHighWaiting == 0) || index < pool->front_index)){
				if(index != default_index){ //spurious wake up
					rthread_cv_wait(&pool->swimmers[pool->front_index % 14].cv);
				}
				else{
					index = pool->back_index;
					pool->back_index++;
					pool->nMiddleWaiting++;
					rthread_cv_wait(&pool->swimmers[pool->front_index % 14].cv);
				}
			}
			if(index != default_index){	//had to go through the while loop		
				pool->nMiddleWaiting--;
			}
			pool->nMiddleEntered++;
			pool->swimmers[pool->front_index % 14].type = -1;
		}
		else if (level == 1){
			pool->swimmers[pool->front_index % 14].type = 1;
			while(!((pool->nMiddleEntered == 0 && pool->nMiddleWaiting == 0) || index < pool->front_index)){
				if(index != default_index){
					rthread_cv_wait(&pool->swimmers[pool->front_index % 14].cv);
				}
				else{
					index = pool->back_index;
					pool->back_index++;
					pool->nHighWaiting++;
					rthread_cv_wait(&pool->swimmers[pool->front_index % 14].cv); //overflow!!! mod with 14
				}
			}
			if(index != default_index){			
				pool->nHighWaiting--;
			}
			pool->nHighEntered++;
			pool->swimmers[pool->front_index % 14].type = -1;
		}
		else{
			printf("level is not a valid parameter value (0 or 1)\n");
		}
	}
}

void pool_exit(struct pool *pool, int level){
	rthread_with(&pool->lock) {
		if (level == 0){
			pool->nMiddleEntered--;
			if (pool->front_index == pool->back_index || pool->nMiddleEntered > 0){ //no one waiting, just leave 
				return;
			}
			if (pool->nMiddleEntered == 0 && pool->swimmers[pool->front_index % 14].type == 1){ //if no middle in the pool and next is high 
				while(pool->swimmers[pool->front_index % 14].type == 1){
					rthread_cv_notify(&pool->swimmers[pool->front_index % 14].cv);
					pool->front_index++;
				}
			}

		}
		else if (level == 1){
			pool->nHighEntered--;
			if (pool->front_index == pool->back_index || pool->nHighEntered > 0){ //no one waiting, just leave 
				return;
			}
			if (pool->nHighEntered == 0 && pool->swimmers[pool->front_index % 14].type == 0){
				while(pool->swimmers[pool->front_index % 14].type == 0){
					rthread_cv_notify(&pool->swimmers[pool->front_index % 14].cv);
					pool->front_index++;
				}
			}		

		}
		else{
			printf("level is not a valid parameter value (0 or 1)\n");			
		}		
	}
}


char *middle[] = {
	"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9"
};
char *high[] = {
	"h0", "h1", "h2", "h3", "h4", "h5", "h6", "h7", "h8", "h9"
};

void swimmer(void *shared, void *arg){
	struct pool *pool = (struct pool *) shared;
	char *name = (char *) arg;
	for (int i = 0; i < NEXPERIMENTS; i++) {
		rthread_delay(random() % 1000);
		printf("swimmer %s entering pool\n", name);
		pool_enter(pool, *name == 'h');
		printf("swimmer %s entered pool\n", name);
		rthread_delay(random() % 1000);
		printf("swimmer %s leaving pool\n", name);
		pool_exit(pool, *name == 'h');
		printf("swimmer %s left pool\n", name);
	}
}

int main(){
	struct pool pool;
	pool_init(&pool);
	for (int i = 0; i < NMIDDLE; i++) {
		rthread_create(swimmer, &pool, middle[i]);
	}
	for (int i = 0; i < NHIGH; i++) {
		rthread_create(swimmer, &pool, high[i]);
	}
	rthread_run();
	return 0;
}
