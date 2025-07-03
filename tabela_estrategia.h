#ifndef TABELA_ESTRATEGIA_H
#define TABELA_ESTRATEGIA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ACAO_HIT = 0,
    ACAO_STAND,
    ACAO_DOUBLE,
    ACAO_SPLIT,
    ACAO_DOUBLE_OR_HIT,
    ACAO_DOUBLE_OR_STAND,
    ACAO_SPLIT_OR_HIT,
    ACAO_SPLIT_OR_STAND
} AcaoEstrategia;

// rank_upcard: 2-11 (onde 11 = A)
AcaoEstrategia estrategia_hard(int total, int rank_upcard);
AcaoEstrategia estrategia_soft(int total, int rank_upcard);
AcaoEstrategia estrategia_par(int par_rank, int rank_upcard);

#ifdef __cplusplus
}
#endif

#endif // TABELA_ESTRATEGIA_H 