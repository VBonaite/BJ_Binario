#include "realtime_strategy_integration.h"
#include "real_time_ev.h"
#include "tabela_estrategia.h"
#include "jogo.h"
#include <stdio.h>
#include <string.h>

// Variáveis globais
bool realtime_ev_enabled = true;  // REATIVADO - Investigação do problema
RealtimeStrategyStats realtime_stats = {0};

// ====================== INICIALIZAÇÃO DO SISTEMA ======================

void init_realtime_strategy_system(bool load_lookup_tables) {
    printf("🚀 Inicializando sistema de EV em tempo real...\n");
    
    // Inicializar cache de EV
    init_ev_cache();
    
    // Carregar tabelas de lookup somente se solicitado
    if (load_lookup_tables) {
        if (!split_ev_table_loaded) {
            printf("   Carregando tabelas de split EV...\n");
            load_split_ev_table("./Resultados");
        }
        
        if (!dealer_freq_table_loaded) {
            printf("   Carregando tabelas de frequência do dealer...\n");
            load_dealer_freq_table("./Resultados");
        }
    } else {
        printf("   Tabelas de lookup: PULADAS (EV em tempo real desabilitado)\n");
    }
    
    // Resetar estatísticas
    reset_realtime_strategy_stats();
    
    // Habilitar sistema
    // realtime_ev_enabled = true;  // COMENTADO: manter valor original da variável
    
    printf("✅ Sistema de EV em tempo real inicializado!\n");
    printf("   - Cache de EV: Ativo\n");
    printf("   - Tabelas de split: %s\n", split_ev_table_loaded ? "Carregadas" : "Fallback");
    printf("   - Tabelas dealer freq: %s\n", dealer_freq_table_loaded ? "Carregadas" : "Fallback");
}

void cleanup_realtime_strategy_system(void) {
    // Imprimir estatísticas antes de finalizar
    print_realtime_strategy_stats();
    clear_ev_cache();
    realtime_ev_enabled = false;
    printf("🧹 Sistema de EV em tempo real finalizado.\n");
}

// ====================== FUNÇÃO PRINCIPAL DE DECISÃO ======================

AcaoEstrategia determinar_acao_realtime(
    const Mao *mao, 
    uint64_t mao_bits, 
    int dealer_up_rank,
    double true_count,
    const ShoeCounter* shoe_counter,
    bool is_initial_hand
) {
    realtime_stats.total_decisions++;
    
    // Se sistema não está habilitado, usar estratégia básica
    if (!realtime_ev_enabled || !shoe_counter || !shoe_counter->initialized) {
        realtime_stats.basic_strategy_decisions++;
        return estrategia_basica_super_rapida(mao_bits, dealer_up_rank);
    }
    
    // Verificar blackjack natural
    if (is_initial_hand && mao->blackjack) {
        return ACAO_STAND;
    }
    
    // Calcular ação baseada em EV em tempo real
    AcaoEstrategia realtime_action = determine_optimal_action_realtime(
        mao_bits, dealer_up_rank, true_count, shoe_counter, is_initial_hand
    );
    
    // Aplicar regras especiais
    realtime_action = handle_special_rules(
        realtime_action, mao, is_initial_hand, 
        is_initial_hand, // double_allowed
        is_initial_hand  // split_allowed
    );
    
    realtime_stats.realtime_decisions++;
    
    // TEMPORARIAMENTE COMENTADO: Para comparação, calcular também estratégia básica
    /*
    AcaoEstrategia basic_action = estrategia_basica_super_rapida(mao_bits, dealer_up_rank);
    
    // Log da decisão se for diferente
    if (realtime_action != basic_action) {
        realtime_stats.differences_found++;
        
        // Calcular melhoria de EV (simplificado)
        RealTimeEVResult ev_result = calculate_real_time_ev(
            mao_bits, dealer_up_rank, true_count, shoe_counter,
            is_initial_hand, is_initial_hand, is_initial_hand
        );
        
        double ev_improvement = 0.01; // Placeholder para melhoria estimada
        realtime_stats.avg_ev_improvement += ev_improvement;
        
        log_strategy_decision(mao_bits, dealer_up_rank, true_count, 
                            realtime_action, basic_action, ev_improvement);
    }
    */
    
    return realtime_action;
}

