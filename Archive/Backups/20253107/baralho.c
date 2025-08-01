#include "baralho.h"
#include "constantes.h"
#include "rng.h"
#include <stdlib.h>
#include <stdio.h>

static const char RANK_CHARS[13] = {'2','3','4','5','6','7','8','9','T','J','Q','K','A'};

// Codifica o rank para o padrão de 39 bits: grupo de 3 bits por rank
static Carta encode_rank(int idx) {
    return (Carta)1ULL << (idx * 3);
}

void baralho_criar(Shoe *shoe) {
    shoe->total = (size_t)DECKS * 52;
    shoe->cartas = (Carta*)malloc(sizeof(Carta) * shoe->total);
    if (!shoe->cartas) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    shoe->topo = 0;

    size_t pos = 0;
    for (int d = 0; d < DECKS; ++d) {
        for (int rank = 0; rank < 13; ++rank) {
            Carta c = encode_rank(rank);
            for (int suit = 0; suit < 4; ++suit) {
                shoe->cartas[pos++] = c;
            }
        }
    }
}

void baralho_embaralhar(Shoe *shoe) {
    for (size_t i = shoe->total - 1; i > 0; --i) {
        size_t j = rng_range((uint32_t)(i + 1));
        Carta tmp = shoe->cartas[i];
        shoe->cartas[i] = shoe->cartas[j];
        shoe->cartas[j] = tmp;
    }
    shoe->topo = 0; // reiniciar topo após embaralhar
}

Carta baralho_comprar(Shoe *shoe) {
    if (shoe->topo >= shoe->total) {
        fprintf(stderr, "Shoe vazio!\n");
        exit(EXIT_FAILURE);
    }
    return shoe->cartas[shoe->topo++];
}

void baralho_destruir(Shoe *shoe) {
    free(shoe->cartas);
    shoe->cartas = NULL;
    shoe->total = shoe->topo = 0;
}

char carta_para_char(Carta c) {
#if defined(__GNUC__)
    int idx = __builtin_ctzll(c) / 3;
#else
    int idx = 0;
    while ((c & 0x7ULL) == 0) {
        c >>= 3;
        ++idx;
    }
#endif
    return RANK_CHARS[idx];
} 

int carta_para_rank_idx(Carta c) {
#if defined(__GNUC__)
    return __builtin_ctzll(c) / 3;
#else
    int idx = 0;
    while ((c & 0x7ULL) == 0) {
        c >>= 3;
        ++idx;
    }
    return idx;
#endif
}

