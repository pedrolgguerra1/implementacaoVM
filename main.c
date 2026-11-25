#include <stdio.h>
#include <stdlib.h>

#include "vm.h"

static void usage(const char *prog)
{
    fprintf(stderr, "Uso: %s <arquivo_de_enderecos> <fifo|lru> <fifo|lru>\n", prog);
    fprintf(stderr, "  2o argumento: algoritmo de substituicao de pagina\n");
    fprintf(stderr, "  3o argumento: algoritmo de substituicao da TLB\n");
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Erro: numero de argumentos invalido (recebidos %d, esperados 3).\n", argc - 1);
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    alg_t page_alg, tlb_alg;
    if (parse_alg(argv[2], &page_alg) != 0) {
        fprintf(stderr, "Erro: algoritmo de pagina invalido '%s' (use fifo ou lru).\n", argv[2]);
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (parse_alg(argv[3], &tlb_alg) != 0) {
        fprintf(stderr, "Erro: algoritmo de TLB invalido '%s' (use fifo ou lru).\n", argv[3]);
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    uint32_t *addrs = NULL;
    size_t n = 0;
    if (load_addresses(argv[1], &addrs, &n) != 0)
        return EXIT_FAILURE;

    if (pager_init(page_alg, BACKING_FILE) != 0) {
        free(addrs);
        return EXIT_FAILURE;
    }

    const char *out_path = getenv("VM_OUT");     /* so para os testes; padrao: correct.txt */
    if (!out_path || !*out_path)
        out_path = OUTPUT_FILE;
    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Erro: nao foi possivel criar '%s'.\n", out_path);
        free(addrs);
        return EXIT_FAILURE;
    }

#if USE_TLB
    tlb_init(tlb_alg);
#else
    (void)tlb_alg;
#endif

    long faults = 0, tlb_hits = 0;
    for (size_t i = 0; i < n; i++) {
        uint32_t va = addrs[i];
        int page = (int)((va >> 8) & 0xFF);
        int off  = (int)(va & OFFSET_MASK);
        int fault, evicted, frame;
        unsigned long long now = i + 1;
        int slot = -1;

#if USE_TLB
        slot = tlb_lookup(page, now, &frame);
        if (slot >= 0) {
            tlb_hits++;
            pager_touch(frame, now);
        } else {                         
            frame = pager_translate(page, now, &fault, &evicted);
            faults += fault;
            if (evicted >= 0)
                tlb_invalidate(evicted);
            slot = tlb_insert(page, frame, now);
        }
#else
        frame = pager_translate(page, now, &fault, &evicted);
        faults += fault;
#endif

        unsigned pa = (unsigned)frame * PAGE_SIZE + (unsigned)off;
        fprintf(out, "Virtual address: %u TLB: %d Physical address: %u Value: %d\n",
                va, slot, pa, pager_read(frame, off));
    }

    fprintf(out, "Number of Translated Addresses = %zu\n", n);
    fprintf(out, "Page Faults = %ld\n", faults);
    fprintf(out, "Page Fault Rate = %.3f\n", (double)faults / (double)n);
    fprintf(out, "TLB Hits = %ld\n", tlb_hits);
    fprintf(out, "TLB Hit Rate = %.3f\n", (double)tlb_hits / (double)n);

    fclose(out);
    free(addrs);
    printf("Concluido: %zu enderecos traduzidos, %ld page faults. Saida em %s\n", n, faults, out_path);
    return EXIT_SUCCESS;
}
