#include "ev_calculator.h"
#include "shoe_counter.h"
#include "dealer_freq_lookup.h"
#include <stdio.h>
#include <math.h>

// ====================== CÁLCULO DE EV STAND ======================

double calculate_ev_stand(int player_total, int dealer_upcard, const ShoeCounter* counter) {
    /*
     * EV de STAND = Soma das probabilidades de cada resultado final do dealer
     * multiplicada pelo payoff contra esse resultado.
     * 
     * Se jogador tem 12 e fica:
     * - Se dealer bust: +1 (jogador ganha)
     * - Se dealer 17-21: -1 (jogador perde)
     * - Se dealer 12-16: Impossível (dealer sempre continua)
     */
    
    printf("  📊 Calculando EV de STAND com %d vs dealer %d:\n", player_total, dealer_upcard);
    
    double ev_stand = 0.0;
    
    // Probabilidade do dealer estourar
    double prob_dealer_bust = get_dealer_bust_probability(dealer_upcard, counter);
    double payoff_dealer_bust = 1.0; // Jogador ganha
    ev_stand += prob_dealer_bust * payoff_dealer_bust;
    
    printf("    Dealer bust: %.4f × %.1f = %.4f\n", 
           prob_dealer_bust, payoff_dealer_bust, prob_dealer_bust * payoff_dealer_bust);
    
    // Probabilidades do dealer terminar em 17, 18, 19, 20, 21
    for (int dealer_final = 17; dealer_final <= 21; dealer_final++) {
        double prob_dealer_final = get_dealer_final_probability(dealer_upcard, dealer_final, counter);
        double payoff = -1.0; // Jogador sempre perde (12 < 17-21)
        ev_stand += prob_dealer_final * payoff;
        
        printf("    Dealer %d: %.4f × %.1f = %.4f\n", 
               dealer_final, prob_dealer_final, payoff, prob_dealer_final * payoff);
    }
    
    printf("    EV_STAND total = %.6f\n", ev_stand);
    return ev_stand;
}

// ====================== CÁLCULO DE EV HIT ======================

double calculate_ev_hit(int player_total, int dealer_upcard, const ShoeCounter* counter, int depth) {
    /*
     * EV de HIT = Soma sobre todas as cartas possíveis:
     * P(carta) × EV_ótimo_após_receber_carta
     * 
     * Para cada carta possível:
     * - Se nova mão > 21: EV = -1 (bust)
     * - Se nova mão <= 21: EV = max(EV_stand_nova_mão, EV_hit_nova_mão)
     */
    
    if (depth > MAX_RECURSION_DEPTH) {
        printf("    ⚠️  Limite de recursão atingido\n");
        return -1.0; // Conservador
    }
    
    printf("  📊 Calculando EV de HIT com %d vs dealer %d (depth=%d):\n", 
           player_total, dealer_upcard, depth);
    
    double ev_hit = 0.0;
    double total_probability = 0.0;
    
    // Iterar sobre todos os ranks possíveis (2-A)
    for (int rank_idx = 0; rank_idx < NUM_RANKS; rank_idx++) {
        int count = shoe_counter_get_rank_count(counter, rank_idx);
        if (count <= 0) continue;
        
        // Probabilidade de receber esta carta
        double prob_card = shoe_counter_get_rank_probability(counter, rank_idx);
        total_probability += prob_card;
        
        // Calcular novo total após receber esta carta
        int card_value = rank_idx_to_value(rank_idx);
        if (card_value == 11 && player_total + 11 > 21) {
            card_value = 1; // Ás como 1 se 11 causaria bust
        }
        
        int new_total = player_total + card_value;
        double ev_after_card;
        
        if (new_total > 21) {
            // Bust - EV = -1
            ev_after_card = -1.0;
            printf("    Carta %s (%d): %.4f × -1.000 = %.6f (BUST)\n",
                   (rank_idx <= 7) ? "" : (rank_idx <= 11) ? "10" : "A",
                   card_value, prob_card, prob_card * ev_after_card);
        } else {
            // Não bust - calcular EV ótimo da nova situação
            // Simular remoção da carta para cálculo recursivo
            ShoeCounter temp_counter = *counter;
            temp_counter.counts[rank_idx]--;
            temp_counter.total_cards--;
            
            // EV ótimo = max(EV_stand, EV_hit) da nova mão
            double ev_stand_new = calculate_ev_stand(new_total, dealer_upcard, &temp_counter);
            double ev_hit_new = (new_total < 21) ? 
                calculate_ev_hit(new_total, dealer_upcard, &temp_counter, depth + 1) : ev_stand_new;
            
            ev_after_card = (ev_stand_new > ev_hit_new) ? ev_stand_new : ev_hit_new;
            char action = (ev_stand_new > ev_hit_new) ? 'S' : 'H';
            
            printf("    Carta %s (%d → %d): %.4f × %.6f = %.6f (%c)\n",
                   (rank_idx <= 7) ? "" : (rank_idx <= 11) ? "10" : "A",
                   card_value, new_total, prob_card, ev_after_card, prob_card * ev_after_card, action);
        }
        
        ev_hit += prob_card * ev_after_card;
    }
    
    printf("    EV_HIT total = %.6f (prob_total=%.6f)\n", ev_hit, total_probability);
    return ev_hit;
}

