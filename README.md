# simd-examples
Simple SIMD examples for training purposes

# TODO
Add makefile

# `average_sse.c`

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


