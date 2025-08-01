#ifndef SHOE_COUNTER_H
#define SHOE_COUNTER_H

#include "baralho.h"
#include <stdint.h>
#include <stdbool.h>

// Número de ranks diferentes (2-A = 13 tipos)
#define NUM_RANKS 13

// Estrutura para contar cartas restantes no shoe
typedef struct {
    int counts[NUM_RANKS];     // Contagem por rank (índices 0-12 = ranks 2-A)
    int total_cards;           // Total de cartas restantes
    int original_decks;        // Número original de decks
    bool initialized;          // Flag para verificar se foi inicializado
} ShoeCounter;

// Funções principais
void shoe_counter_init(ShoeCounter* counter, int num_decks);
void shoe_counter_remove_card(ShoeCounter* counter, Carta carta);
void shoe_counter_reset(ShoeCounter* counter);

// Funções de consulta
int shoe_counter_get_rank_count(const ShoeCounter* counter, int rank_idx);
int shoe_counter_get_total_cards(const ShoeCounter* counter);
double shoe_counter_get_rank_probability(const ShoeCounter* counter, int rank_idx);
double shoe_counter_get_specific_rank_probability(const ShoeCounter* counter, int rank_value);

// Funções de análise
int shoe_counter_get_ten_value_cards(const ShoeCounter* counter);
int shoe_counter_get_aces(const ShoeCounter* counter);
double shoe_counter_get_blackjack_probability(const ShoeCounter* counter);
double shoe_counter_get_bust_probability_on_hit(const ShoeCounter* counter, int current_hand_value);

// Funções utilitárias
void shoe_counter_print_status(const ShoeCounter* counter);
bool shoe_counter_validate(const ShoeCounter* counter);

// Conversões entre rank_idx (0-12) e rank_value (2-11)
int rank_value_to_idx(int rank_value);
int rank_idx_to_value(int rank_idx);

#endif // SHOE_COUNTER_H 