// ====================== CÁLCULO DE EV DOUBLE ======================

double calculate_ev_double(int player_total, int dealer_upcard, const ShoeCounter* counter) {
    /*
     * EV de DOUBLE = 2 × (EV de pegar exatamente uma carta e parar)
     * É similar ao HIT, mas:
     * 1. Só pega uma carta
     * 2. Multiplica o resultado por 2 (aposta dobrada)
     */
    
    printf("  📊 Calculando EV de DOUBLE com %d vs dealer %d:\n", player_total, dealer_upcard);
    
    double ev_double = 0.0;
    
    for (int rank_idx = 0; rank_idx < NUM_RANKS; rank_idx++) {
        int count = shoe_counter_get_rank_count(counter, rank_idx);
        if (count <= 0) continue;
        
        double prob_card = shoe_counter_get_rank_probability(counter, rank_idx);
        
        int card_value = rank_idx_to_value(rank_idx);
        if (card_value == 11 && player_total + 11 > 21) {
            card_value = 1;
        }
        
        int new_total = player_total + card_value;
        double ev_after_card;
        
        if (new_total > 21) {
            ev_after_card = -1.0;
        } else {
            // Simular remoção da carta
            ShoeCounter temp_counter = *counter;
            temp_counter.counts[rank_idx]--;
            temp_counter.total_cards--;
            
            // Para double, sempre fica após receber a carta
            ev_after_card = calculate_ev_stand(new_total, dealer_upcard, &temp_counter);
        }
        
        ev_double += prob_card * ev_after_card;
    }
    
    // Multiplicar por 2 porque a aposta é dobrada
    ev_double *= 2.0;
    
    printf("    EV_DOUBLE total = %.6f\n", ev_double);
    return ev_double;
}

// ====================== FUNÇÕES AUXILIARES ======================

double get_dealer_bust_probability(int dealer_upcard, const ShoeCounter* counter) {
    // Simplificação: usar probabilidade baseada na composição atual
    // Em implementação completa, usaria as lookup tables de frequência
    
    // Aproximação baseada em probabilidades conhecidas
    double bust_probs[] = {0.35, 0.37, 0.40, 0.42, 0.42, 0.26, 0.24, 0.23, 0.23, 0.17};
    int upcard_idx = dealer_upcard - 2;
    if (upcard_idx < 0 || upcard_idx > 9) return 0.25;
    
    // Ajustar baseado na composição do shoe (simplificado)
    double base_prob = bust_probs[upcard_idx];
    
    // Fator de ajuste baseado na concentração de cartas altas
    int high_cards = shoe_counter_get_ten_value_cards(counter);
    double high_concentration = (double)high_cards / shoe_counter_get_total_cards(counter);
    double normal_high_concentration = 4.0 / 13.0;
    double adjustment = (high_concentration / normal_high_concentration) - 1.0;
    
    return base_prob + (adjustment * 0.1); // Ajuste de até 10%
}

