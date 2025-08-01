#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "shoe_counter.h"
#include "real_time_ev.h"
#include "jogo.h"

// Teste de debug para entender o sistema de EV
void test_debug_ev() {
    printf("=== TESTE DE DEBUG DO SISTEMA DE EV ===\n");
    
    // Criar um shoe counter com 1 deck
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    printf("Estado do shoe:\n");
    printf("  Total de cartas: %d\n", counter.total_cards);
    printf("  Cartas de valor 10: %d\n", shoe_counter_get_ten_value_cards(&counter));
    
    // Testar uma mão mais simples: 12 vs 10
    uint64_t hand_bits = 0;
    // Adicionar 10 (índice 8)
    hand_bits |= (1ULL << (8 * 3));
    // Adicionar 2 (índice 0)
    hand_bits |= (1ULL << (0 * 3));
    
    printf("\nTestando mão 12 vs dealer 10:\n");
    printf("Mão: 10, 2 (valor 12)\n");
    
    // Calcular EV em tempo real
    RealTimeEVResult result = calculate_real_time_ev(
        hand_bits, 10, 0.0, &counter, true, true, true
    );
    
    if (result.calculation_valid) {
        printf("\nResultados:\n");
        printf("  EV Stand: %.6f\n", result.ev_stand);
        printf("  EV Hit: %.6f\n", result.ev_hit);
        printf("  EV Double: %.6f\n", result.ev_double);
        printf("  Melhor ação: %c\n", result.best_action);
        printf("  Melhor EV: %.6f\n", result.best_ev);
        
        // Análise detalhada
        printf("\nAnálise:\n");
        if (result.ev_hit > result.ev_stand) {
            printf("  ✓ EV Hit > EV Stand (esperado para 12 vs 10)\n");
        } else {
            printf("  ✗ EV Hit <= EV Stand (inesperado para 12 vs 10)\n");
        }
        
        printf("  Diferença EV Hit - EV Stand: %.6f\n", result.ev_hit - result.ev_stand);
    } else {
        printf("  ✗ Cálculo inválido\n");
    }
    
    // Testar mão 16 vs 10 novamente com mais detalhes
    printf("\n=== TESTE DETALHADO: 16 vs 10 ===\n");
    
    uint64_t hand_16 = 0;
    hand_16 |= (1ULL << (8 * 3)); // 10
    hand_16 |= (1ULL << (4 * 3)); // 6
    
    printf("Mão: 10, 6 (valor 16)\n");
    
    RealTimeEVResult result2 = calculate_real_time_ev(
        hand_16, 10, 0.0, &counter, true, true, true
    );
    
    if (result2.calculation_valid) {
        printf("\nResultados:\n");
        printf("  EV Stand: %.6f\n", result2.ev_stand);
        printf("  EV Hit: %.6f\n", result2.ev_hit);
        printf("  EV Double: %.6f\n", result2.ev_double);
        printf("  Melhor ação: %c\n", result2.best_action);
        printf("  Melhor EV: %.6f\n", result2.best_ev);
        
        printf("\nAnálise:\n");
        printf("  EV Hit - EV Stand: %.6f\n", result2.ev_hit - result2.ev_stand);
        
        // Verificar se o problema está na profundidade de recursão
        printf("  Profundidade de recursão: %d\n", MAX_RECURSION_DEPTH);
    }
}

// Teste para verificar as probabilidades do dealer
void test_dealer_probabilities() {
    printf("\n=== TESTE DE PROBABILIDADES DO DEALER ===\n");
    
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    DealerProbabilities probs = get_dealer_probabilities(10, 0.0, &counter);
    
    printf("Dealer upcard: 10, True count: 0.0\n");
    printf("P(17): %.4f\n", probs.prob_17);
    printf("P(18): %.4f\n", probs.prob_18);
    printf("P(19): %.4f\n", probs.prob_19);
    printf("P(20): %.4f\n", probs.prob_20);
    printf("P(21): %.4f\n", probs.prob_21);
    printf("P(BJ): %.4f\n", probs.prob_blackjack);
    printf("P(BUST): %.4f\n", probs.prob_bust);
    
    double total = probs.prob_17 + probs.prob_18 + probs.prob_19 + 
                  probs.prob_20 + probs.prob_21 + probs.prob_blackjack + 
                  probs.prob_bust;
    printf("Total: %.4f\n", total);
    printf("Probabilidades válidas: %s\n", probs.probabilities_valid ? "Sim" : "Não");
}

int main() {
    test_debug_ev();
    test_dealer_probabilities();
    return 0;
} 