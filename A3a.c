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
			//assert(pool->nHighEntered == 0 || (pool->nHighEntered > 0 && pool->nMiddleEntered == 0));
			//if there are high schoolers in the pool or the max number of swimmers in pool, wait 
			while (pool->nHighEntered > 0 || pool->nMiddleEntered > NLANES){
				pool->nMiddleWaiting++;
				rthread_cv_wait(&pool->middle);
				pool->nMiddleWaiting--;
			}
			//once you are done waiting or if those conditions aren't met, enter pool and increment variable
			//assert(pool->nHighEntered == 0 || pool->nMiddleEntered < NLANES);
			pool->nMiddleEntered++;
		}
		//corresponds to high school, same logic as above 
		else if (level == 1){
			//assert(pool->nMiddleEntered == 0 || (pool->nMiddleEntered > 0 && pool->nHighEntered == 0));
			while (pool->nMiddleEntered > 0 || pool->nHighEntered > NLANES){
				pool->nHighWaiting++;
				rthread_cv_wait(&pool->high);
				pool->nHighWaiting--;
			}
			//assert(pool->nMiddleEntered == 0 || pool->nHighEntered < NLANES);
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
			//assert(pool->nHighEntered == 0);
			//assert(pool->nMiddleEntered > 0);
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
			//assert(pool->nMiddleEntered == 0);
			//assert(pool->nHighEntered > 0);
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
