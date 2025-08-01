#include "split_ev_lookup.h"
#include "dealer_freq_lookup.h"
#include <stdio.h>

int main() {
    printf("=== EXEMPLO DE USO DAS TABELAS DE LOOKUP ===\n\n");
    
    // Carregar tabelas
    if (!load_split_ev_table("./Resultados")) {
        printf("Erro: Não foi possível carregar tabela de EV de splits\n");
        return 1;
    }
    
    if (!load_dealer_freq_table("./Resultados")) {
        printf("Erro: Não foi possível carregar tabela de frequências do dealer\n");
        return 1;
    }
    
    printf("✅ Tabelas carregadas com sucesso!\n\n");
    
    // Exemplo 1: Análise de splits
    printf("📊 ANÁLISE DE SPLITS\n");
    printf("===================\n");
    
    printf("Split AA vs A em diferentes True Counts:\n");
    for (double tc = -3.0; tc <= 3.0; tc += 1.0) {
        double ev = get_split_ev(11, 11, tc); // AA vs A
        printf("  TC %+.1f: EV = %+.4f\n", tc, ev);
    }
    
    printf("\nComparando diferentes splits com TC = 0.0:\n");
    const char* pairs[] = {"AA", "88", "99", "22"};
    const int pair_ranks[] = {11, 8, 9, 2};
    
    for (int i = 0; i < 4; i++) {
        double ev_vs_6 = get_split_ev(pair_ranks[i], 6, 0.0);
        double ev_vs_10 = get_split_ev(pair_ranks[i], 10, 0.0);
        printf("  %s vs 6: %+.4f | %s vs 10: %+.4f\n", 
               pairs[i], ev_vs_6, pairs[i], ev_vs_10);
    }
    
    // Exemplo 2: Análise de frequências do dealer
    printf("\n🎯 ANÁLISE DE FREQUÊNCIAS DO DEALER\n");
    printf("===================================\n");
    
    printf("Frequência de Blackjack do dealer (TC = 0.0):\n");
    for (int upcard = 10; upcard <= 11; upcard++) {
        double freq_bj = get_dealer_freq_bj(upcard, 0.0);
        const char* upcard_name = (upcard == 10) ? "10" : "A";
        printf("  Upcard %s: %.2f%%\n", upcard_name, freq_bj);
    }
    
    printf("\nFrequência de BUST do dealer por upcard (TC = 0.0):\n");
    for (int upcard = 2; upcard <= 11; upcard++) {
        double freq_bust = get_dealer_freq_bust(upcard, 0.0);
        const char* upcard_name = (upcard == 11) ? "A" : 
                                 (upcard == 10) ? "10" : "";
        if (upcard <= 9) {
            printf("  Upcard %d: %.2f%%\n", upcard, freq_bust);
        } else {
            printf("  Upcard %s: %.2f%%\n", upcard_name, freq_bust);
        }
    }
    
    printf("\nImpacto do True Count na frequência de BJ (upcard A):\n");
    for (double tc = -2.0; tc <= 2.0; tc += 1.0) {
        double freq_bj = get_dealer_freq_bj(11, tc);
        printf("  TC %+.1f: %.2f%%\n", tc, freq_bj);
    }
    
    // Exemplo 3: Análise completa
    printf("\n🔍 ANÁLISE COMPLETA: UPCARD 6 vs TRUE COUNT\n");
    printf("==========================================\n");
    
    printf("TC   | 17    | 18    | 19    | 20    | 21    | BUST  | Total\n");
    printf("-----|-------|-------|-------|-------|-------|-------|-------\n");
    
    for (double tc = -2.0; tc <= 2.0; tc += 1.0) {
        DealerFreqAll freqs = get_dealer_freq_all(6, tc);
        printf("%+.1f | %5.1f | %5.1f | %5.1f | %5.1f | %5.1f | %5.1f | %5.1f\n",
               tc, freqs.freq_17, freqs.freq_18, freqs.freq_19, 
               freqs.freq_20, freqs.freq_21, freqs.freq_bust, freqs.total_freq);
    }
    
    // Exemplo 4: Decisão de split baseada em EV
    printf("\n🎰 DECISÃO DE SPLIT BASEADA EM EV\n");
    printf("=================================\n");
    
    printf("Par 88 vs dealer 8 - Devemos splitar?\n");
    double ev_split = get_split_ev(8, 8, 0.0);
    printf("  EV do split: %+.4f\n", ev_split);
    printf("  Decisão: %s\n", (ev_split > 0.0) ? "SPLIT!" : "NÃO SPLIT");
    
    printf("\nPar AA vs dealer A - Variação com TC:\n");
    for (double tc = -2.0; tc <= 2.0; tc += 0.5) {
        double ev = get_split_ev(11, 11, tc);
        const char* decision = (ev > 0.0) ? "SPLIT" : "NÃO SPLIT";
        printf("  TC %+.1f: EV = %+.4f -> %s\n", tc, ev, decision);
    }
    
    // Limpeza
    unload_split_ev_table();
    unload_dealer_freq_table();
    
    printf("\n✅ Exemplo concluído!\n");
    return 0;
} 