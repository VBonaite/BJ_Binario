#ifndef JOGO_H
#define JOGO_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "tabela_estrategia.h"
#include "baralho.h"
#include "constantes.h"
#include "structures.h"  // Usar estruturas centralizadas
#include "shoe_counter.h" // Para ShoeCounter

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
    bool contabilizada;
    double aposta;
    double pnl;
    char historico[32];
    int hist_len;
    char resultado;
    // Campos para análise de splits
    int split_pair_index;   // Índice do par na análise de splits (-1 se não aplicável)
    int split_upcard_index; // Índice do upcard na análise de splits (-1 se não aplicável)
    int split_cards_used;   // Total de cartas usadas no split
    int split_rank_idx;     // Rank índice do par que foi dividido (-1 se não aplicável)
    uint64_t original_split_bits; // Mão original antes do split (apenas 2 cartas iniciais)
} Mao;

int calcular_valor_mao(uint64_t mao);
TipoMao tipo_mao(uint64_t mao);
void avaliar_mao(uint64_t mao_bits, Mao *mao_out);
AcaoEstrategia determinar_acao(const Mao *mao, uint64_t mao_bits, int dealer_up_rank);
const char* acao_to_str(AcaoEstrategia a);
void avaliar_mao_dealer(Mao *dealer, Shoe *shoe, double *running_count, double *true_count);
// Função para jogar uma mão individual com ShoeCounter
Mao* jogar_mao(Mao *mao, Shoe *shoe, int dealer_up_rank, Mao *nova_mao_out, 
               double *running_count, double *true_count, ShoeCounter *shoe_counter, bool ev_realtime_enabled);

void verificar_mao(Mao *jogador, const Mao *dealer);
void calcular_pnl(Mao *mao);
int calcular_maos_contabilizadas(double true_count);

// Funções para sistema de apostas
int definir_aposta(size_t cartas_restantes, int vitorias, double true_count, int maos_jogadas, double loss_shoe, double unidade_atual);
double calcular_unidade(double bankroll);

#ifdef __cplusplus
}
#endif

#endif // JOGO_H 
