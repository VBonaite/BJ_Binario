#ifndef EV_CALCULATOR_H
#define EV_CALCULATOR_H

#include "shoe_counter.h"
#include "dealer_freq_lookup.h"
#include <stdbool.h>

// Estrutura para resultado do cálculo de EV
typedef struct {
    double ev_stand;
    double ev_hit;
    double ev_double;
    double best_ev;
    char best_action;
    bool calculation_valid;
} EVResult;

// Funções principais de cálculo de EV
double calculate_ev_stand(int player_total, int dealer_upcard, const ShoeCounter* counter);
double calculate_ev_hit(int player_total, int dealer_upcard, const ShoeCounter* counter, int depth);
double calculate_ev_double(int player_total, int dealer_upcard, const ShoeCounter* counter);

// Função principal que calcula todos os EVs
EVResult calculate_all_evs(int player_total, int dealer_upcard, const ShoeCounter* counter);

// Funções auxiliares
double get_dealer_bust_probability(int dealer_upcard, const ShoeCounter* counter);
double get_dealer_final_probability(int dealer_upcard, int final_total, const ShoeCounter* counter);
double calculate_optimal_ev_recursive(int player_total, int dealer_upcard, const ShoeCounter* counter, int depth);

// Constantes para controle de recursão
#define MAX_RECURSION_DEPTH 10
#define BUST_TOTAL 22

// Função de demonstração
void demonstrate_ev_calculation_for_12(int dealer_upcard);

#endif // EV_CALCULATOR_H 