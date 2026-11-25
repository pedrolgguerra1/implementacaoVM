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

int tlb_lookup(int page, unsigned long long now, int *frame)
{
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page == page) {
            tlb[i].last_use = now;
            *frame = tlb[i].frame;
            return i;
        }
    }
    return -1;
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
