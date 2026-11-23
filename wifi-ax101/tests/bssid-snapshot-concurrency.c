#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#define ITERATIONS 3000000

static _Alignas(8) _Atomic uint64_t snapshot;
static _Atomic int done;
static _Atomic uint8_t torn_bytes[6];
static _Atomic int phase;
static _Atomic int reader_done;

static uint64_t pack(const uint8_t b[6]) {
    return (uint64_t)b[0] | ((uint64_t)b[1] << 8) |
        ((uint64_t)b[2] << 16) | ((uint64_t)b[3] << 24) |
        ((uint64_t)b[4] << 32) | ((uint64_t)b[5] << 40);
}
static int valid(uint64_t v, uint64_t a, uint64_t b) {
    return v == a || v == b;
}
static const uint8_t A[6] = {0x10,0x21,0x32,0x43,0x54,0x65};
static const uint8_t B[6] = {0xa0,0xb1,0xc2,0xd3,0xe4,0xf5};

static void *safe_writer(void *unused) {
    int i; (void)unused;
    for (i=0;i<ITERATIONS;i++)
        atomic_store_explicit(&snapshot, pack((i & 1) ? A : B), memory_order_seq_cst);
    atomic_store_explicit(&done,1,memory_order_release);
    return NULL;
}
static void *safe_reader(void *unused) {
    uint64_t a=pack(A), b=pack(B), v; (void)unused;
    do {
        v=atomic_load_explicit(&snapshot,memory_order_seq_cst);
        if (!valid(v,a,b)) return (void *)1;
    } while (!atomic_load_explicit(&done,memory_order_acquire));
    return NULL;
}

/* Negative control: six individually atomic octets, but no coherent snapshot. */
static void *unsafe_writer(void *unused) {
    int i; (void)unused;
    for (i=0;i<3;i++) atomic_store_explicit(&torn_bytes[i],A[i],memory_order_relaxed);
    atomic_store_explicit(&phase,1,memory_order_release);
    while (!atomic_load_explicit(&reader_done,memory_order_acquire)) sched_yield();
    for (i=3;i<6;i++) atomic_store_explicit(&torn_bytes[i],A[i],memory_order_relaxed);
    return NULL;
}
static void *unsafe_reader(void *unused) {
    uint8_t got[6]; int i; (void)unused;
    while (!atomic_load_explicit(&phase,memory_order_acquire)) sched_yield();
    for (i=0;i<6;i++) got[i]=atomic_load_explicit(&torn_bytes[i],memory_order_relaxed);
    atomic_store_explicit(&reader_done,1,memory_order_release);
    return (got[0]==A[0] && got[1]==A[1] && got[2]==A[2] &&
        got[3]==B[3] && got[4]==B[4] && got[5]==B[5]) ? (void *)1 : NULL;
}

int main(void) {
    pthread_t w,r; void *wr,*rr; int i;
    uint64_t a=pack(A);
    if (((uintptr_t)&snapshot & 7) != 0) {
        fprintf(stderr,"snapshot alignment is not 8 bytes\n"); return 2;
    }
    if (!atomic_is_lock_free(&snapshot)) {
        fprintf(stderr,"64-bit atomic is not lock-free on this target\n"); return 2;
    }
    atomic_store(&snapshot,a);
    if (pthread_create(&r,NULL,safe_reader,NULL)!=0 ||
        pthread_create(&w,NULL,safe_writer,NULL)!=0) return 2;
    pthread_join(w,&wr); pthread_join(r,&rr);
    if (wr != NULL || rr != NULL) { fprintf(stderr,"atomic snapshot tore\n"); return 1; }
    for (i=0;i<6;i++) atomic_store(&torn_bytes[i],B[i]);
    if (pthread_create(&r,NULL,unsafe_reader,NULL)!=0 ||
        pthread_create(&w,NULL,unsafe_writer,NULL)!=0) return 2;
    pthread_join(w,&wr); pthread_join(r,&rr);
    if (rr == NULL) { fprintf(stderr,"negative control failed to detect torn BSSID\n"); return 1; }
    puts("BSSID_ATOMIC_SNAPSHOT=PASS");
    puts("BSSID_UNSAFE_NEGATIVE_CONTROL=CAUGHT_TORN_SNAPSHOT");
    return 0;
}
