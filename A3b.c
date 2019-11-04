#define MIDDLE 0
#define HIGH 1
#define NLANES 7

// change A3test so that NMIDDLE and NHIGH are 7 

struct pool {
rthread_lock_t lock;
// you can add more monitor variables here
};
void pool_init(struct pool *pool){
memset(pool, 0, sizeof(*pool));
rthread_lock_init(&pool->lock);
// initialize your monitor variables here
}
void pool_enter(struct pool *pool, int level){
rthread_with(&pool->lock) {
// write the code here to enter the pool
}
}
void pool_exit(struct pool *pool, int level){
rthread_with(&pool->lock) {
// write the code here to exit the pool
}
}
