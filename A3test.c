#include <stdio.h>
#include <stdlib.h>
#include "rthread.h"
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
pool_enter(pool, *name == ’h’);
printf("swimmer %s entered pool\n", name);
rthread_delay(random() % 1000);
printf("swimmer %s leaving pool\n", name);
pool_exit(pool, *name == ’h’);
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