#ifndef REAL_TIME_EV_H
#define REAL_TIME_EV_H

#include "shoe_counter.h"
#include "dealer_freq_lookup.h"
#include "split_ev_lookup.h"
#include "structures.h"
#include "tabela_estrategia.h"  // Para AcaoEstrategia
#include "jogo.h"              // Para Mao
#include <stdbool.h>

// Constantes para cálculo de EV
#define MAX_HAND_VALUE 21
#define MIN_HAND_VALUE 12
#define MAX_RECURSION_DEPTH 4  // CORREÇÃO CRÍTICA: aumentar profundidade para cálculos mais precisos

// Limites para True Count
#define MAX_TC_LIMIT 6.5
#define MIN_TC_LIMIT -6.5

// Estrutura para armazenar EVs calculados
typedef struct {
    double ev_stand;
    double ev_hit;
    double ev_double;
    double ev_split;
    bool has_split_option;
    char best_action;
    double best_ev;
    bool calculation_valid;
} RealTimeEVResult;

// Estrutura para probabilidades do dealer
typedef struct {
    double prob_17;
    double prob_18;
    double prob_19;
    double prob_20;
    double prob_21;
    double prob_blackjack;
    double prob_bust;
    bool probabilities_valid;
} DealerProbabilities;

// ====================== FUNÇÕES PRINCIPAIS ======================

// Função principal para calcular EV em tempo real
RealTimeEVResult calculate_real_time_ev(
    uint64_t hand_bits,
    int dealer_upcard,
    double true_count,
    const ShoeCounter* counter,
    bool is_initial_hand,
    bool double_allowed,
    bool split_allowed
);

// Calcula EVs individuais
double calculate_ev_stand_realtime(int hand_value, int dealer_upcard, double true_count, const ShoeCounter* counter);
double calculate_ev_hit_realtime(uint64_t hand_bits, int dealer_upcard, double true_count, const ShoeCounter* counter, int depth);
double calculate_ev_double_realtime(uint64_t hand_bits, int dealer_upcard, double true_count, const ShoeCounter* counter);
double calculate_ev_split_realtime(int pair_rank, int dealer_upcard, double true_count, const ShoeCounter* counter);

// ====================== FUNÇÕES AUXILIARES ======================

// Normaliza True Count para lookup nas tabelas
double normalize_true_count(double true_count);

// Obtém bin correto para True Count
double get_tc_bin_start(double true_count);

// Calcula probabilidades do dealer baseado em upcard e TC
DealerProbabilities get_dealer_probabilities(int dealer_upcard, double true_count, const ShoeCounter* counter);

// Determina se a mão é soft, hard ou par
bool is_soft_hand(uint64_t hand_bits);
bool is_pair_hand(uint64_t hand_bits);
int get_pair_rank(uint64_t hand_bits);

// Calcula valor ótimo após receber uma carta específica
double calculate_ev_after_receiving_card(uint64_t current_hand, int card_rank, int dealer_upcard, 
                                        double true_count, const ShoeCounter* counter, int depth);

// Adiciona carta à mão (função auxiliar)
uint64_t add_card_to_hand(uint64_t hand_bits, int card_rank);

// Simula remoção de carta do shoe counter
ShoeCounter simulate_card_removal(const ShoeCounter* counter, int card_rank);

// CORREÇÃO CRÍTICA: Funções corrigidas para tratamento de cartas de valor 10
double get_card_probability(const ShoeCounter* counter, int card_rank);
ShoeCounter simulate_card_removal_corrected(const ShoeCounter* counter, int card_rank);

// ====================== MELHORIAS RECOMENDADAS ======================

// Mapeamento biunívoco com parâmetro suit_offset
int rank_value_to_idx_bijective(int rank_value, int suit_offset);

// Iteração por rank_idx nos loops de EV
void iterate_by_rank_idx_for_ev(const ShoeCounter* counter, 
                                void (*callback)(int rank_idx, double probability, void* user_data),
                                void* user_data);

// Simulação de remoção por rank_idx
ShoeCounter simulate_card_removal_by_idx(const ShoeCounter* counter, int rank_idx);

// Normalização aprimorada com tolerância 1e-6
bool validate_probability_sum(const ShoeCounter* counter);

// Lookup de frequências reais do dealer
DealerProbabilities get_dealer_probabilities_real_lookup(int dealer_upcard, double true_count, const ShoeCounter* counter);

// Validação estatística forte
bool validate_ev_calculations_statistical(const RealTimeEVResult* result, 
                                         uint64_t hand_bits, 
                                         int dealer_upcard, 
                                         double true_count,
                                         const ShoeCounter* counter);

// ====================== FUNÇÕES DE MEMOIZAÇÃO ======================

// Cache para EVs já calculados (opcional, para otimização)
typedef struct {
    uint64_t hand_bits;
    int dealer_upcard;
    double true_count;
    RealTimeEVResult result;
    bool valid;
} EVCache;

// Funções de cache otimizado
void init_ev_cache(void);
bool get_cached_ev(uint64_t hand_bits, int dealer_upcard, double true_count, RealTimeEVResult* result);
void cache_ev_result(uint64_t hand_bits, int dealer_upcard, double true_count, const RealTimeEVResult* result);
void clear_ev_cache(void);
void print_cache_stats(void);

// ====================== FUNÇÕES DE INTEGRAÇÃO ======================

// Substitui a chamada de estratégia básica
AcaoEstrategia determine_optimal_action_realtime(
    uint64_t hand_bits,
    int dealer_upcard,
    double true_count,
    const ShoeCounter* counter,
    bool is_initial_hand
);

// Função para debug/logging
void print_ev_breakdown(const RealTimeEVResult* result, uint64_t hand_bits, int dealer_upcard, double true_count);

// ====================== FUNÇÕES DE VALIDAÇÃO ======================

// Valida se os cálculos estão consistentes
bool validate_ev_calculations(const RealTimeEVResult* result);

// Valida dados de entrada para evitar estados inconsistentes
bool validate_shoe_counter(const ShoeCounter* counter);
bool validate_dealer_probabilities(const DealerProbabilities* probs);
bool validate_true_count(double true_count);

// Compara com estratégia básica tradicional (para debug)
void compare_with_basic_strategy(uint64_t hand_bits, int dealer_upcard, 
                                AcaoEstrategia basic_action, AcaoEstrategia realtime_action);

#endif // REAL_TIME_EV_H 