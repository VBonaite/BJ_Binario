#ifndef JOGO_H
#define JOGO_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "tabela_estrategia.h"
#include "baralho.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAO_HARD = 0,
    MAO_SOFT,
    MAO_PAR,
    MAO_BLACKJACK
} TipoMao;

typedef struct {
    uint64_t bits;
    uint64_t initial_bits;
    int valor;
    bool blackjack;
    TipoMao tipo;
    bool finalizada;
    bool from_split;
    bool isdouble;
    double aposta;
    double pnl;
    char historico[32];
    int hist_len;
    char resultado;
} Mao;

int calcular_valor_mao(uint64_t mao);
TipoMao tipo_mao(uint64_t mao);
void avaliar_mao(uint64_t mao_bits, Mao *mao_out);
AcaoEstrategia determinar_acao(const Mao *mao, uint64_t mao_bits, int dealer_up_rank);
const char* acao_to_str(AcaoEstrategia a);
void avaliar_mao_dealer(Mao *dealer, Shoe *shoe, double *running_count, double *true_count, FILE *count_file, int shoe_num);
Mao* jogar_mao(Mao *mao, Shoe *shoe, int dealer_up_rank, Mao *nova_mao_out, double *running_count, double *true_count, FILE *count_file, int shoe_num);

void verificar_mao(Mao *jogador, const Mao *dealer);
void calcular_pnl(Mao *mao);
#ifdef __cplusplus
}
#endif

#endif // JOGO_H 
