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
	//make array of condition variables
	rthread_cv_t cv_array[NMIDDLE + NHIGH];
	rthread_cv_t middle;
	rthread_cv_t high;
	int index;
	int nHighEntered, nMiddleEntered;
};

void pool_init(struct pool *pool){
	memset(pool, 0, sizeof(*pool));
	rthread_lock_init(&pool->lock);
	// initialize your monitor variables here
	rthread_cv_init(&pool->middle, &pool->lock);
	rthread_cv_init(&pool->high, &pool->lock);
	rthread_cv_init(&pool->cv_array, &pool->lock); //am i initializing this correctly 
	pool->index = 0;
	pool->nHighEntered = pool->nMiddleEntered = 0;
}

void pool_enter(struct pool *pool, int level){
	rthread_with(&pool->lock) {
		if (level == 0){ //corressponds to middle school
			while(pool->nHighEntered > 0){
				pool->cv_array[pool->index + 1] = pool->middle; //the first index does not get allocated 
				rthread_cv_wait(&pool->cv_array[pool->index]);
				pool->nMiddleEntered++;
				pool->index++;		
			}
		}
		else if (level == 1){
			while(pool->nMiddleEntered > 0){
				pool->cv_array[pool->index + 1] = pool->high;
				rthread_cv_wait(&pool->cv_array[pool->index]);
				pool->nHighEntered++;
				pool->index++;
			}
		}
		else{
			printf("level is not a valid parameter value (0 or 1)\n");
		}
	}
}

void pool_exit(struct pool *pool, int level){
	rthread_with(&pool->lock) {
		for(int i = 0; i < NMIDDLE + NHIGH; i++){
			rthread_cv_notify(&pool->cv_array[i + 1]);
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
