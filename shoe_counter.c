#include "shoe_counter.h"
#include "constantes.h"
#include <stdio.h>
#include <string.h>

// Nomes dos ranks para debug
static const char* rank_names[NUM_RANKS] = {
    "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"
};

// Valores dos ranks para cálculos de probabilidade
static const int rank_values[NUM_RANKS] = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10, 11
};

// =============== FUNÇÕES PRINCIPAIS ===============

void shoe_counter_init(ShoeCounter* counter, int num_decks) {
    if (!counter) return;
    
    counter->original_decks = num_decks;
    counter->total_cards = num_decks * 52;
    
    // Inicializar cada rank com 4 cartas por deck
    for (int i = 0; i < NUM_RANKS; i++) {
        counter->counts[i] = num_decks * 4;
    }
    
    counter->initialized = true;
    
    // printf("📊 ShoeCounter inicializado: %d decks, %d cartas totais\n", 
    //        num_decks, counter->total_cards);
}

void shoe_counter_remove_card(ShoeCounter* counter, Carta carta) {
    if (!counter || !counter->initialized) {
        fprintf(stderr, "ERRO: ShoeCounter não inicializado!\n");
        return;
    }
    
    // Obter rank da carta usando a função existente
    int rank_idx = carta_para_rank_idx(carta);
    
    if (rank_idx < 0 || rank_idx >= NUM_RANKS) {
        fprintf(stderr, "ERRO: Rank inválido: %d\n", rank_idx);
        return;
    }
    
    if (counter->counts[rank_idx] <= 0) {
        fprintf(stderr, "AVISO: Tentativa de remover carta já esgotada (rank %s)\n", 
                rank_names[rank_idx]);
        return;
    }
    
    counter->counts[rank_idx]--;
    counter->total_cards--;
}

void shoe_counter_reset(ShoeCounter* counter) {
    if (!counter) return;
    
    shoe_counter_init(counter, counter->original_decks);
}

// =============== FUNÇÕES DE CONSULTA ===============

int shoe_counter_get_rank_count(const ShoeCounter* counter, int rank_idx) {
    if (!counter || !counter->initialized || rank_idx < 0 || rank_idx >= NUM_RANKS) {
        return 0;
    }
    return counter->counts[rank_idx];
}

int shoe_counter_get_total_cards(const ShoeCounter* counter) {
    if (!counter || !counter->initialized) {
        return 0;
    }
    return counter->total_cards;
}

double shoe_counter_get_rank_probability(const ShoeCounter* counter, int rank_idx) {
    if (!counter || !counter->initialized || rank_idx < 0 || rank_idx >= NUM_RANKS) {
        return 0.0;
    }
    
    if (counter->total_cards <= 0) {
        return 0.0;
    }
    
    return (double)counter->counts[rank_idx] / (double)counter->total_cards;
}

double shoe_counter_get_specific_rank_probability(const ShoeCounter* counter, int rank_value) {
    int rank_idx = rank_value_to_idx(rank_value);
    if (rank_idx == -1) {
        return 0.0;
    }
    return shoe_counter_get_rank_probability(counter, rank_idx);
}

// =============== FUNÇÕES DE ANÁLISE ===============

int shoe_counter_get_ten_value_cards(const ShoeCounter* counter) {
    if (!counter || !counter->initialized) {
        return 0;
    }
    
    // Somar 10, J, Q, K (índices 8, 9, 10, 11)
    return counter->counts[8] + counter->counts[9] + 
           counter->counts[10] + counter->counts[11];
}

int shoe_counter_get_aces(const ShoeCounter* counter) {
    if (!counter || !counter->initialized) {
        return 0;
    }
    
    // Ás está no índice 12
    return counter->counts[12];
}

double shoe_counter_get_blackjack_probability(const ShoeCounter* counter) {
    if (!counter || !counter->initialized || counter->total_cards <= 1) {
        return 0.0;
    }
    
    int ten_cards = shoe_counter_get_ten_value_cards(counter);
    int aces = shoe_counter_get_aces(counter);
    
    // P(BJ) = P(A primeiro e 10 segundo) + P(10 primeiro e A segundo)
    // P(A primeiro e 10 segundo) = (aces/total) * (ten_cards/(total-1))
    // P(10 primeiro e A segundo) = (ten_cards/total) * (aces/(total-1))
    
    double total = (double)counter->total_cards;
    double prob1 = ((double)aces / total) * ((double)ten_cards / (total - 1.0));
    double prob2 = ((double)ten_cards / total) * ((double)aces / (total - 1.0));
    
    return prob1 + prob2;
}

