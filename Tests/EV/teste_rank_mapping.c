#include <stdio.h>
#include <stdlib.h>
#include "shoe_counter.h"

// Teste para verificar o problema de mapeamento de ranks
void test_rank_mapping() {
    printf("=== TESTE DE MAPEAMENTO DE RANKS ===\n");
    
    // Testar rank_value_to_idx para valor 10
    printf("rank_value_to_idx(10) = %d\n", rank_value_to_idx(10));
    
    // Testar rank_value_to_idx para todos os valores
    for (int rank_value = 2; rank_value <= 11; rank_value++) {
        int idx = rank_value_to_idx(rank_value);
        printf("rank_value_to_idx(%d) = %d\n", rank_value, idx);
    }
    
    // Testar rank_idx_to_value para todos os índices
    for (int rank_idx = 0; rank_idx < NUM_RANKS; rank_idx++) {
        int value = rank_idx_to_value(rank_idx);
        printf("rank_idx_to_value(%d) = %d\n", rank_idx, value);
    }
    
    // Testar mapeamento reverso
    printf("\n=== TESTE DE MAPEAMENTO REVERSO ===\n");
    for (int rank_value = 2; rank_value <= 11; rank_value++) {
        int idx = rank_value_to_idx(rank_value);
        int value_back = rank_idx_to_value(idx);
        printf("rank_value_to_idx(%d) = %d, rank_idx_to_value(%d) = %d %s\n", 
               rank_value, idx, idx, value_back, 
               (rank_value == value_back) ? "✓" : "✗");
    }
    
    // Testar especificamente o problema do valor 10
    printf("\n=== PROBLEMA DO VALOR 10 ===\n");
    printf("rank_value_to_idx(10) = %d\n", rank_value_to_idx(10));
    printf("rank_idx_to_value(8) = %d\n", rank_idx_to_value(8));
    printf("rank_idx_to_value(9) = %d\n", rank_idx_to_value(9));
    printf("rank_idx_to_value(10) = %d\n", rank_idx_to_value(10));
    printf("rank_idx_to_value(11) = %d\n", rank_idx_to_value(11));
    
    // Verificar se todos os índices 8-11 mapeiam para valor 10
    for (int idx = 8; idx <= 11; idx++) {
        int value = rank_idx_to_value(idx);
        printf("Índice %d -> valor %d %s\n", idx, value, 
               (value == 10) ? "✓" : "✗");
    }
}

// Teste para verificar probabilidades de cartas
void test_card_probabilities() {
    printf("\n=== TESTE DE PROBABILIDADES ===\n");
    
    // Criar um shoe counter com 1 deck
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    printf("Total de cartas: %d\n", counter.total_cards);
    printf("Cartas de valor 10 (índices 8-11): %d\n", 
           counter.counts[8] + counter.counts[9] + counter.counts[10] + counter.counts[11]);
    
    // Verificar contagem individual
    for (int idx = 8; idx <= 11; idx++) {
        printf("Índice %d: %d cartas\n", idx, counter.counts[idx]);
    }
    
    // Calcular probabilidade de valor 10
    int ten_value_cards = shoe_counter_get_ten_value_cards(&counter);
    double prob_ten = (double)ten_value_cards / counter.total_cards;
    printf("Probabilidade de valor 10: %.4f (%.2f%%)\n", prob_ten, prob_ten * 100);
    
    // Verificar se a probabilidade está correta (16/52 ≈ 0.3077)
    double expected_prob = 16.0 / 52.0;
    printf("Probabilidade esperada: %.4f (%.2f%%)\n", expected_prob, expected_prob * 100);
    printf("Diferença: %.4f\n", prob_ten - expected_prob);
}

int main() {
    test_rank_mapping();
    test_card_probabilities();
    return 0;
} 