#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "shoe_counter.h"
#include "real_time_ev.h"

// Teste para verificar se as correções funcionam
void test_corrections() {
    printf("=== TESTE DAS CORREÇÕES DO SISTEMA DE EV ===\n");
    
    // Criar um shoe counter com 1 deck
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    printf("Estado inicial:\n");
    printf("  Total de cartas: %d\n", counter.total_cards);
    printf("  Cartas de valor 10: %d\n", shoe_counter_get_ten_value_cards(&counter));
    
    // Testar a nova função get_card_probability
    printf("\n=== TESTE DA FUNÇÃO get_card_probability ===\n");
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        double prob = get_card_probability(&counter, card_rank);
        printf("  card_rank=%d, prob=%.4f (%.2f%%)\n", card_rank, prob, prob * 100);
    }
    
    // Verificar especificamente o valor 10
    double prob_ten = get_card_probability(&counter, 10);
    double expected_prob_ten = 16.0 / 52.0;
    printf("\nProbabilidade de valor 10:\n");
    printf("  Calculada: %.4f (%.2f%%)\n", prob_ten, prob_ten * 100);
    printf("  Esperada:  %.4f (%.2f%%)\n", expected_prob_ten, expected_prob_ten * 100);
    printf("  Diferença: %.4f\n", prob_ten - expected_prob_ten);
    printf("  Status: %s\n", (fabs(prob_ten - expected_prob_ten) < 0.0001) ? "✓ CORRETO" : "✗ INCORRETO");
    
    // Testar a nova função simulate_card_removal_corrected
    printf("\n=== TESTE DA FUNÇÃO simulate_card_removal_corrected ===\n");
    
    // Testar remoção de carta de valor 10
    ShoeCounter temp_counter = simulate_card_removal_corrected(&counter, 10);
    printf("Após remover carta de valor 10:\n");
    printf("  Total de cartas: %d\n", temp_counter.total_cards);
    printf("  Cartas de valor 10 restantes: %d\n", 
           temp_counter.counts[8] + temp_counter.counts[9] + temp_counter.counts[10] + temp_counter.counts[11]);
    
    // Verificar contagem individual
    for (int idx = 8; idx <= 11; idx++) {
        printf("  Índice %d: %d cartas\n", idx, temp_counter.counts[idx]);
    }
    
    // Verificar se a remoção foi feita corretamente
    int expected_remaining = 15; // 16 - 1
    int actual_remaining = temp_counter.counts[8] + temp_counter.counts[9] + temp_counter.counts[10] + temp_counter.counts[11];
    printf("  Esperado: %d cartas restantes\n", expected_remaining);
    printf("  Real: %d cartas restantes\n", actual_remaining);
    printf("  Status: %s\n", (actual_remaining == expected_remaining) ? "✓ CORRETO" : "✗ INCORRETO");
    
    // Testar remoção de outras cartas
    printf("\nTestando remoção de carta de valor 5:\n");
    ShoeCounter temp_counter2 = simulate_card_removal_corrected(&counter, 5);
    printf("  Total de cartas: %d\n", temp_counter2.total_cards);
    printf("  Cartas de valor 5 restantes: %d\n", temp_counter2.counts[3]); // índice 3 = valor 5
    
    // Testar remoção de Ás
    printf("\nTestando remoção de Ás:\n");
    ShoeCounter temp_counter3 = simulate_card_removal_corrected(&counter, 11);
    printf("  Total de cartas: %d\n", temp_counter3.total_cards);
    printf("  Áses restantes: %d\n", temp_counter3.counts[12]); // índice 12 = Ás
}

// Teste para verificar se o sistema de EV agora calcula probabilidades corretas
void test_ev_probabilities_corrected() {
    printf("\n=== TESTE DE PROBABILIDADES CORRIGIDAS ===\n");
    
    // Criar um shoe counter com 1 deck
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    int total_cards = shoe_counter_get_total_cards(&counter);
    
    // Simular o que o sistema de EV corrigido faria
    printf("Simulando iteração do sistema de EV corrigido:\n");
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        double prob = get_card_probability(&counter, card_rank);
        printf("  card_rank=%d, prob=%.4f (%.2f%%)\n", card_rank, prob, prob * 100);
    }
    
    // Verificar se a soma das probabilidades é 1.0
    double total_prob = 0.0;
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        total_prob += get_card_probability(&counter, card_rank);
    }
    printf("\nSoma das probabilidades: %.4f\n", total_prob);
    printf("Status: %s\n", (fabs(total_prob - 1.0) < 0.0001) ? "✓ CORRETO" : "✗ INCORRETO");
}

int main() {
    test_corrections();
    test_ev_probabilities_corrected();
    return 0;
} 