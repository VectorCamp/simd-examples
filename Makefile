CC=gcc
CFLAGS=-O3 -march=native -Wall

ALL=average_neon

all: $(ALL)

average_neon: average_neon.c
	$(CC) $(CFLAGS) -o average_neon average_neon.c 


.PHONY: clean
clean:
	-rm -f $(ALL)

