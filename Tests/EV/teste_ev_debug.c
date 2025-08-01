#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "real_time_ev.h"
#include "shoe_counter.h"
#include "jogo.h"

// Teste específico para debugar cálculos de EV
void test_ev_debug() {
    printf("🔍 TESTE DE DEBUG DOS CÁLCULOS DE EV\n");
    printf("=====================================\n\n");
    
    // Inicializar shoe counter com 8 decks
    ShoeCounter counter = {0};
    shoe_counter_init(&counter, 8);
    
    // Teste 1: Mão 16 vs Dealer 10 (situação conhecida)
    printf("📊 TESTE 1: Mão 16 vs Dealer 10 (TC = 0)\n");
    printf("-------------------------------------------\n");
    
    // Criar mão 16 (8+8)
    uint64_t hand_16 = 0;
    hand_16 |= (2ULL << (7 * 3)); // 2 cartas de rank 7 (valor 8)
    
    double true_count = 0.0;
    int dealer_upcard = 10;
    
    // Calcular EV Stand
    double ev_stand = calculate_ev_stand_realtime(16, dealer_upcard, true_count, &counter);
    printf("EV Stand (16 vs 10): %.6f\n", ev_stand);
    
    // Calcular EV Hit
    double ev_hit = calculate_ev_hit_realtime(hand_16, dealer_upcard, true_count, &counter, 0);
    printf("EV Hit (16 vs 10): %.6f\n", ev_hit);
    
    // Calcular EV Double
    double ev_double = calculate_ev_double_realtime(hand_16, dealer_upcard, true_count, &counter);
    printf("EV Double (16 vs 10): %.6f\n", ev_double);
    
    printf("Melhor ação: %s\n", (ev_stand > ev_hit && ev_stand > ev_double) ? "STAND" : 
                                (ev_hit > ev_double) ? "HIT" : "DOUBLE");
    printf("\n");
    
    // Teste 2: Mão 12 vs Dealer 4 (situação conhecida)
    printf("📊 TESTE 2: Mão 12 vs Dealer 4 (TC = 0)\n");
    printf("------------------------------------------\n");
    
    // Criar mão 12 (6+6)
    uint64_t hand_12 = 0;
    hand_12 |= (2ULL << (5 * 3)); // 2 cartas de rank 5 (valor 6)
    
    dealer_upcard = 4;
    
    ev_stand = calculate_ev_stand_realtime(12, dealer_upcard, true_count, &counter);
    printf("EV Stand (12 vs 4): %.6f\n", ev_stand);
    
    ev_hit = calculate_ev_hit_realtime(hand_12, dealer_upcard, true_count, &counter, 0);
    printf("EV Hit (12 vs 4): %.6f\n", ev_hit);
    
    ev_double = calculate_ev_double_realtime(hand_12, dealer_upcard, true_count, &counter);
    printf("EV Double (12 vs 4): %.6f\n", ev_double);
    
    printf("Melhor ação: %s\n", (ev_stand > ev_hit && ev_stand > ev_double) ? "STAND" : 
                                (ev_hit > ev_double) ? "HIT" : "DOUBLE");
    printf("\n");
    
    // Teste 3: Verificar probabilidades do dealer
    printf("📊 TESTE 3: Probabilidades do Dealer (10, TC=0)\n");
    printf("------------------------------------------------\n");
    
    DealerProbabilities dealer_probs = get_dealer_probabilities(10, 0.0, &counter);
    printf("P(17): %.6f\n", dealer_probs.prob_17);
    printf("P(18): %.6f\n", dealer_probs.prob_18);
    printf("P(19): %.6f\n", dealer_probs.prob_19);
    printf("P(20): %.6f\n", dealer_probs.prob_20);
    printf("P(21): %.6f\n", dealer_probs.prob_21);
    printf("P(BJ): %.6f\n", dealer_probs.prob_blackjack);
    printf("P(BUST): %.6f\n", dealer_probs.prob_bust);
    
    double total = dealer_probs.prob_17 + dealer_probs.prob_18 + dealer_probs.prob_19 + 
                  dealer_probs.prob_20 + dealer_probs.prob_21 + dealer_probs.prob_blackjack + 
                  dealer_probs.prob_bust;
    printf("Soma total: %.6f (deve ser ~1.0)\n", total);
    printf("\n");
    
    // Teste 4: Verificar se os dados estão sendo carregados corretamente
    printf("📊 TESTE 4: Validação dos Dados Carregados\n");
    printf("-------------------------------------------\n");
    
    printf("Dealer 10 -> 17 (TC=0): %.6f\n", get_dealer_freq(10, 17, 0.0));
    printf("Dealer 10 -> 18 (TC=0): %.6f\n", get_dealer_freq(10, 18, 0.0));
    printf("Dealer 10 -> 19 (TC=0): %.6f\n", get_dealer_freq(10, 19, 0.0));
    printf("Dealer 10 -> 20 (TC=0): %.6f\n", get_dealer_freq(10, 20, 0.0));
    printf("Dealer 10 -> 21 (TC=0): %.6f\n", get_dealer_freq(10, 21, 0.0));
    printf("Dealer 10 -> BJ (TC=0): %.6f\n", get_dealer_freq(10, 22, 0.0));
    printf("Dealer 10 -> BUST (TC=0): %.6f\n", get_dealer_freq(10, 23, 0.0));
    printf("\n");
    
    // Teste 5: Verificar se as decisões estão corretas
    printf("📊 TESTE 5: Comparação com Estratégia Básica\n");
    printf("---------------------------------------------\n");
    
    // Mão 16 vs Dealer 10: Estratégia básica = HIT
    printf("Mão 16 vs Dealer 10:\n");
    printf("  Estratégia Básica: HIT\n");
    printf("  EV Stand: %.6f\n", ev_stand);
    printf("  EV Hit: %.6f\n", ev_hit);
    printf("  EV Double: %.6f\n", ev_double);
    printf("  Decisão EV Tempo Real: %s\n", (ev_stand > ev_hit && ev_stand > ev_double) ? "STAND" : 
                                            (ev_hit > ev_double) ? "HIT" : "DOUBLE");
    printf("  ✅ Decisão correta: %s\n", ((ev_hit > ev_double) ? "HIT" : "DOUBLE") == "HIT" ? "SIM" : "NÃO");
    printf("\n");
    
    // Mão 12 vs Dealer 4: Estratégia básica = HIT
    printf("Mão 12 vs Dealer 4:\n");
    printf("  Estratégia Básica: HIT\n");
    printf("  EV Stand: %.6f\n", ev_stand);
    printf("  EV Hit: %.6f\n", ev_hit);
    printf("  EV Double: %.6f\n", ev_double);
    printf("  Decisão EV Tempo Real: %s\n", (ev_stand > ev_hit && ev_stand > ev_double) ? "STAND" : 
                                            (ev_hit > ev_double) ? "HIT" : "DOUBLE");
    printf("  ✅ Decisão correta: %s\n", ((ev_hit > ev_double) ? "HIT" : "DOUBLE") == "HIT" ? "SIM" : "NÃO");
    printf("\n");
    
    // Teste 6: Verificar se há problema na magnitude dos EVs
    printf("📊 TESTE 6: Análise da Magnitude dos EVs\n");
    printf("----------------------------------------\n");
    
    printf("EVs muito negativos podem indicar:\n");
    printf("1. Problema na interpretação dos dados\n");
    printf("2. Problema na fórmula matemática\n");
    printf("3. Dados incorretos\n");
    printf("4. Sistema funcionando corretamente (EVs negativos são normais)\n");
    printf("\n");
    
    // Teste 7: Verificar EVs para situações conhecidas
    printf("📊 TESTE 7: EVs para Situações Conhecidas\n");
    printf("------------------------------------------\n");
    
    // Mão 20 vs Dealer 10 (deveria ser positiva)
    double ev_20_vs_10 = calculate_ev_stand_realtime(20, 10, 0.0, &counter);
    printf("EV Stand (20 vs 10): %.6f\n", ev_20_vs_10);
    
    // Mão 19 vs Dealer 10 (deveria ser moderadamente negativa)
    double ev_19_vs_10 = calculate_ev_stand_realtime(19, 10, 0.0, &counter);
    printf("EV Stand (19 vs 10): %.6f\n", ev_19_vs_10);
    
    // Mão 18 vs Dealer 10 (deveria ser negativa)
    double ev_18_vs_10 = calculate_ev_stand_realtime(18, 10, 0.0, &counter);
    printf("EV Stand (18 vs 10): %.6f\n", ev_18_vs_10);
    
    // Mão 17 vs Dealer 10 (deveria ser muito negativa)
    double ev_17_vs_10 = calculate_ev_stand_realtime(17, 10, 0.0, &counter);
    printf("EV Stand (17 vs 10): %.6f\n", ev_17_vs_10);
    
    printf("\n");
}

int main() {
    // Carregar dados
    if (!load_dealer_freq_table("Resultados")) {
        printf("ERRO: Falha ao carregar frequências do dealer!\n");
        return 1;
    }
    
    if (!load_split_ev_table("Resultados")) {
        printf("ERRO: Falha ao carregar EV de splits!\n");
        return 1;
    }
    
    // Executar testes de debug
    test_ev_debug();
    
    return 0;
} 