// ====================== REGRAS ESPECIAIS ======================

AcaoEstrategia handle_special_rules(
    AcaoEstrategia base_action,
    const Mao *mao,
    bool is_initial_hand,
    bool double_allowed,
    bool split_allowed
) {
    // Double só é permitido em mãos iniciais
    if (base_action == ACAO_DOUBLE && !is_initial_hand) {
        return ACAO_HIT; // Fallback para hit
    }
    
    if ((base_action == ACAO_DOUBLE_OR_HIT) && !double_allowed) {
        return ACAO_HIT;
    }
    
    if ((base_action == ACAO_DOUBLE_OR_STAND) && !double_allowed) {
        return ACAO_STAND;
    }
    
    // Split só é permitido em mãos iniciais com pares
    if (base_action == ACAO_SPLIT && (!is_initial_hand || !split_allowed)) {
        return ACAO_STAND; // Fallback para stand
    }
    
    if ((base_action == ACAO_SPLIT_OR_HIT) && !split_allowed) {
        return ACAO_HIT;
    }
    
    if ((base_action == ACAO_SPLIT_OR_STAND) && !split_allowed) {
        return ACAO_STAND;
    }
    
    return base_action;
}

bool is_aces_split(const Mao *mao) {
    return mao->from_split && mao->split_rank_idx == 12; // Ás = índice 12
}

bool is_double_action(AcaoEstrategia action) {
    return (action == ACAO_DOUBLE || 
            action == ACAO_DOUBLE_OR_HIT || 
            action == ACAO_DOUBLE_OR_STAND);
}

// ====================== ATUALIZAÇÃO DO SHOE COUNTER ======================

void update_shoe_counter_after_card(ShoeCounter* counter, Carta carta) {
    if (counter && counter->initialized) {
        shoe_counter_remove_card(counter, carta);
    }
}

// ====================== ESTATÍSTICAS E LOGGING ======================

void log_strategy_decision(
    uint64_t hand_bits,
    int dealer_upcard,
    double true_count,
    AcaoEstrategia realtime_action,
    AcaoEstrategia basic_action,
    double ev_improvement
) {
    // Log apenas se habilitado para debug
    static int log_count = 0;
    if (log_count < 10) { // Limitar logs
        printf("📊 Decisão diferente #%d: Mão=%d vs %d, TC=%.1f\n", 
               ++log_count, calcular_valor_mao(hand_bits), dealer_upcard, true_count);
        printf("   Básica: %s, Real-time: %s, Melhoria EV: +%.4f\n",
               acao_to_str(basic_action), acao_to_str(realtime_action), ev_improvement);
    }
}

