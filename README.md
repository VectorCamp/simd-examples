# simd-examples
Simple SIMD examples for training purposes

## TODO
Add makefile

## Examples

### `average_sse.c`

Calculates the average of `N` values in scalar and using SSE on x86

```
$ gcc -O3 average_sse.c -o average_sse
```

```
$ ./average_sse
updated diff1 tv_sec:2 tv_usec:151147
updated diff2 tv_sec:1 tv_usec:337442
average of 1073741824 elements = 16777216.000000 (scalar), 16777216.000000 (SSE)
scalar: 2 sec, usec: 151147
SSE: 1 sec, usec: 337442
```
### `average_avx2.c`

Calculates the average of `N` values in scalar and using AVX2 on x86

```
$ gcc -mavx2 -O3 average_avx2.c -o average_avx2
```

```
$ ./average_avx2 
updated diff1 tv_sec:2 tv_usec:549127
average of 1073741824 elements = 16777216.000000 (scalar), 16777216.000000 (AVX2)
scalar: 2 sec, usec: 549127
avx2   : 1 sec, usec: 337110
```

### `average_avx512.c`

Calculates the average of `N` values in scalar and using AVX512 on x86

```
$ gcc -mavx512f -O3 average_avx512.c -o average_avx512
```

```
$ ./average_avx512
updated diff1 tv_sec:2 tv_usec:550474
average of 1073741824 elements = 16777216.000000 (scalar), 16777216.000000 (AVX512)
scalar: 2 sec, usec: 550474
avx512   : 1 sec, usec: 336621

