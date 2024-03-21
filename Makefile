CC=gcc
CFLAGS=-O3 -Wall

ARCH := $(shell uname -m)

ifeq ($(ARCH),arm64)
    CFLAGS += -march=armv8-a+simd+crc
    ALL += average_neon
else ifeq ($(ARCH),x86_64)
    CFLAGS += -march=native
    ALL += average_avx2 average_avx512 average_sse
else ifeq ($(ARCH),ppc64le)  
    CFLAGS += -mcpu=native
    ALL += average_power
else ifeq ($(ARCH),x86_64)
    ALL= average_avx512 average_avx2 average_sse scalarxmat44
    all: $(ALL)
    average_sse: average_sse.c
        $(CC) $(CFLAGS) average_sse.c -o average_sse
    average_avx2: average_avx2.c
        $(CC) -mavx2 $(CFLAGS) average_avx2.c -o average_avx2
else ifeq ($(ARCH),aarch64)
    CFLAGS += -march=native
    ALL=average_neon
    all: $(ALL)
    average_neon: average_neon.c
        $(CC) $(CFLAGS) -o average_neon average_neon.c
endif

average_avx512: average_avx512.c
    $(CC) -mavx512f $(CFLAGS) average_avx512.c -o average_avx512

average_avx2: average_avx2.c
    $(CC) $(CFLAGS) -mavx2 -o average_avx2 average_avx2.c

average_sse: average_sse.c
    $(CC) $(CFLAGS) -msse4.2 -o average_sse average_sse.c

average_power: average_power.c
    $(CC) $(CFLAGS) -maltivec -o average_power average_power.c

scalarxmat44: scalarxmat44.c
    $(CC) $(CFLAGS) scalarxmat44.c -o scalarxmat44

.PHONY: clean
clean:
    -rm -f $(ALL)
