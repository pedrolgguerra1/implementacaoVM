CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2 -D_POSIX_C_SOURCE=200809L
SRC     = main.c validate.c pager.c

all: vm

vm: $(SRC) vm.h
	$(CC) $(CFLAGS) -o $@ $(SRC)

vm_256: $(SRC) vm.h
	$(CC) $(CFLAGS) -DFRAMES=256 -o $@ $(SRC)

clean:
	rm -f vm vm_256 *.o

.PHONY: all clean
