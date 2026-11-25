CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -D_POSIX_C_SOURCE=200809L -pthread

vm: main.c validate.c pager.c tlb.c vm.h
	$(CC) $(CFLAGS) -o vm main.c validate.c pager.c tlb.c

clean:
	rm -f vm