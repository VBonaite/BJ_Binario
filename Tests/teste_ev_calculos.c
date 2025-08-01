#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "real_time_ev.h"
#include "shoe_counter.h"
#include "jogo.h"

// Teste de validação dos cálculos de EV
void test_ev_calculations() {
    printf("🧪 TESTE DE VALIDAÇÃO DOS CÁLCULOS DE EV\n");
    printf("==========================================\n\n");
    
    // Inicializar shoe counter com composição padrão (8 decks)
    ShoeCounter counter = {0};
    shoe_counter_init(&counter, 8);
    
    // Adicionar 8 decks (416 cartas)
    for (int rank = 0; rank < NUM_RANKS; rank++) {
        counter.counts[rank] = 32; // 8 decks × 4 cartas por rank
    }
    counter.total_cards = 416;
    counter.initialized = true;
    
    // Teste 1: Mão 16 vs Dealer 10 (situação conhecida)
    printf("📊 TESTE 1: Mão 16 vs Dealer 10 (TC = 0)\n");
    printf("-------------------------------------------\n");
    
    // Criar mão 16 (8+8)
    uint64_t hand_16 = 0;
    hand_16 |= (2ULL << (7 * 3)); // 2 cartas de rank 7 (valor 8)
    
    double true_count = 0.0;
    int dealer_upcard = 10;
    
    RealTimeEVResult result = calculate_real_time_ev(
        hand_16, dealer_upcard, true_count, &counter, true, true, true
    );
    
    printf("Mão: 16, Dealer: 10, TC: 0.0\n");
    printf("EV Stand:  %+.6f\n", result.ev_stand);
    printf("EV Hit:    %+.6f\n", result.ev_hit);
    printf("EV Double: %+.6f\n", result.ev_double);
    printf("Melhor ação: %c (EV = %+.6f)\n", result.best_action, result.best_ev);
    printf("\n");
    
    // Teste 2: Mão 20 vs Dealer 6 (situação favorável)
    printf("📊 TESTE 2: Mão 20 vs Dealer 6 (TC = 0)\n");
    printf("-------------------------------------------\n");
    
    // Criar mão 20 (10+10)
    uint64_t hand_20 = 0;
    hand_20 |= (2ULL << (8 * 3)); // 2 cartas de rank 8 (valor 10)
    
    dealer_upcard = 6;
    
    result = calculate_real_time_ev(
        hand_20, dealer_upcard, true_count, &counter, true, true, true
    );
    
    printf("Mão: 20, Dealer: 6, TC: 0.0\n");
    printf("EV Stand:  %+.6f\n", result.ev_stand);
    printf("EV Hit:    %+.6f\n", result.ev_hit);
    printf("EV Double: %+.6f\n", result.ev_double);
    printf("Melhor ação: %c (EV = %+.6f)\n", result.best_action, result.best_ev);
    printf("\n");
    
    // Teste 3: Mão 12 vs Dealer 4 (situação neutra)
    printf("📊 TESTE 3: Mão 12 vs Dealer 4 (TC = 0)\n");
    printf("-------------------------------------------\n");
    
    // Criar mão 12 (6+6)
    uint64_t hand_12 = 0;
    hand_12 |= (2ULL << (4 * 3)); // 2 cartas de rank 4 (valor 6)
    
    dealer_upcard = 4;
    
    result = calculate_real_time_ev(
        hand_12, dealer_upcard, true_count, &counter, true, true, true
    );
    
    printf("Mão: 12, Dealer: 4, TC: 0.0\n");
    printf("EV Stand:  %+.6f\n", result.ev_stand);
    printf("EV Hit:    %+.6f\n", result.ev_hit);
    printf("EV Double: %+.6f\n", result.ev_double);
    printf("Melhor ação: %c (EV = %+.6f)\n", result.best_action, result.best_ev);
    printf("\n");
    
    // Teste 4: Mão Soft 18 (A+7) vs Dealer 9
    printf("📊 TESTE 4: Mão Soft 18 (A+7) vs Dealer 9 (TC = 0)\n");
    printf("----------------------------------------------------\n");
    
    // Criar mão soft 18 (A+7)
    uint64_t hand_soft_18 = 0;
    hand_soft_18 |= (1ULL << (5 * 3)); // 1 carta de rank 5 (valor 7)
    hand_soft_18 |= (1ULL << (12 * 3)); // 1 carta de rank 12 (Ás)
    
    dealer_upcard = 9;
    
    result = calculate_real_time_ev(
        hand_soft_18, dealer_upcard, true_count, &counter, true, true, true
    );
    
    printf("Mão: Soft 18, Dealer: 9, TC: 0.0\n");
    printf("EV Stand:  %+.6f\n", result.ev_stand);
    printf("EV Hit:    %+.6f\n", result.ev_hit);
    printf("EV Double: %+.6f\n", result.ev_double);
    printf("Melhor ação: %c (EV = %+.6f)\n", result.best_action, result.best_ev);
    printf("\n");
    
    // Teste 5: Verificar probabilidades do dealer
    printf("📊 TESTE 5: Probabilidades do Dealer (TC = 0)\n");
    printf("-----------------------------------------------\n");
    
    DealerProbabilities dealer_probs = get_dealer_probabilities(10, 0.0, &counter);
    
    printf("Dealer 10 vs TC 0.0:\n");
    printf("P(17): %.4f\n", dealer_probs.prob_17);
    printf("P(18): %.4f\n", dealer_probs.prob_18);
    printf("P(19): %.4f\n", dealer_probs.prob_19);
    printf("P(20): %.4f\n", dealer_probs.prob_20);
    printf("P(21): %.4f\n", dealer_probs.prob_21);
    printf("P(BJ): %.4f\n", dealer_probs.prob_blackjack);
    printf("P(BUST): %.4f\n", dealer_probs.prob_bust);
    
    double total = dealer_probs.prob_17 + dealer_probs.prob_18 + dealer_probs.prob_19 + 
                  dealer_probs.prob_20 + dealer_probs.prob_21 + dealer_probs.prob_blackjack + 
                  dealer_probs.prob_bust;
    printf("Total: %.4f\n", total);
    printf("\n");
}

int main() {
    // Inicializar sistemas necessários
    init_ev_cache();
    
    // Executar testes
    test_ev_calculations();
    
    return 0;
} 