#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "rthread.h"
#define MIDDLE 0
#define HIGH 1
#define NLANES 7
#define NMIDDLE 10
#define NHIGH 10
#define NEXPERIMENTS 5

struct pool {
	rthread_lock_t lock;
	rthread_cv_t high;
	rthread_cv_t middle; 
	 // Variables to keep track of the state of the pool
	int nHighEntered, nHighWaiting;
	int nMiddleEntered, nMiddleWaiting;
};

void pool_init(struct pool *pool){
	memset(pool, 0, sizeof(*pool));
	rthread_lock_init(&pool->lock);
	//two condition variabes
	//one for middle schooler and one for high schoolers
	rthread_cv_init(&pool->high, &pool->lock);
	rthread_cv_init(&pool->middle, &pool->lock);
	pool->nHighEntered = pool->nHighWaiting = 0;
	pool->nMiddleEntered = pool->nMiddleWaiting = 0;
}

void pool_enter(struct pool *pool, int level){
	rthread_with(&pool->lock) {
		//corresponds to middle school
		if (level == 0){
			//if there are high schoolers in the pool or the max number of swimmers in pool, wait 
			while (pool->nHighEntered > 0 || pool->nMiddleEntered > NLANES){
				pool->nMiddleWaiting++;
				rthread_cv_wait(&pool->middle);
				pool->nMiddleWaiting--;
			}
			//once you are done waiting or if those conditions aren't met, enter pool and increment variable
			pool->nMiddleEntered++;
		}
		//corresponds to high school, same logic as above 
		else if (level == 1){
			while (pool->nMiddleEntered > 0 || pool->nHighEntered > NLANES){
				pool->nHighWaiting++;
				rthread_cv_wait(&pool->high);
				pool->nHighWaiting--;
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
		//middle school
		if (level == 0){ 
			//decrement variable
			pool->nMiddleEntered--;
			if(pool->nMiddleWaiting > 0){
				//let middle schoolers in if they are waiting
				rthread_cv_notify(&pool->middle);
			}
 			//if no one is the pool and both waiting, give priority to the other team
			else if(pool->nMiddleEntered == 0){
				rthread_cv_notifyAll(&pool->high);
			}
		}
		//same logic as above but with high school
		else if (level == 1){
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


char *middle[] = {
	"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13", "m14", "m15", "m16", "m17", "m18", "m19"
};
char *high[] = {
	"h0", "h1", "h2", "h3", "h4", "h5", "h6", "h7", "h8", "h9", "h10", "h11", "h12", "h13", "h14", "h15", "h16", "h17", "h18", "h19"
};

void swimmer(void *shared, void *arg){
	struct pool *pool = (struct pool *) shared;
	char *name = (char *) arg;
	for (int i = 0; i < NEXPERIMENTS; i++) {
		rthread_delay(random() % 10);
		printf("swimmer %s entering pool\n", name);
		pool_enter(pool, *name == 'h');
		printf("swimmer %s entered pool\n", name);
		rthread_delay(random() % 10);
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