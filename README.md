# Implementação 6 – Gerenciador de Memória Virtual

Infraestrutura de Software – CESAR School.
Implementação em C do *Designing a Virtual Memory Manager* (Silberschatz et al., *Operating System Concepts*, 10ª ed., p. P-51), com as modificações pedidas pelo professor.

## Como o programa funciona

- Endereços lógicos de 16 bits: 8 bits de página (256 páginas) + 8 bits de deslocamento (256 bytes por página/frame).
- Memória física com **128 frames**, preenchidos de 0 a 127; quando enche, aplica-se **FIFO** ou **LRU** para escolher o frame a ser substituído.
- **TLB de 16 entradas** com substituição **FIFO** ou **LRU**.
- A busca na TLB usa **uma thread (pthread) por entrada**; a thread principal espera todas (`pthread_join`) e lê o resultado.
- O conteúdo das páginas vem do arquivo `BACKING_STORE.bin` (65.536 bytes), que deve estar no diretório de execução.
- Saída em `correct.txt`, no mesmo formato do arquivo do professor (endereço virtual, entrada da TLB, endereço físico, valor e as estatísticas finais).

## Arquivos

| Arquivo | Conteúdo |
|---|---|
| `main.c` | argumentos, laço de tradução (TLB → tabela de páginas), saída |
| `validate.c` | validação do algoritmo (fifo/lru) e do arquivo de endereços |
| `pager.c` | tabela de páginas, memória física, FIFO/LRU de páginas |
| `tlb.c` | TLB, FIFO/LRU da TLB, busca sequencial e busca com threads |
| `vm.h` | constantes e protótipos |
| `Makefile` | `make`, `make clean`, `make evidencias`, `make tar LOGIN=<login>` |
| `evidencias.sh` / `evidencias.log` | testes dos fluxos da rubrica e seu resultado |

## Compilação e execução

Requer `gcc`, `make` e `pthread` (Linux, Unix ou macOS).

```sh
make                              
./vm addresses.txt lru fifo       
make clean                        
```

Uso: `./vm <arquivo_de_enderecos> <fifo|lru> <fifo|lru>`

1. arquivo de endereços lógicos (um inteiro de 0 a 65535 por linha);
2. algoritmo de substituição de **página**;
3. algoritmo de substituição da **TLB**.

O resultado é gravado em `correct.txt` (a variável de ambiente `VM_OUT` pode trocar o nome do arquivo de saída; é usada só nos testes).


## Evidências dos fluxos da rubrica

Para mostrar cada fluxo separadamente, o `Makefile` gera variantes do mesmo código (por macros de compilação). O programa entregue (`vm`) tem sempre TLB com threads.

| Fluxo | Alvo do Makefile | Macros |
|---|---|---|
| sem TLB, 256 frames | `vm_notlb256` | `-DUSE_TLB=0 -DFRAMES=256` |
| sem TLB, 128 frames (FIFO ou LRU) | `vm_notlb128` | `-DUSE_TLB=0` |
| com TLB, sem thread (FIFO ou LRU) | `vm_tlb_seq` | `-DTLB_THREADS=0` |
| com TLB e threads | `vm` | – |

Resumo dos resultados com `addresses.txt` (1000 endereços):

| Fluxo | Page faults | TLB hits |
|---|---|---|
| sem TLB, 256 frames | 244 | – |
| sem TLB, 128 frames, FIFO | 538 | – |
| sem TLB, 128 frames, LRU | 539 | – |
| com TLB, página LRU, TLB FIFO | 539 | 54 |
| com TLB, página LRU, TLB LRU | 539 | 55 |
| com TLB, página FIFO, TLB FIFO | 538 | 54 |
| com TLB, página FIFO, TLB LRU | 538 | 55 |

Com `./vm addresses.txt lru fifo`, a saída é idêntica (byte a byte) ao `correct.txt` do professor, tanto na busca sequencial quanto na busca com threads.

## Decisões de projeto

- A coluna `TLB:` da saída é o **índice da entrada (0 a 15)** da TLB que atendeu o acesso (no hit) ou que recebeu a tradução (no miss), como no arquivo de referência.
- Um acesso resolvido pela TLB também conta como uso da página para o LRU.
- Quando um frame é reaproveitado, a entrada da TLB dessa página é invalidada, para a TLB nunca apontar para um frame que já guarda outra página.
- Cada thread de busca só lê a sua própria entrada da TLB e escreve só no seu resultado; a TLB é alterada apenas pela thread principal, fora da busca. Não há *race condition* (verificado com ThreadSanitizer: `-fsanitize=thread`).
