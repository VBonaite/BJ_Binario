#include "shoe_counter.h"
#include "baralho.h"
#include "constantes.h"
#include <stdio.h>

/**
 * EXEMPLO DE INTEGRAÇÃO DO SHOE COUNTER AO SIMULADOR PRINCIPAL
 * 
 * Este exemplo mostra como integrar o sistema de contagem de cartas
 * ao simulador de blackjack existente para análises em tempo real.
 */

// Simulação simplificada de uma rodada de blackjack
void simular_rodada_com_probabilidades(ShoeCounter* counter, Shoe* shoe) {
    printf("\n🎲 SIMULANDO RODADA COM ANÁLISE DE PROBABILIDADES\n");
    printf("================================================\n");
    
    // Estado inicial
    printf("📊 Estado antes da distribuição:\n");
    printf("   Cartas restantes: %d\n", shoe_counter_get_total_cards(counter));
    printf("   Prob. Blackjack: %.2f%%\n", 
           shoe_counter_get_blackjack_probability(counter) * 100.0);
    printf("   Cartas valor 10: %d (%.1f%%)\n", 
           shoe_counter_get_ten_value_cards(counter),
           100.0 * shoe_counter_get_ten_value_cards(counter) / shoe_counter_get_total_cards(counter));
    
    // Distribuir cartas para 4 jogadores + dealer upcard
    printf("\n🎴 Distribuindo cartas:\n");
    
    uint64_t jogador_maos[4] = {0}; // Simplified player hands
    uint64_t dealer_mao = 0;
    
    // Primeira rodada - jogadores
    for (int i = 0; i < 4; i++) {
        Carta carta = baralho_comprar(shoe);
        shoe_counter_remove_card(counter, carta);
        jogador_maos[i] |= carta; // Simplified card addition
        printf("   Jogador %d recebe: %c\n", i+1, carta_para_char(carta));
    }
    
    // Dealer upcard
    Carta dealer_upcard = baralho_comprar(shoe);
    shoe_counter_remove_card(counter, dealer_upcard);
    dealer_mao |= dealer_upcard;
    printf("   Dealer upcard: %c\n", carta_para_char(dealer_upcard));
    
    // Segunda rodada - jogadores
    for (int i = 0; i < 4; i++) {
        Carta carta = baralho_comprar(shoe);
        shoe_counter_remove_card(counter, carta);
        jogador_maos[i] |= carta;
        printf("   Jogador %d recebe: %c\n", i+1, carta_para_char(carta));
    }
    
    // Análise após distribuição inicial
    printf("\n📈 Análise pós-distribuição:\n");
    printf("   Cartas restantes: %d\n", shoe_counter_get_total_cards(counter));
    printf("   Prob. Blackjack: %.2f%%\n", 
           shoe_counter_get_blackjack_probability(counter) * 100.0);
    
    // Análise de probabilidades para diferentes totais de mão
    printf("\n🎯 Probabilidades de bust ao pedir carta:\n");
    for (int total = 12; total <= 20; total++) {
        double bust_prob = shoe_counter_get_bust_probability_on_hit(counter, total);
        printf("   Com %d: %.1f%%\n", total, bust_prob * 100.0);
    }
    
    // Dealer hole card (não contabilizado ainda)
    Carta dealer_hole = baralho_comprar(shoe);
    printf("\n🔒 Dealer recebe hole card: %c (não contabilizada)\n", 
           carta_para_char(dealer_hole));
    
    // Agora contabilizar hole card
    shoe_counter_remove_card(counter, dealer_hole);
    dealer_mao |= dealer_hole;
    
    printf("\n📊 Estado final da rodada:\n");
    printf("   Cartas restantes: %d\n", shoe_counter_get_total_cards(counter));
    printf("   Penetração do shoe: %.1f%%\n", 
           100.0 * (1.0 - (double)shoe_counter_get_total_cards(counter) / (8.0 * 52.0)));
}