double get_dealer_final_probability(int dealer_upcard, int final_total, const ShoeCounter* counter) {
    // Simplificação: distribuição aproximada baseada em estudos conhecidos
    // Em implementação completa, usaria as lookup tables reais
    
    double probs[5] = {0.13, 0.13, 0.13, 0.18, 0.12}; // Para 17,18,19,20,21
    int idx = final_total - 17;
    if (idx < 0 || idx > 4) return 0.0;
    
    // Normalizar para que soma com bust prob = 1.0
    double bust_prob = get_dealer_bust_probability(dealer_upcard, counter);
    double non_bust_prob = 1.0 - bust_prob;
    
    return probs[idx] * non_bust_prob / 0.69; // 0.69 = soma das probs base
}

// ====================== FUNÇÃO PRINCIPAL ======================

EVResult calculate_all_evs(int player_total, int dealer_upcard, const ShoeCounter* counter) {
    EVResult result = {0};
    
    printf("\n🎯 CALCULANDO EXPECTED VALUES\n");
    printf("=============================\n");
    printf("Jogador: %d | Dealer upcard: %d\n", player_total, dealer_upcard);
    printf("Cartas restantes: %d\n\n", shoe_counter_get_total_cards(counter));
    
    // Calcular EVs de todas as ações
    result.ev_stand = calculate_ev_stand(player_total, dealer_upcard, counter);
    result.ev_hit = calculate_ev_hit(player_total, dealer_upcard, counter, 1);
    result.ev_double = calculate_ev_double(player_total, dealer_upcard, counter);
    
    // Determinar melhor ação
    result.best_ev = result.ev_stand;
    result.best_action = 'S';
    
    if (result.ev_hit > result.best_ev) {
        result.best_ev = result.ev_hit;
        result.best_action = 'H';
    }
    
    if (result.ev_double > result.best_ev) {
        result.best_ev = result.ev_double;
        result.best_action = 'D';
    }
    
    result.calculation_valid = true;
    
    // Resumo
    printf("\n📋 RESUMO DOS EXPECTED VALUES:\n");
    printf("==============================\n");
    printf("EV(Stand):  %+.6f\n", result.ev_stand);
    printf("EV(Hit):    %+.6f\n", result.ev_hit);
    printf("EV(Double): %+.6f\n", result.ev_double);
    printf("\n🎯 MELHOR AÇÃO: %c (EV = %+.6f)\n", result.best_action, result.best_ev);
    
    return result;
}

// ====================== DEMONSTRAÇÃO ======================

void demonstrate_ev_calculation_for_12(int dealer_upcard) {
    printf("🃏 DEMONSTRAÇÃO: CÁLCULO DE EV PARA MÃO DE 12\n");
    printf("=============================================\n\n");
    
    // Inicializar shoe counter com composição completa
    ShoeCounter counter;
    shoe_counter_init(&counter, 8);
    
    printf("Cenário: Jogador tem 12, dealer mostra %d\n", dealer_upcard);
    printf("Shoe: 8 decks completos (%d cartas)\n\n", shoe_counter_get_total_cards(&counter));
    
    // Calcular todos os EVs
    EVResult result = calculate_all_evs(12, dealer_upcard, &counter);
    
    printf("\n💡 INTERPRETAÇÃO:\n");
    printf("=================\n");
    
    if (result.best_action == 'S') {
        printf("A melhor ação é STAND porque:\n");
        printf("- A probabilidade de bust ao pedir carta é alta\n");
        printf("- É melhor apostar na chance do dealer estourar\n");
    } else if (result.best_action == 'H') {
        printf("A melhor ação é HIT porque:\n");
        printf("- A chance de melhorar a mão compensa o risco de bust\n");
        printf("- O dealer tem alta probabilidade de fazer uma boa mão\n");
    } else {
        printf("A melhor ação é DOUBLE porque:\n");
        printf("- A situação é muito favorável ao jogador\n");
        printf("- Vale a pena arriscar o dobro da aposta\n");
    }
    
    printf("\nDiferença EV(Hit) - EV(Stand): %+.6f\n", result.ev_hit - result.ev_stand);
    if (fabs(result.ev_hit - result.ev_stand) < 0.001) {
        printf("→ As duas ações são praticamente equivalentes!\n");
    }
} 