void print_realtime_strategy_stats(void) {
    printf("\n📈 ESTATÍSTICAS DO SISTEMA DE EV EM TEMPO REAL\n");
    printf("==============================================\n");
    printf("Total de decisões: %d\n", realtime_stats.total_decisions);
    printf("Decisões em tempo real: %d\n", realtime_stats.realtime_decisions);
    printf("Decisões estratégia básica: %d\n", realtime_stats.basic_strategy_decisions);
    printf("Diferenças encontradas: %d\n", realtime_stats.differences_found);
    
    // Estatísticas de fallback
    printf("\n🛡️  ESTATÍSTICAS DE FALLBACK\n");
    printf("============================\n");
    printf("Total de fallbacks: %d\n", realtime_stats.total_fallbacks);
    
    if (realtime_stats.total_decisions > 0) {
        double fallback_rate = 100.0 * realtime_stats.total_fallbacks / realtime_stats.total_decisions;
        printf("Taxa de fallback: %.2f%%\n", fallback_rate);
    }
    
    printf("\nDetalhamento de fallbacks:\n");
    printf("  Entradas inválidas: %d\n", realtime_stats.fallback_invalid_input);
    printf("  Casos triviais (BJ/bust): %d\n", realtime_stats.fallback_trivial_cases);
    printf("  Contador inválido: %d\n", realtime_stats.fallback_invalid_counter);
    printf("  True count inválido: %d\n", realtime_stats.fallback_invalid_true_count);
    printf("  Cálculo falhou: %d\n", realtime_stats.fallback_calculation_failed);
    printf("  Valores EV insanos: %d\n", realtime_stats.fallback_invalid_ev_values);
    printf("  Ação inválida: %d\n", realtime_stats.fallback_invalid_action);
    printf("  Restrições de contexto: %d\n", realtime_stats.fallback_context_restrictions);
    
    // Estatísticas originais
    printf("\n📊 ESTATÍSTICAS DE PERFORMANCE\n");
    printf("==============================\n");
    if (realtime_stats.realtime_decisions > 0) {
        double diff_percentage = 100.0 * realtime_stats.differences_found / realtime_stats.realtime_decisions;
        printf("Percentual de diferenças: %.2f%%\n", diff_percentage);
    }
    
    if (realtime_stats.differences_found > 0) {
        double avg_improvement = realtime_stats.avg_ev_improvement / realtime_stats.differences_found;
        printf("Melhoria média de EV: +%.6f\n", avg_improvement);
    }
    
    // Taxa de sucesso do sistema
    int successful_realtime = realtime_stats.realtime_decisions - 
                             (realtime_stats.total_fallbacks - realtime_stats.fallback_trivial_cases);
    if (realtime_stats.total_decisions > 0) {
        double success_rate = 100.0 * successful_realtime / realtime_stats.total_decisions;
        printf("Taxa de sucesso EV tempo real: %.2f%%\n", success_rate);
    }
}

void print_fallback_summary(void) {
    if (realtime_stats.total_fallbacks == 0) {
        printf("🛡️ Sistema robusto: Nenhum fallback necessário!\n");
        return;
    }
    
    printf("🛡️ Resumo de fallbacks: %d total (%.2f%% das decisões)\n", 
           realtime_stats.total_fallbacks, 
           realtime_stats.total_decisions > 0 ? 
           100.0 * realtime_stats.total_fallbacks / realtime_stats.total_decisions : 0.0);
    
    // Mostrar apenas os tipos que ocorreram
    if (realtime_stats.fallback_invalid_input > 0)
        printf("  - Entradas inválidas: %d\n", realtime_stats.fallback_invalid_input);
    if (realtime_stats.fallback_trivial_cases > 0)
        printf("  - Casos triviais: %d\n", realtime_stats.fallback_trivial_cases);
    if (realtime_stats.fallback_invalid_counter > 0)
        printf("  - Contador inválido: %d\n", realtime_stats.fallback_invalid_counter);
    if (realtime_stats.fallback_invalid_true_count > 0)
        printf("  - True count inválido: %d\n", realtime_stats.fallback_invalid_true_count);
    if (realtime_stats.fallback_calculation_failed > 0)
        printf("  - Cálculo falhou: %d\n", realtime_stats.fallback_calculation_failed);
    if (realtime_stats.fallback_invalid_ev_values > 0)
        printf("  - Valores EV insanos: %d\n", realtime_stats.fallback_invalid_ev_values);
    if (realtime_stats.fallback_invalid_action > 0)
        printf("  - Ação inválida: %d\n", realtime_stats.fallback_invalid_action);
    if (realtime_stats.fallback_context_restrictions > 0)
        printf("  - Restrições contexto: %d\n", realtime_stats.fallback_context_restrictions);
}

void reset_realtime_strategy_stats(void) {
    memset(&realtime_stats, 0, sizeof(realtime_stats));
} 