#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int enabled, refs, started, release_callback, finished;

static void *callback(void *unused)
{
    (void)unused;
    pthread_mutex_lock(&lock);
    if (!enabled) { pthread_mutex_unlock(&lock); return NULL; }
    refs++;
    started = 1;
    pthread_cond_broadcast(&cv);
    while (!release_callback)
        pthread_cond_wait(&cv, &lock);
    finished = 1;
    refs--;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&lock);
    return NULL;
}

static void *safe_disable(void *unused)
{
    (void)unused;
    pthread_mutex_lock(&lock);
    enabled = 0;
    release_callback = 1;
    pthread_cond_broadcast(&cv);
    while (refs != 0)
        pthread_cond_wait(&cv, &lock);
    pthread_mutex_unlock(&lock);
    return NULL;
}

static void reset_state(void)
{
    enabled = 1;
    refs = 0;
    started = 0;
    release_callback = 0;
    finished = 0;
}

int main(void)
{
    pthread_t cb, dis;
    pthread_mutex_lock(&lock);
    reset_state();
    pthread_mutex_unlock(&lock);
    if (pthread_create(&cb, NULL, callback, NULL) != 0) return 2;
    pthread_mutex_lock(&lock);
    while (!started) pthread_cond_wait(&cv, &lock);
    pthread_mutex_unlock(&lock);
    if (pthread_create(&dis, NULL, safe_disable, NULL) != 0) return 2;
    pthread_join(dis, NULL);
    pthread_join(cb, NULL);
    pthread_mutex_lock(&lock);
    if (!finished || refs != 0) {
        pthread_mutex_unlock(&lock);
        fprintf(stderr, "callback drain returned before active user exited\n");
        return 1;
    }
    pthread_mutex_unlock(&lock);

    pthread_mutex_lock(&lock);
    reset_state();
    pthread_mutex_unlock(&lock);
    if (pthread_create(&cb, NULL, callback, NULL) != 0) return 2;
    pthread_mutex_lock(&lock);
    while (!started) pthread_cond_wait(&cv, &lock);
    enabled = 0;                 /* negative control: no reference drain */
    if (finished) {
        pthread_mutex_unlock(&lock);
        return 1;
    }
    release_callback = 1;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&lock);
    pthread_join(cb, NULL);
    puts("CALLBACK_DRAIN=PASS");
    puts("CALLBACK_NO_DRAIN_NEGATIVE_CONTROL=CAUGHT_ACTIVE_CALLBACK");
    return 0;
}
