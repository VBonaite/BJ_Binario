#ifndef REALTIME_STRATEGY_INTEGRATION_H
#define REALTIME_STRATEGY_INTEGRATION_H

#include "real_time_ev.h"
#include "shoe_counter.h"
#include "structures.h"
#include "tabela_estrategia.h"  // Para AcaoEstrategia
#include "jogo.h"              // Para Mao
#include <stdbool.h>

// Flag global para habilitar/desabilitar cálculo em tempo real
extern bool realtime_ev_enabled;

// Funções de inicialização e limpeza
void init_realtime_strategy_system(bool load_lookup_tables);
void cleanup_realtime_strategy_system(void);

// Função principal que substitui determinar_acao() em jogo.c
AcaoEstrategia determinar_acao_realtime(
    const Mao *mao, 
    uint64_t mao_bits, 
    int dealer_up_rank,
    double true_count,
    const ShoeCounter* shoe_counter,
    bool is_initial_hand
);

// Implementa regras especiais
AcaoEstrategia handle_special_rules(
    AcaoEstrategia base_action,
    const Mao *mao,
    bool is_initial_hand,
    bool double_allowed,
    bool split_allowed
);

// Tratamento de pares de Ases (recebem só uma carta)
bool is_aces_split(const Mao *mao);

// Tratamento de doubles (recebem só uma carta)
bool is_double_action(AcaoEstrategia action);

// Função para atualizar shoe counter após carta distribuída
void update_shoe_counter_after_card(ShoeCounter* counter, Carta carta);

// Funções de estatísticas e logging
typedef struct {
    int total_decisions;
    int realtime_decisions;
    int basic_strategy_decisions;
    int differences_found;
    double avg_ev_improvement;
    
    // Contadores de fallback específicos
    int fallback_invalid_input;        // FALLBACK #1: entradas inválidas
    int fallback_trivial_cases;        // FALLBACK #2,#3: blackjack, bust
    int fallback_invalid_counter;      // FALLBACK #4: shoe counter inválido
    int fallback_invalid_true_count;   // FALLBACK #5: true count inválido
    int fallback_calculation_failed;   // FALLBACK #6: cálculo falhou
    int fallback_invalid_ev_values;    // FALLBACK #7: valores EV insanos
    int fallback_invalid_action;       // FALLBACK #8,#11: ação inválida
    int fallback_context_restrictions; // FALLBACK #9,#10: double/split em contexto errado
    int total_fallbacks;               // Total de fallbacks usados
} RealtimeStrategyStats;

extern RealtimeStrategyStats realtime_stats;

void log_strategy_decision(
    uint64_t hand_bits,
    int dealer_upcard,
    double true_count,
    AcaoEstrategia realtime_action,
    AcaoEstrategia basic_action,
    double ev_improvement
);

void print_realtime_strategy_stats(void);
void print_fallback_summary(void);  // Função auxiliar para imprimir apenas fallbacks
void reset_realtime_strategy_stats(void);

#endif // REALTIME_STRATEGY_INTEGRATION_H 