// Exemplo de análise de composition-dependent strategy
void exemplo_composition_dependent_analysis(ShoeCounter* counter) {
    printf("\n🧮 ANÁLISE COMPOSITION-DEPENDENT\n");
    printf("=================================\n");
    
    // Exemplo: analisar diferença entre 16 = 10+6 vs 16 = 9+7
    printf("Cenário: Jogador com 16, dealer mostra 10\n\n");
    
    // Simular remoção de cartas específicas
    ShoeCounter temp_counter1 = *counter;
    ShoeCounter temp_counter2 = *counter;
    
    // Cenário 1: 16 = 10 + 6 (remove um 10 e um 6)
    temp_counter1.counts[8]--;  // Remove um 10 (índice 8)
    temp_counter1.counts[4]--;  // Remove um 6 (índice 4)
    temp_counter1.total_cards -= 2;
    
    // Cenário 2: 16 = 9 + 7 (remove um 9 e um 7)
    temp_counter2.counts[7]--;  // Remove um 9 (índice 7)
    temp_counter2.counts[5]--;  // Remove um 7 (índice 5)
    temp_counter2.total_cards -= 2;
    
    double bust_prob1 = shoe_counter_get_bust_probability_on_hit(&temp_counter1, 16);
    double bust_prob2 = shoe_counter_get_bust_probability_on_hit(&temp_counter2, 16);
    
    printf("16 = 10+6: Prob. bust = %.2f%%\n", bust_prob1 * 100.0);
    printf("16 = 9+7:  Prob. bust = %.2f%%\n", bust_prob2 * 100.0);
    printf("Diferença: %.2f pontos percentuais\n", (bust_prob2 - bust_prob1) * 100.0);
    
    if (bust_prob1 > bust_prob2) {
        printf("→ 16=10+6 tem maior probabilidade de bust (menos 6s restantes)\n");
    } else {
        printf("→ 16=9+7 tem maior probabilidade de bust (menos 7s restantes)\n");
    }
}

// Exemplo de como usar para decisões de apostas
void exemplo_betting_decisions(const ShoeCounter* counter) {
    printf("\n💰 DECISÕES DE APOSTAS BASEADAS EM COMPOSIÇÃO\n");
    printf("=============================================\n");
    
    double ace_ratio = (double)shoe_counter_get_aces(counter) / shoe_counter_get_total_cards(counter);
    double ten_ratio = (double)shoe_counter_get_ten_value_cards(counter) / shoe_counter_get_total_cards(counter);
    double normal_ace_ratio = 1.0 / 13.0;  // 7.69%
    double normal_ten_ratio = 4.0 / 13.0;  // 30.77%
    
    printf("Concentração de Ases: %.2f%% (normal: %.2f%%)\n", 
           ace_ratio * 100.0, normal_ace_ratio * 100.0);
    printf("Concentração de 10s: %.2f%% (normal: %.2f%%)\n", 
           ten_ratio * 100.0, normal_ten_ratio * 100.0);
    
    double bj_prob = shoe_counter_get_blackjack_probability(counter);
    double normal_bj_prob = 4.0 / 13.0 * 1.0 / 12.0 * 2.0; // Aproximação
    
    printf("\nProb. Blackjack: %.3f%% (normal: %.3f%%)\n", 
           bj_prob * 100.0, normal_bj_prob * 100.0);
    
    if (bj_prob > normal_bj_prob * 1.1) {
        printf("🟢 RECOMENDAÇÃO: Aumentar aposta (maior prob. de BJ)\n");
    } else if (bj_prob < normal_bj_prob * 0.9) {
        printf("🔴 RECOMENDAÇÃO: Diminuir aposta (menor prob. de BJ)\n");
    } else {
        printf("🟡 RECOMENDAÇÃO: Manter aposta (prob. normal)\n");
    }
}

int main() {
    printf("🃏 EXEMPLO DE INTEGRAÇÃO DO SHOE COUNTER\n");
    printf("========================================\n");
    
    // Inicializar sistemas
    ShoeCounter counter;
    shoe_counter_init(&counter, 8);
    
    Shoe shoe;
    baralho_criar(&shoe);
    baralho_embaralhar(&shoe);
    
    // Simular algumas rodadas
    for (int rodada = 1; rodada <= 3; rodada++) {
        printf("\n\n🎯 RODADA %d\n", rodada);
        printf("===========\n");
        
        simular_rodada_com_probabilidades(&counter, &shoe);
        
        if (rodada == 2) {
            exemplo_composition_dependent_analysis(&counter);
        }
        
        exemplo_betting_decisions(&counter);
    }
    
    printf("\n\n📋 STATUS FINAL DO SHOE\n");
    printf("=======================\n");
    shoe_counter_print_status(&counter);
    
    // Cleanup
    baralho_destruir(&shoe);
    
    printf("\n💡 PONTOS DE INTEGRAÇÃO NO SIMULADOR PRINCIPAL:\n");
    printf("===============================================\n");
    printf("1. Inicializar ShoeCounter junto com o Shoe\n");
    printf("2. Chamar shoe_counter_remove_card() a cada carta distribuída\n");
    printf("3. Usar probabilidades para decisões estratégicas\n");
    printf("4. Implementar composition-dependent basic strategy\n");
    printf("5. Otimizar apostas baseadas na composição do shoe\n");
    printf("6. Resetar counter a cada novo shoe\n\n");
    
    return 0;
} 