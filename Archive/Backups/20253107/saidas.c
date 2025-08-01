#include "saidas.h"
#include <stdio.h>

void imprimir_mao(uint64_t mao) {
    const char ranks[13] = {'2','3','4','5','6','7','8','9','T','J','Q','K','A'};
    for (int idx = 0; idx < 13; ++idx) {
        uint64_t count = (mao >> (idx * 3)) & 0x7ULL;
        for (uint64_t k = 0; k < count; ++k) {
            putchar(ranks[idx]);
        }
    }
    putchar('\n');
}

void mao_para_string(uint64_t mao, char *buf) {
    const char ranks[13] = {'2','3','4','5','6','7','8','9','T','J','Q','K','A'};
    int pos = 0;
    for (int idx = 0; idx < 13; ++idx) {
        uint64_t count = (mao >> (idx * 3)) & 0x7ULL;
        for (uint64_t k = 0; k < count; ++k) {
            buf[pos++] = ranks[idx];
        }
    }
    buf[pos] = '\0';
} 