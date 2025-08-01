#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void rng_init(void);
uint32_t rng_u32(void);
uint32_t rng_range(uint32_t max);

#endif // RNG_H 