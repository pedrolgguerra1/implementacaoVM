#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "vm.h"

int parse_alg(const char *text, alg_t *alg)
{
    if (strcmp(text, "fifo") == 0) { *alg = ALG_FIFO; return 0; }
    if (strcmp(text, "lru")  == 0) { *alg = ALG_LRU;  return 0; }
    return -1;
}

static int parse_line(const char *s, uint32_t *value)
{
    size_t len = strlen(s);
    if (len == 0 || len > 5)
        return -1;
    uint32_t v = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] < '0' || s[i] > '9')
            return -1;
        v = v * 10 + (uint32_t)(s[i] - '0');
    }
    if (v > 65535)
        return -1;
    *value = v;
    return 0;
}

int load_addresses(const char *path, uint32_t **addrs, size_t *count)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "Erro: arquivo de enderecos '%s' nao existe ou nao pode ser acessado (%s).\n",
                path, strerror(errno));
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Erro: '%s' nao e um arquivo regular.\n", path);
        return -1;
    }
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s' (%s).\n", path, strerror(errno));
        return -1;
    }

    size_t cap = 1024, n = 0;
    uint32_t *buf = malloc(cap * sizeof *buf);
    if (!buf) {
        fprintf(stderr, "Erro: memoria insuficiente.\n");
        fclose(f);
        return -1;
    }

    char *line = NULL;
    size_t line_cap = 0;
    ssize_t got;
    size_t lineno = 0;
    int seen_blank = 0;
    int rc = 0;

    while ((got = getline(&line, &line_cap, f)) != -1) {
        lineno++;
        while (got > 0 && (line[got - 1] == '\n' || line[got - 1] == '\r' ||
                           line[got - 1] == ' '  || line[got - 1] == '\t'))
            line[--got] = '\0';

        if (got == 0) {
            seen_blank = 1;
            continue;
        }
        uint32_t v;
        if (seen_blank || parse_line(line, &v) != 0) {
            fprintf(stderr, "Erro: formato invalido em '%s', linha %zu: esperado um inteiro entre 0 e 65535 por linha.\n",
                    path, lineno);
            rc = -1;
            break;
        }
        if (n == cap) {
            cap *= 2;
            uint32_t *tmp = realloc(buf, cap * sizeof *buf);
            if (!tmp) {
                fprintf(stderr, "Erro: memoria insuficiente.\n");
                rc = -1;
                break;
            }
            buf = tmp;
        }
        buf[n++] = v;
    }
    if (rc == 0 && ferror(f)) {
        fprintf(stderr, "Erro: falha ao ler '%s' (%s).\n", path, strerror(errno));
        rc = -1;
    }
    if (rc == 0 && n == 0) {
        fprintf(stderr, "Erro: o arquivo '%s' nao contem enderecos.\n", path);
        rc = -1;
    }

    free(line);
    fclose(f);
    if (rc != 0) {
        free(buf);
        return -1;
    }
    *addrs = buf;
    *count = n;
    return 0;
}
