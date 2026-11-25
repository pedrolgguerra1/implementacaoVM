#ifndef VM_H
#define VM_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE    256
#define NUM_PAGES    256
#define OFFSET_MASK  0xFF
#define BACKING_SIZE (NUM_PAGES * PAGE_SIZE)

#ifndef FRAMES
#define FRAMES 128
#endif

#define TLB_SIZE 16

#ifndef USE_TLB
#define USE_TLB 1
#endif

#ifndef TLB_THREADS
#define TLB_THREADS 1
#endif

#define BACKING_FILE "BACKING_STORE.bin"
#define OUTPUT_FILE  "correct.txt"

typedef enum { ALG_FIFO, ALG_LRU } alg_t;

int parse_alg(const char *text, alg_t *alg);
int load_addresses(const char *path, uint32_t **addrs, size_t *count);

int  pager_init(alg_t alg, const char *backing_path);
int  pager_translate(int page, unsigned long long now, int *fault, int *evicted_page);
void pager_touch(int frame, unsigned long long now);
signed char pager_read(int frame, int offset);

void tlb_init(alg_t alg);
int  tlb_lookup(int page, unsigned long long now, int *frame);
int  tlb_insert(int page, int frame, unsigned long long now);
void tlb_invalidate(int page);
unsigned long tlb_threads_created(void);

#endif