double shoe_counter_get_bust_probability_on_hit(const ShoeCounter* counter, int current_hand_value) {
    if (!counter || !counter->initialized || counter->total_cards <= 0) {
        return 0.0;
    }
    
    if (current_hand_value >= 21) {
        return (current_hand_value > 21) ? 1.0 : 0.0; // Já estourou ou blackjack
    }
    
    int bust_cards = 0;
    
    // Contar cartas que causariam bust
    for (int rank_idx = 0; rank_idx < NUM_RANKS; rank_idx++) {
        int card_value = rank_values[rank_idx];
        
        // Para Ás, sempre usar valor 1 (nunca causa bust)
        if (rank_idx == 12) { // Ás
            card_value = 1;
        }
        
        if (current_hand_value + card_value > 21) {
            bust_cards += counter->counts[rank_idx];
        }
    }
    
    return (double)bust_cards / (double)counter->total_cards;
}

// =============== FUNÇÕES UTILITÁRIAS ===============

void shoe_counter_print_status(const ShoeCounter* counter) {
    if (!counter || !counter->initialized) {
        printf("❌ ShoeCounter não inicializado\n");
        return;
    }
    
    printf("\n📊 STATUS DO SHOE COUNTER\n");
    printf("=========================\n");
    printf("Decks originais: %d\n", counter->original_decks);
    printf("Cartas restantes: %d\n", counter->total_cards);
    printf("Penetração: %.1f%%\n", 
           100.0 * (1.0 - (double)counter->total_cards / (double)(counter->original_decks * 52)));
    
    printf("\nCONTAGENS POR RANK:\n");
    printf("Rank | Count | Prob(%%)\n");
    printf("-----|-------|-------\n");
    
    for (int i = 0; i < NUM_RANKS; i++) {
        double prob = shoe_counter_get_rank_probability(counter, i) * 100.0;
        printf(" %-3s |  %3d  | %5.1f\n", 
               rank_names[i], counter->counts[i], prob);
    }
    
    printf("\nANÁLISES ESPECIAIS:\n");
    printf("Cartas valor 10: %d (%.1f%%)\n", 
           shoe_counter_get_ten_value_cards(counter),
           100.0 * shoe_counter_get_ten_value_cards(counter) / counter->total_cards);
    printf("Ases: %d (%.1f%%)\n", 
           shoe_counter_get_aces(counter),
           100.0 * shoe_counter_get_aces(counter) / counter->total_cards);
    printf("Prob. Blackjack: %.4f%%\n", 
           shoe_counter_get_blackjack_probability(counter) * 100.0);
    printf("\n");
}

bool shoe_counter_validate(const ShoeCounter* counter) {
    if (!counter || !counter->initialized) {
        return false;
    }
    
    // Verificar se contagens são não-negativas
    for (int i = 0; i < NUM_RANKS; i++) {
        if (counter->counts[i] < 0) {
            fprintf(stderr, "ERRO: Contagem negativa para rank %s: %d\n", 
                    rank_names[i], counter->counts[i]);
            return false;
        }
    }
    
    // Verificar se total é consistente
    int sum = 0;
    for (int i = 0; i < NUM_RANKS; i++) {
        sum += counter->counts[i];
    }
    
    if (sum != counter->total_cards) {
        fprintf(stderr, "ERRO: Total inconsistente. Soma: %d, Total: %d\n", 
                sum, counter->total_cards);
        return false;
    }
    
    return true;
}

// =============== CONVERSÕES ===============

int rank_value_to_idx(int rank_value) {
    // Converter valor do rank (2-11) para índice (0-12)
    if (rank_value >= 2 && rank_value <= 9) {
        return rank_value - 2;  // 2->0, 3->1, ..., 9->7
    } else if (rank_value == 10) {
        return 8;  // 10 usa índice 8
    } else if (rank_value == 11) {
        return 12; // Ás usa índice 12
    }
    return -1; // Valor inválido
}

int rank_idx_to_value(int rank_idx) {
    // Converter índice (0-12) para valor do rank (2-11)
    if (rank_idx >= 0 && rank_idx <= 7) {
        return rank_idx + 2;  // 0->2, 1->3, ..., 7->9
    } else if (rank_idx >= 8 && rank_idx <= 11) {
        return 10;  // J, Q, K, 10 todos valem 10
    } else if (rank_idx == 12) {
        return 11;  // Ás
    }
    return -1; // Índice inválido
} 