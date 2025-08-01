#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "dealer_freq_lookup.h"
#include "split_ev_lookup.h"

// Teste de validação dos dados carregados
void test_data_validation() {
    printf("🧪 TESTE DE VALIDAÇÃO DOS DADOS CARREGADOS\n");
    printf("============================================\n\n");
    
    // Teste 1: Verificar carregamento das frequências do dealer
    printf("📊 TESTE 1: Frequências do Dealer\n");
    printf("----------------------------------\n");
    
    // Testar upcard 10 (dealer 10) vs resultado 17
    double freq_10_17 = get_dealer_freq(10, 17, 0.0);
    printf("Dealer 10 -> 17 (TC=0): %.6f\n", freq_10_17);
    
    // Testar upcard 10 (dealer 10) vs resultado 18
    double freq_10_18 = get_dealer_freq(10, 18, 0.0);
    printf("Dealer 10 -> 18 (TC=0): %.6f\n", freq_10_18);
    
    // Testar upcard 10 (dealer 10) vs resultado 19
    double freq_10_19 = get_dealer_freq(10, 19, 0.0);
    printf("Dealer 10 -> 19 (TC=0): %.6f\n", freq_10_19);
    
    // Testar upcard 10 (dealer 10) vs resultado 20
    double freq_10_20 = get_dealer_freq(10, 20, 0.0);
    printf("Dealer 10 -> 20 (TC=0): %.6f\n", freq_10_20);
    
    // Testar upcard 10 (dealer 10) vs resultado 21
    double freq_10_21 = get_dealer_freq(10, 21, 0.0);
    printf("Dealer 10 -> 21 (TC=0): %.6f\n", freq_10_21);
    
    // Testar upcard 10 (dealer 10) vs resultado BJ
    double freq_10_bj = get_dealer_freq(10, 22, 0.0); // 22 = BJ
    printf("Dealer 10 -> BJ (TC=0): %.6f\n", freq_10_bj);
    
    // Testar upcard 10 (dealer 10) vs resultado BUST
    double freq_10_bust = get_dealer_freq(10, 23, 0.0); // 23 = BUST
    printf("Dealer 10 -> BUST (TC=0): %.6f\n", freq_10_bust);
    
    // Verificar se soma aproximadamente 1.0
    double total_freq_10 = freq_10_17 + freq_10_18 + freq_10_19 + freq_10_20 + freq_10_21 + freq_10_bj + freq_10_bust;
    printf("Soma das probabilidades (dealer 10): %.6f (deve ser ~1.0)\n\n", total_freq_10);
    
    // Teste 2: Verificar carregamento dos splits
    printf("📊 TESTE 2: EV de Splits\n");
    printf("-------------------------\n");
    
    // Testar split AA vs dealer 2
    double ev_aa_vs_2 = get_split_ev(11, 2, 0.0); // 11 = A
    printf("Split AA vs Dealer 2 (TC=0): %.6f\n", ev_aa_vs_2);
    
    // Testar split 88 vs dealer 10
    double ev_88_vs_10 = get_split_ev(8, 10, 0.0);
    printf("Split 88 vs Dealer 10 (TC=0): %.6f\n", ev_88_vs_10);
    
    // Testar split 1010 vs dealer 6
    double ev_1010_vs_6 = get_split_ev(10, 6, 0.0);
    printf("Split 1010 vs Dealer 6 (TC=0): %.6f\n", ev_1010_vs_6);
    
    printf("\n");
    
    // Teste 3: Verificar diferentes true counts
    printf("📊 TESTE 3: Diferentes True Counts\n");
    printf("-----------------------------------\n");
    
    printf("Dealer 10 -> 17 (TC=-3): %.6f\n", get_dealer_freq(10, 17, -3.0));
    printf("Dealer 10 -> 17 (TC=0): %.6f\n", get_dealer_freq(10, 17, 0.0));
    printf("Dealer 10 -> 17 (TC=+3): %.6f\n", get_dealer_freq(10, 17, 3.0));
    
    printf("\n");
    
    // Teste 4: Verificar upcards diferentes
    printf("📊 TESTE 4: Diferentes Upcards\n");
    printf("--------------------------------\n");
    
    printf("Dealer 2 -> 17 (TC=0): %.6f\n", get_dealer_freq(2, 17, 0.0));
    printf("Dealer 6 -> 17 (TC=0): %.6f\n", get_dealer_freq(6, 17, 0.0));
    printf("Dealer 10 -> 17 (TC=0): %.6f\n", get_dealer_freq(10, 17, 0.0));
    printf("Dealer A -> 17 (TC=0): %.6f\n", get_dealer_freq(11, 17, 0.0));
    
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
    
    // Executar testes
    test_data_validation();
    
    return 0;
} 