#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "shoe_counter.h"
#include "real_time_ev.h"
#include "jogo.h"

// Teste simples para verificar o sistema de EV corrigido
void test_simple_ev() {
    printf("=== TESTE SIMPLES DO SISTEMA DE EV CORRIGIDO ===\n");
    
    // Criar um shoe counter com 1 deck
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    // Criar uma mão simples (10, 6 = 16)
    uint64_t hand_bits = 0;
    // Adicionar 10 (índice 8)
    hand_bits |= (1ULL << (8 * 3));
    // Adicionar 6 (índice 4)
    hand_bits |= (1ULL << (4 * 3));
    
    printf("Mão: 10, 6 (valor 16)\n");
    printf("Dealer upcard: 10\n");
    printf("True count: 0.0\n");
    
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
        
        // Verificar se o resultado faz sentido
        printf("\nAnálise:\n");
        if (result.ev_hit > result.ev_stand) {
            printf("  ✓ EV Hit > EV Stand (esperado para 16 vs 10)\n");
        } else {
            printf("  ✗ EV Hit <= EV Stand (inesperado para 16 vs 10)\n");
        }
        
        if (result.best_action == 'H' || result.best_action == 'D') {
            printf("  ✓ Melhor ação é Hit ou Double (esperado)\n");
        } else {
            printf("  ✗ Melhor ação é Stand (inesperado para 16 vs 10)\n");
        }
        
        printf("  ✓ Cálculo válido\n");
    } else {
        printf("  ✗ Cálculo inválido\n");
    }
}

// Teste para verificar se as probabilidades estão corretas
void test_probability_consistency() {
    printf("\n=== TESTE DE CONSISTÊNCIA DE PROBABILIDADES ===\n");
    
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    // Verificar se a soma das probabilidades é 1.0
    double total_prob = 0.0;
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        double prob = get_card_probability(&counter, card_rank);
        total_prob += prob;
        printf("  card_rank=%d: %.4f\n", card_rank, prob);
    }
    
    printf("Soma total: %.4f\n", total_prob);
    printf("Status: %s\n", (fabs(total_prob - 1.0) < 0.0001) ? "✓ CORRETO" : "✗ INCORRETO");
    
    // Verificar especificamente o valor 10
    double prob_ten = get_card_probability(&counter, 10);
    printf("Probabilidade de valor 10: %.4f (%.2f%%)\n", prob_ten, prob_ten * 100);
    printf("Esperado: %.4f (%.2f%%)\n", 16.0/52.0, (16.0/52.0)*100);
}

int main() {
    test_simple_ev();
    test_probability_consistency();
    return 0;
} 