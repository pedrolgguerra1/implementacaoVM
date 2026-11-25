#include <stdio.h>
#include <string.h>

#include "vm.h"

static signed char store[BACKING_SIZE];
static signed char memory[FRAMES][PAGE_SIZE];
static int page_to_frame[NUM_PAGES];
static int frame_to_page[FRAMES];
static unsigned long long last_use[FRAMES];
static int next_free;
static int fifo_next;
static alg_t page_alg;

int pager_init(alg_t alg, const char *backing_path)
{
    FILE *f = fopen(backing_path, "rb");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'.\n", backing_path);
        return -1;
    }
    size_t got = fread(store, 1, BACKING_SIZE, f);
    int extra = fgetc(f);
    fclose(f);
    if (got != BACKING_SIZE || extra != EOF) {
        fprintf(stderr, "Erro: '%s' deve ter exatamente %d bytes.\n", backing_path, BACKING_SIZE);
        return -1;
    }

    for (int i = 0; i < NUM_PAGES; i++)
        page_to_frame[i] = -1;
    for (int i = 0; i < FRAMES; i++)
        frame_to_page[i] = -1;
    next_free = 0;
    fifo_next = 0;
    page_alg = alg;
    return 0;
}

static int choose_victim(void)
{
    if (page_alg == ALG_FIFO) {
        int v = fifo_next;
        fifo_next = (fifo_next + 1) % FRAMES;
        return v;
    }
    int victim = 0;
    for (int i = 1; i < FRAMES; i++)
        if (last_use[i] < last_use[victim])
            victim = i;
    return victim;
}

int pager_translate(int page, unsigned long long now, int *fault, int *evicted_page)
{
    *fault = 0;
    *evicted_page = -1;

    int frame = page_to_frame[page];
    if (frame >= 0) {
        last_use[frame] = now;
        return frame;
    }

    *fault = 1;
    if (next_free < FRAMES) {
        frame = next_free++;
    } else {
        frame = choose_victim();
        *evicted_page = frame_to_page[frame];
        page_to_frame[*evicted_page] = -1;
    }
    memcpy(memory[frame], &store[page * PAGE_SIZE], PAGE_SIZE);
    frame_to_page[frame] = page;
    page_to_frame[page] = frame;
    last_use[frame] = now;
    return frame;
}

void pager_touch(int frame, unsigned long long now)
{
    last_use[frame] = now;
}

signed char pager_read(int frame, int offset)
{
    return memory[frame][offset];
}
