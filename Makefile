CC=gcc
CFLAGS=-O3 -pg -Wall

all: average_avx512 average_avx2 average_sse

average_sse: average_sse.c
	$(CC) $(CFLAGS) average_sse.c -o average_sse

average_avx2: average_avx2.c
	$(CC) -mavx2 $(CFLAGS) average_avx2.c -o average_avx2

average_avx512: average_avx512.c
	$(CC) -mavx512f $(CFLAGS) average_avx512.c -o average_avx512

clean:
	$(RM) average_avx512 average_avx2 average_sse gmon.out