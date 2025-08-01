#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "shoe_counter.h"
#include "real_time_ev.h"
#include "jogo.h"

// Teste específico para verificar o cálculo do EV Hit
void test_ev_hit_calculation() {
    printf("=== TESTE ESPECÍFICO DO EV HIT ===\n");
    
    // Criar um shoe counter com 1 deck
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    // Testar mão 8 vs dealer 10 (deveria ser hit claramente)
    uint64_t hand_bits = 0;
    hand_bits |= (1ULL << (6 * 3)); // 8
    hand_bits |= (1ULL << (0 * 3)); // 2 (8+2=10, mas vamos testar 8+2=10)
    
    printf("Testando mão 8 vs dealer 10:\n");
    printf("Mão: 8, 2 (valor 10)\n");
    
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
        
        printf("\nAnálise:\n");
        if (result.ev_hit > result.ev_stand) {
            printf("  ✓ EV Hit > EV Stand (esperado para 10 vs 10)\n");
        } else {
            printf("  ✗ EV Hit <= EV Stand (inesperado para 10 vs 10)\n");
        }
        
        printf("  Diferença EV Hit - EV Stand: %.6f\n", result.ev_hit - result.ev_stand);
    }
    
    // Testar mão 12 vs dealer 10
    printf("\n=== TESTE: 12 vs 10 ===\n");
    uint64_t hand_12 = 0;
    hand_12 |= (1ULL << (8 * 3)); // 10
    hand_12 |= (1ULL << (0 * 3)); // 2
    
    RealTimeEVResult result2 = calculate_real_time_ev(
        hand_12, 10, 0.0, &counter, true, true, true
    );
    
    if (result2.calculation_valid) {
        printf("\nResultados:\n");
        printf("  EV Stand: %.6f\n", result2.ev_stand);
        printf("  EV Hit: %.6f\n", result2.ev_hit);
        printf("  EV Double: %.6f\n", result2.ev_double);
        printf("  Melhor ação: %c\n", result2.best_action);
        printf("  Melhor EV: %.6f\n", result2.best_ev);
        
        printf("\nAnálise:\n");
        printf("  Diferença EV Hit - EV Stand: %.6f\n", result2.ev_hit - result2.ev_stand);
    }
}

// Teste para verificar as probabilidades de cartas individuais
void test_individual_card_probabilities() {
    printf("\n=== TESTE DE PROBABILIDADES INDIVIDUAIS ===\n");
    
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    printf("Probabilidades de cartas individuais:\n");
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        double prob = get_card_probability(&counter, card_rank);
        printf("  card_rank=%d: %.4f (%.2f%%)\n", card_rank, prob, prob * 100);
    }
    
    // Verificar se a soma é 1.0
    double total = 0.0;
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        total += get_card_probability(&counter, card_rank);
    }
    printf("Soma total: %.4f\n", total);
    printf("Status: %s\n", (fabs(total - 1.0) < 0.0001) ? "✓ CORRETO" : "✗ INCORRETO");
}

int main() {
    test_ev_hit_calculation();
    test_individual_card_probabilities();
    return 0;
} 