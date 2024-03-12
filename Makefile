CC=gcc
CFLAGS=-O3

all: average_avx2 average_sse

average_sse: average_sse.c
	$(CC) $(CFLAGS) average_sse.c -o average_sse

average_avx2: average_avx2.c
	$(CC) -mavx2 $(CFLAGS) average_avx2.c -o average_avx2

clean:
	$(RM) average_avx2 average_sse