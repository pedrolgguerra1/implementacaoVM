#include <pthread.h>

#include "vm.h"

typedef struct {
    int page;
    int frame;
    unsigned long long last_use;
} tlb_entry_t;

static tlb_entry_t tlb[TLB_SIZE];
static int fifo_next;
static alg_t tlb_alg;

void tlb_init(alg_t alg)
{
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].page = -1;
        tlb[i].frame = -1;
        tlb[i].last_use = 0;
    }
    fifo_next = 0;
    tlb_alg = alg;
}

static unsigned long threads_created;

static int lookup_sequential(int page)
{
    for (int i = 0; i < TLB_SIZE; i++)
        if (tlb[i].page == page)
            return i;
    return -1;
}

typedef struct {
    int idx;
    int page;
    int hit;
} search_arg_t;

static void *search_entry(void *p)
{
    search_arg_t *a = p;
    a->hit = (tlb[a->idx].page == a->page);
    return NULL;
}

static int lookup_threads(int page)
{
    pthread_t th[TLB_SIZE];
    search_arg_t arg[TLB_SIZE];
    int started[TLB_SIZE];

    for (int i = 0; i < TLB_SIZE; i++) {
        arg[i].idx = i;
        arg[i].page = page;
        arg[i].hit = 0;
        started[i] = (pthread_create(&th[i], NULL, search_entry, &arg[i]) == 0);
        if (started[i])
            threads_created++;
        else
            search_entry(&arg[i]);
    }

    int found = -1;
    for (int i = 0; i < TLB_SIZE; i++) {
        if (started[i])
            pthread_join(th[i], NULL);
        if (arg[i].hit)
            found = i;
    }
    return found;
}

int tlb_lookup(int page, unsigned long long now, int *frame)
{
    int slot = TLB_THREADS ? lookup_threads(page) : lookup_sequential(page);
    if (slot >= 0) {
        tlb[slot].last_use = now;
        *frame = tlb[slot].frame;
    }
    return slot;
}

unsigned long tlb_threads_created(void)
{
    return threads_created;
}

int tlb_insert(int page, int frame, unsigned long long now)
{
    int slot = -1;

    if (tlb_alg == ALG_FIFO) {
        slot = fifo_next;
        fifo_next = (fifo_next + 1) % TLB_SIZE;
    } else {
        for (int i = 0; i < TLB_SIZE; i++)
            if (tlb[i].page == -1) { slot = i; break; }
        if (slot < 0) {
            slot = 0;
            for (int i = 1; i < TLB_SIZE; i++)
                if (tlb[i].last_use < tlb[slot].last_use)
                    slot = i;
        }
    }

    tlb[slot].page = page;
    tlb[slot].frame = frame;
    tlb[slot].last_use = now;
    return slot;
}

void tlb_invalidate(int page)
{
    for (int i = 0; i < TLB_SIZE; i++)
        if (tlb[i].page == page)
            tlb[i].page = -1;
}
