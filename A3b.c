#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "rthread.h"
#define MIDDLE 0
#define HIGH 1
#define NLANES 3
#define NMIDDLE 3
#define NHIGH 3
#define NEXPERIMENTS 2


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
		//printf("high entered %d\n", pool->nHighEntered);
		//printf("middle entered %d\n", pool->nMiddleEntered);

		if (level == 0){ //corresponds to middle school
			while(!((pool->nHighEntered == 0 && pool->nHighWaiting == 0) || index < pool->front_index)){ //less than or equal to?
				if(index != default_index){ //spurious wake up
					rthread_cv_wait(&pool->swimmers[(index) % (NMIDDLE + NHIGH)].cv);
				}
				else{
					pool->swimmers[pool->back_index % (NMIDDLE + NHIGH)].type = 0;
					index = pool->back_index;
					pool->back_index++;
					pool->nMiddleWaiting++;
					rthread_cv_wait(&pool->swimmers[(pool->back_index) % (NMIDDLE + NHIGH)].cv);
				}
			}
			if(index != default_index){	//had to go through the while loop		
				pool->nMiddleWaiting--;
				pool->front_index++;
				pool->swimmers[(index) % (NMIDDLE + NHIGH)].type = -1;

			}
			pool->nMiddleEntered++;


		}
		else if (level == 1){
			while(!((pool->nMiddleEntered == 0 && pool->nMiddleWaiting == 0) || index < pool->front_index)){
				if(index != default_index){
					rthread_cv_wait(&pool->swimmers[(index) % (NMIDDLE + NHIGH)].cv);
				}
				else{
					pool->swimmers[pool->back_index % (NMIDDLE + NHIGH)].type = 1;
					index = pool->back_index;
					pool->back_index++;
					pool->nHighWaiting++;
					rthread_cv_wait(&pool->swimmers[(pool->back_index) % (NMIDDLE + NHIGH)].cv);
				}
			}
			if(index != default_index){			
				pool->nHighWaiting--;
				pool->front_index++;
				pool->swimmers[(index) % (NMIDDLE + NHIGH)].type = -1;
			}
			pool->nHighEntered++;
		}
		else{
			printf("level is not a valid parameter value (0 or 1)\n");
		}
	}
}

void pool_exit(struct pool *pool, int level){
	rthread_with(&pool->lock) {
		printf("this is front index %d\n", pool->front_index);
		printf("this is back index %d\n", pool->back_index);
		for(int i = 0; i < NHIGH + NMIDDLE; i++){
			printf("%d", pool->swimmers[i].type);
		}

		if (level == 0){
			pool->nMiddleEntered--;
			if (pool->front_index == pool->back_index || pool->nMiddleEntered > 0){ //no one waiting, just leave 
			}
			else if (pool->nMiddleEntered == 0){ 
				printf("middle enetere d%d\n", pool->nMiddleEntered);
				int count = pool->front_index;
				if(pool->swimmers[count % (NMIDDLE + NHIGH)].type == 0){ printf("yes\n");}
				while(pool->swimmers[count % (NMIDDLE + NHIGH)].type == 1){
					rthread_cv_notify(&pool->swimmers[(count) % (NMIDDLE + NHIGH)].cv);
					count++;
				}
			}

		}
		else if (level == 1){
			pool->nHighEntered--;
			if (pool->front_index == pool->back_index || pool->nHighEntered > 0){ //no one waiting, just leave 
			}
			else if (pool->nHighEntered == 0 ){
				printf("high enetere d%d\n", pool->nHighEntered);
				int count = pool->front_index;
				if(pool->swimmers[count % (NMIDDLE + NHIGH)].type == 0){ printf("yes\n");}
				while(pool->swimmers[count % (NMIDDLE + NHIGH)].type == 0){
					rthread_cv_notify(&pool->swimmers[(count) % (NMIDDLE + NHIGH)].cv);
					count++;
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
