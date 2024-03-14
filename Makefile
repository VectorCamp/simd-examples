CC=gcc
CFLAGS=-O3 -Wall

ARCH := $(shell uname -m)

ifeq ($(ARCH),arm64)
	CFLAGS += -march=armv8-a+simd+crc
	ALL += average_neon
endif

ifeq ($(ARCH),x86_64)
	CFLAGS += -march=native
	ALL += average_avx2 average_avx512 average_sse
endif

all: $(ALL)

average_neon: average_neon.c
	$(CC) $(CFLAGS) -o average_neon average_neon.c 

average_avx2: average_avx2.c
	$(CC) $(CFLAGS) -mavx2 -o average_avx2 average_avx2.c

average_avx512: average_avx512.c
	$(CC) $(CFLAGS) -mavx512f -o average_avx512 average_avx512.c

average_sse: average_sse.c
	$(CC) $(CFLAGS) -msse4.2 -o average_sse average_sse.c

.PHONY: clean
clean:
	-rm -f $(ALL)
