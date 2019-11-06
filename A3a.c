#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "rthread.h"
#define MIDDLE 0
#define HIGH 1
#define NLANES 7

struct pool {
	rthread_lock_t lock;
	// you can add more monitor variables here
	rthread_cv_t high;
	rthread_cv_t middle; 
	 // Variables to keep track of the state of the pool
	int nHighEntered, nHighWaiting;
	int nMiddleEntered, nMiddleWaiting;
};

void pool_init(struct pool *pool){
	memset(pool, 0, sizeof(*pool));
	rthread_lock_init(&pool->lock);
	// initialize your monitor variables here
	rthread_cv_init(&pool->high, &pool->lock);
	rthread_cv_init(&pool->middle, &pool->lock);
	pool->nHighEntered = pool->nHighWaiting = 0;
	pool->nMiddleEntered = pool->nMiddleWaiting = 0;
}

void pool_enter(struct pool *pool, int level){
	rthread_with(&pool->lock) {
	// write the code here to enter the pool
		if (level == 0){ //corressponds to middle school
			assert(pool->nHighEntered == 0 || (pool->nHighEntered > 0 && pool->nMiddleEntered == 0));
			while (pool->nHighEntered > 0 || pool->nMiddleEntered > 7){
				pool->nMiddleWaiting++;
				rthread_cv_wait(&pool->middle); //decrement waiting once you leave thu
				pool->nMiddleWaiting--;
			}
			assert(pool->nHighEntered == 0 || pool->nMiddleEntered < 7);
			pool->nMiddleEntered++;
		}
		else if (level == 1){ //high school
			assert(pool->nMiddleEntered == 0 || (pool->nMiddleEntered > 0 && pool->nHighEntered == 0));
			while (pool->nMiddleEntered > 0 || pool->nHighEntered > 7){
				pool->nHighWaiting++;
				rthread_cv_wait(&pool->high);
				pool->nHighWaiting--;

			}
			assert(pool->nMiddleEntered == 0 || pool->nHighEntered < 7);
			pool->nHighEntered++;

		}
		else{
			printf("level is not a valid parameter value (0 or 1)\n");
		}
	}
}

void pool_exit(struct pool *pool, int level){
	rthread_with(&pool->lock) {
		// write the code here to exit the pool
		if (level == 0){ 
			assert(pool->nHighEntered == 0);
			assert(pool->nMiddleEntered > 0);
			pool->nMiddleEntered--;
			if(pool->nMiddleWaiting > 0){
				rthread_cv_notify(&pool->middle);
			}
			else if(pool->nMiddleEntered == 0){ //if no one is the pool and both waiting, give priority to the other team
				rthread_cv_notifyAll(&pool->high);
			}
		}
		else if (level == 1){
			assert(pool->nMiddleEntered == 0);
			assert(pool->nHighEntered > 0);
			pool->nHighEntered--;
			if(pool->nHighWaiting > 0 ){
				rthread_cv_notify(&pool->high);
			}
			else if(pool->nHighEntered == 0){
				rthread_cv_notifyAll(&pool->middle);
			}

		}
		else{
			printf("level is not a valid parameter value (0 or 1)\n");			
		}
	}
}

#define NMIDDLE 10
#define NHIGH 10
#define NEXPERIMENTS 5

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