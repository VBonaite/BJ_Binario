#include <stdio.h>
#include <stdlib.h>
#include "shoe_counter.h"
#include "real_time_ev.h"

// Teste para verificar como o sistema de EV calcula probabilidades
void test_ev_probabilities() {
    printf("=== TESTE DE PROBABILIDADES NO SISTEMA DE EV ===\n");
    
    // Criar um shoe counter com 1 deck
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    printf("Total de cartas: %d\n", counter.total_cards);
    
    // Verificar como o sistema de EV itera sobre as cartas
    printf("\n=== ITERAÇÃO DO SISTEMA DE EV ===\n");
    int total_cards_available = shoe_counter_get_total_cards(&counter);
    
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        int rank_idx = rank_value_to_idx(card_rank);
        if (rank_idx < 0) {
            printf("rank_value_to_idx(%d) = %d (INVÁLIDO)\n", card_rank, rank_idx);
            continue;
        }
        
        int available_cards = shoe_counter_get_rank_count(&counter, rank_idx);
        double card_prob = (double)available_cards / total_cards_available;
        
        printf("card_rank=%d, rank_idx=%d, available_cards=%d, prob=%.4f (%.2f%%)\n", 
               card_rank, rank_idx, available_cards, card_prob, card_prob * 100);
    }
    
    // Verificar especificamente o problema do valor 10
    printf("\n=== PROBLEMA DO VALOR 10 ===\n");
    int ten_value_cards = shoe_counter_get_ten_value_cards(&counter);
    double prob_ten_correct = (double)ten_value_cards / total_cards_available;
    printf("Probabilidade correta de valor 10: %.4f (%.2f%%)\n", prob_ten_correct, prob_ten_correct * 100);
    
    // Verificar como o sistema de EV calcula a probabilidade de valor 10
    double prob_ten_ev = 0.0;
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        if (card_rank == 10) {
            int rank_idx = rank_value_to_idx(card_rank);
            int available_cards = shoe_counter_get_rank_count(&counter, rank_idx);
            prob_ten_ev += (double)available_cards / total_cards_available;
        }
    }
    printf("Probabilidade de valor 10 no sistema EV: %.4f (%.2f%%)\n", prob_ten_ev, prob_ten_ev * 100);
    
    // Verificar a diferença
    printf("Diferença: %.4f (%.2f%%)\n", prob_ten_ev - prob_ten_correct, (prob_ten_ev - prob_ten_correct) * 100);
    
    // Verificar se o problema está na função simulate_card_removal
    printf("\n=== TESTE DE SIMULAÇÃO DE REMOÇÃO ===\n");
    ShoeCounter temp_counter = simulate_card_removal(&counter, 10);
    printf("Após remover carta de valor 10:\n");
    printf("  Total de cartas: %d\n", temp_counter.total_cards);
    printf("  Cartas de valor 10 restantes: %d\n", 
           temp_counter.counts[8] + temp_counter.counts[9] + temp_counter.counts[10] + temp_counter.counts[11]);
    
    // Verificar contagem individual após remoção
    for (int idx = 8; idx <= 11; idx++) {
        printf("  Índice %d: %d cartas\n", idx, temp_counter.counts[idx]);
    }
}

// Teste para verificar o problema específico mencionado no relatório
void test_specific_problem() {
    printf("\n=== TESTE DO PROBLEMA ESPECÍFICO ===\n");
    
    // Criar um shoe counter com 1 deck
    ShoeCounter counter;
    shoe_counter_init(&counter, 1);
    
    printf("Estado inicial:\n");
    printf("  Total de cartas: %d\n", counter.total_cards);
    printf("  Cartas de valor 10: %d\n", shoe_counter_get_ten_value_cards(&counter));
    
    // Simular o que acontece no sistema de EV
    printf("\nSimulando iteração do sistema de EV:\n");
    int total_cards = shoe_counter_get_total_cards(&counter);
    
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        int rank_idx = rank_value_to_idx(card_rank);
        int available_cards = shoe_counter_get_rank_count(&counter, rank_idx);
        double prob = (double)available_cards / total_cards;
        
        printf("  card_rank=%d, rank_idx=%d, cards=%d, prob=%.4f\n", 
               card_rank, rank_idx, available_cards, prob);
    }
    
    // Verificar se o problema está na contagem de cartas de valor 10
    printf("\nVerificação de cartas de valor 10:\n");
    int ten_cards_manual = 0;
    for (int rank_idx = 8; rank_idx <= 11; rank_idx++) {
        ten_cards_manual += counter.counts[rank_idx];
        printf("  rank_idx=%d: %d cartas\n", rank_idx, counter.counts[rank_idx]);
    }
    printf("  Total manual: %d\n", ten_cards_manual);
    printf("  Total função: %d\n", shoe_counter_get_ten_value_cards(&counter));
}

int main() {
    test_ev_probabilities();
    test_specific_problem();
    return 0;
} 