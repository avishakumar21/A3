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
	//an array of structs
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
	//initalize each conditional variable in struct array
	for( int i = 0; i < NMIDDLE + NHIGH; i++){
		rthread_cv_init(&pool->swimmers[i].cv, &pool->lock);
		pool->swimmers[i].type = -1; //initalize to -1
	}
	pool->front_index = 0; 
	pool->back_index = 0;
	pool->nHighEntered = pool->nMiddleEntered = 0;
	pool->nMiddleWaiting = pool->nHighWaiting = 0;
}

//set deafult index to an arbitary large number 
int default_index = NMIDDLE*(NEXPERIMENTS + 1) + NHIGH*(NEXPERIMENTS + 1);
void pool_enter(struct pool *pool, int level){
	int index = default_index;
	rthread_with(&pool->lock) {
		//corresponds to middle school
		if (level == 0){
			/*wait if there are high schoolers in the pool or if they are waiting and if your index isn't 
			less than the first index of the waiting queue */
			while((pool->nHighEntered != 0 || pool->nHighWaiting != 0) && !(index < pool->front_index)){
				//if spurious wake up, go back to sleep and wait to be notified 
				if(index != default_index){ 
					rthread_cv_wait(&pool->swimmers[(index) % (NMIDDLE + NHIGH)].cv);
				}
				else{
					//increment indexes and waiting variables
					index = pool->back_index;
					pool->back_index++;
					pool->nMiddleWaiting++;
					//set the type of the swimmer in waiting queue to 0 and wait 
					pool->swimmers[index % (NMIDDLE + NHIGH)].type = 0;
					rthread_cv_wait(&pool->swimmers[(index) % (NMIDDLE + NHIGH)].cv);
				}
			}
			//had to go through the while loop	
			if(index != default_index){	
				//if it waited, reset the variables before entering	
				pool->nMiddleWaiting--;
				pool->swimmers[(index) % (NMIDDLE + NHIGH)].type = -1;
			}
			pool->nMiddleEntered++;
		}
		//same logic as above but with high schoolers
		else if (level == 1){  
			while((pool->nMiddleEntered != 0 && pool->nMiddleWaiting != 0) && !(index < pool->front_index)){
				if(index != default_index){
					rthread_cv_wait(&pool->swimmers[(index) % (NMIDDLE + NHIGH)].cv);
				}
				else{
					index = pool->back_index;
					pool->back_index++;
					pool->nHighWaiting++;
					pool->swimmers[index % (NMIDDLE + NHIGH)].type = 1;
					rthread_cv_wait(&pool->swimmers[(index) % (NMIDDLE + NHIGH)].cv);

				}
			}
			if(index != default_index){			
				pool->nHighWaiting--;
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
        //corresponds to middle school
		if (level == 0){
			//decrement the entered variable
			pool->nMiddleEntered--; 
			//if there is no one in the waiting queue, or if there are other middle schoolers in the pool, do nothing
			if (pool->front_index == pool->back_index || pool->nMiddleEntered > 0){ 
			}
			//if the pool is empty now, notify the next group of high schoolers in order
			else if (pool->nMiddleEntered == 0){ 
				while(pool->swimmers[pool->front_index % (NMIDDLE + NHIGH)].type == 1){
					rthread_cv_notify(&pool->swimmers[(pool->front_index) % (NMIDDLE + NHIGH)].cv);
					pool->front_index++;
				}
			}

		}
		//same logic as above with high schoolers
		else if (level == 1){
			pool->nHighEntered--;
			if (pool->front_index == pool->back_index || pool->nHighEntered > 0){
			}
			else if (pool->nHighEntered == 0 ){
				while(pool->swimmers[pool->front_index % (NMIDDLE + NHIGH)].type == 0){
					rthread_cv_notify(&pool->swimmers[(pool->front_index) % (NMIDDLE + NHIGH)].cv);
					pool->front_index++;
				}
			}		

		}
		else{
			printf("level is not a valid parameter value (0 or 1)\n");			
		}		
	}
}
