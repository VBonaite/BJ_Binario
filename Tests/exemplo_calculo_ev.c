#include "../ev_calculator.h"
#include "../shoe_counter.h"
#include <stdio.h>

int main() {
    printf("🃏 EXEMPLO PRÁTICO: CALCULANDO EV DE HIT vs STAND\n");
    printf("================================================\n\n");
    
    printf("📚 CONTEXTO TEÓRICO:\n");
    printf("====================\n");
    printf("Expected Value (EV) é o resultado médio esperado de uma ação.\n");
    printf("Para calcular o EV de uma ação em blackjack:\n\n");
    
    printf("🎯 EV de STAND:\n");
    printf("EV = Σ P(resultado_dealer) × payoff(resultado)\n");
    printf("   = P(dealer_bust) × (+1) + P(dealer_17-21) × (-1)\n\n");
    
    printf("🎯 EV de HIT:\n");
    printf("EV = Σ P(carta) × EV_ótimo_após_receber_carta\n");
    printf("   = P(carta_não_bust) × EV_situação_melhorada + P(carta_bust) × (-1)\n\n");
    
    // Exemplos com diferentes upcards do dealer
    int dealer_upcards[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    int num_upcards = sizeof(dealer_upcards) / sizeof(dealer_upcards[0]);
    
    printf("🎰 DEMONSTRAÇÃO PRÁTICA: MÃO DE 12 vs DIFERENTES UPCARDS\n");
    printf("========================================================\n\n");
    
    for (int i = 0; i < num_upcards; i++) {
        int dealer_upcard = dealer_upcards[i];
        
        printf("┌─────────────────────────────────────────────────────────┐\n");
        printf("│ CENÁRIO %d: Jogador 12 vs Dealer %s%-23s│\n", 
               i+1, 
               (dealer_upcard == 11) ? "A" : "", 
               (dealer_upcard == 11) ? "" : "");
        if (dealer_upcard != 11) {
            printf("│ CENÁRIO %d: Jogador 12 vs Dealer %-2d                    │\n", i+1, dealer_upcard);
        }
        printf("└─────────────────────────────────────────────────────────┘\n");
        
        // Demonstrar o cálculo detalhado apenas para os primeiros casos
        if (i < 3) {
            demonstrate_ev_calculation_for_12(dealer_upcard);
        } else {
            // Para os demais, mostrar apenas o resumo
            ShoeCounter counter;
            shoe_counter_init(&counter, 8);
            
            EVResult result = calculate_all_evs(12, dealer_upcard, &counter);
            
            printf("📋 RESUMO:\n");
            printf("EV(Stand): %+.6f | EV(Hit): %+.6f | Melhor: %c\n", 
                   result.ev_stand, result.ev_hit, result.best_action);
        }
        
        printf("\n" "═══════════════════════════════════════════════════════════\n\n");
    }
    
    printf("📊 RESUMO GERAL: ESTRATÉGIA PARA MÃO DE 12\n");
    printf("==========================================\n\n");
    
    // Tabela resumo
    printf("┌──────────────┬──────────────┬──────────────┬──────────────┐\n");
    printf("│ Dealer Upcard│   EV(Stand)  │    EV(Hit)   │ Melhor Ação  │\n");
    printf("├──────────────┼──────────────┼──────────────┼──────────────┤\n");
    
    for (int i = 0; i < num_upcards; i++) {
        int dealer_upcard = dealer_upcards[i];
        
        ShoeCounter counter;
        shoe_counter_init(&counter, 8);
        
        // Cálculo simplificado (sem output detalhado)
        double ev_stand = calculate_ev_stand(12, dealer_upcard, &counter);
        double ev_hit = calculate_ev_hit(12, dealer_upcard, &counter, 1);
        
        char best = (ev_hit > ev_stand) ? 'H' : 'S';
        const char* upcard_str = (dealer_upcard == 11) ? "A" : "";
        
        if (dealer_upcard == 11) {
            printf("│      %s       │   %+.6f   │   %+.6f   │      %c       │\n",
                   upcard_str, ev_stand, ev_hit, best);
        } else {
            printf("│      %-2d      │   %+.6f   │   %+.6f   │      %c       │\n",
                   dealer_upcard, ev_stand, ev_hit, best);
        }
    }
    
    printf("└──────────────┴──────────────┴──────────────┴──────────────┘\n\n");
    
    printf("🎓 CONCLUSÕES EDUCATIVAS:\n");
    printf("=========================\n");
    printf("1. Contra upcards baixos (2-6): Geralmente STAND\n");
    printf("   → Dealer tem alta chance de bust\n\n");
    printf("2. Contra upcards altos (7-A): Geralmente HIT\n");
    printf("   → Dealer provavelmente fará uma boa mão\n\n");
    printf("3. A diferença de EV é muitas vezes pequena\n");
    printf("   → Decisões marginais são comuns no blackjack\n\n");
    printf("4. Composition-dependent: Usar shoe counter permite\n");
    printf("   ajustes baseados nas cartas já saídas\n\n");
    
    printf("💡 APLICAÇÃO PRÁTICA:\n");
    printf("=====================\n");
    printf("Este mesmo método pode ser usado para:\n");
    printf("• Qualquer total de mão (não só 12)\n");
    printf("• Mãos soft (com Ás)\n");
    printf("• Decisões de double down\n");
    printf("• Análise de splits\n");
    printf("• Estratégias composition-dependent avançadas\n\n");
    
    return 0;
} 