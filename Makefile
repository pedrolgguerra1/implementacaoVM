CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2 -D_POSIX_C_SOURCE=200809L
SRC     = main.c validate.c pager.c tlb.c
HDR     = vm.h

all: vm

vm: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC)

vm_notlb256: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -DUSE_TLB=0 -DFRAMES=256 -o $@ $(SRC)

vm_notlb128: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -DUSE_TLB=0 -o $@ $(SRC)

clean:
	rm -f vm vm_notlb256 vm_notlb128 *.o

.PHONY: all clean
