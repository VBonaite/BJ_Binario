#define _POSIX_C_SOURCE 199309L
#include "rng.h"
#include <time.h>
#include <unistd.h>
#include <stdint.h>

// Estado interno do RNG (não pode ser zero)
static uint64_t rng_state = 88172645463393265ULL;

// xorshift64* — rápido e com boa distribuição para a simulação
static uint64_t xorshift64star(void) {
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 2685821657736338717ULL;
}

void rng_init(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t seed = ((uint64_t)ts.tv_nsec) ^ ((uint64_t)ts.tv_sec << 21) ^ ((uint64_t)getpid() << 32) ^ (uint64_t)(uintptr_t)&ts;
    if (seed == 0) seed = 88172645463393265ULL;
    rng_state = seed;
    // Descartar algumas saídas iniciais
    for (int i = 0; i < 4; ++i) {
        xorshift64star();
    }
}

uint32_t rng_u32(void) {
    return (uint32_t)(xorshift64star() >> 32);
}

uint32_t rng_range(uint32_t max) {
    return max ? (rng_u32() % max) : 0;
} 