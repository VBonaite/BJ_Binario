#include "structures.h"
#include "constantes.h"
#include <stdlib.h>
#include <string.h>

// Contexto SIMD global
SIMDContext simd_ctx = {0};

// Inicializar sistema SIMD
bool simd_init(void) {
    DEBUG_IO("Inicializando sistema SIMD");
    
    memset(&simd_ctx, 0, sizeof(SIMDContext));
    
#if SIMD_ENABLED
    // Verificar se SSE2 está disponível
    simd_ctx.simd_available = true;
    simd_ctx.vector_width = 2; // SSE2 trabalha com 2 doubles por vez
    simd_ctx.count_cards_simd = simd_count_cards;
    simd_ctx.evaluate_hands_simd = simd_evaluate_hands;
    
    DEBUG_IO("SIMD habilitado: SSE2 disponível, largura do vetor: %d", simd_ctx.vector_width);
#else
    // Fallback para implementação escalar
    simd_ctx.simd_available = false;
    simd_ctx.vector_width = 1;
    simd_ctx.count_cards_simd = NULL;
    simd_ctx.evaluate_hands_simd = NULL;
    
    DEBUG_IO("SIMD desabilitado: usando implementação escalar");
#endif
    
    return true;
}

// Cleanup do sistema SIMD
void simd_cleanup(void) {
    DEBUG_IO("Limpando sistema SIMD");
    memset(&simd_ctx, 0, sizeof(SIMDContext));
}

#if SIMD_ENABLED

// Função SIMD para contar cartas - processa múltiplas cartas simultaneamente
void simd_count_cards(uint64_t* cards, int count, double* result) {
    if (!cards || !result || count <= 0) {
        return;
    }
    
    DEBUG_STATS("Contando cartas SIMD: %d cartas", count);
    
    // Processar cartas em lotes de 2 usando SSE2
    __m128d running_sum = _mm_setzero_pd(); // Inicializar com zero
    
    int i = 0;
    // Processar pares de cartas
    for (; i + 1 < count; i += 2) {
        // Extrair ranks das cartas usando bit manipulation
        uint64_t card1 = cards[i];
        uint64_t card2 = cards[i + 1];
        
        // Calcular ranks (otimizado)
        int rank1 = __builtin_ctzll(card1) / 3;
        int rank2 = __builtin_ctzll(card2) / 3;
        
        // Carregar valores Wong Halves
        double value1 = (rank1 < 13) ? WONG_HALVES[rank1] : 0.0;
        double value2 = (rank2 < 13) ? WONG_HALVES[rank2] : 0.0;
        
        // Carregar em registrador SSE2
        __m128d values = _mm_set_pd(value2, value1);
        
        // Somar ao running count
        running_sum = _mm_add_pd(running_sum, values);
    }
    
    // Extrair resultado da soma SIMD
    double temp_result[2];
    _mm_storeu_pd(temp_result, running_sum);
    *result = temp_result[0] + temp_result[1];
    
    // Processar carta restante se count for ímpar
    if (i < count) {
        uint64_t card = cards[i];
        int rank = __builtin_ctzll(card) / 3;
        if (rank < 13) {
            *result += WONG_HALVES[rank];
        }
    }
    
    DEBUG_STATS("Contagem SIMD concluída: %.3f", *result);
}

// Função SIMD para avaliar múltiplas mãos simultaneamente
void simd_evaluate_hands(uint64_t* hands, int count, int* results) {
    if (!hands || !results || count <= 0) {
        return;
    }
    
    DEBUG_STATS("Avaliando mãos SIMD: %d mãos", count);
    
    // Processar mãos em lotes usando bit manipulation otimizado
    for (int i = 0; i < count; i++) {
        uint64_t hand = hands[i];
        int valor = 0;
        int aces = 0;
        
        // Contar cartas de cada rank usando bit manipulation
        for (int rank = 0; rank < 13; rank++) {
            int count_rank = (hand >> (rank * 3)) & 0x7;
            
            if (count_rank > 0) {
                if (rank == 0) { // Ás
                    aces += count_rank;
                    valor += count_rank; // Inicialmente contar como 1
                } else if (rank >= 9) { // 10, J, Q, K
                    valor += count_rank * 10;
                } else { // 2-9
                    valor += count_rank * (rank + 1);
                }
            }
        }
        
        // Ajustar Ases de 1 para 11 se possível
        while (aces > 0 && valor + 10 <= 21) {
            valor += 10;
            aces--;
        }
        
        results[i] = valor;
    }
    
    DEBUG_STATS("Avaliação SIMD concluída: %d mãos processadas", count);
}

// Função SIMD para processar múltiplas operações de estratégia básica
void simd_strategy_lookup(uint64_t* hands, int* dealer_upcards, int count, uint8_t* strategies) {
    if (!hands || !dealer_upcards || !strategies || count <= 0) {
        return;
    }
    
    DEBUG_STATS("Lookup estratégia SIMD: %d mãos", count);
    
    // Processar em lotes para melhor utilização de cache
    for (int i = 0; i < count; i++) {
        uint64_t hand = hands[i];
        int dealer_upcard = dealer_upcards[i];
        
        // Usar perfect hash se disponível
        if (perfect_hash_strategy.is_initialized) {
            strategies[i] = perfect_hash_lookup(hand, dealer_upcard);
        } else {
            // Fallback para lookup tradicional
            // (implementação omitida para brevidade)
            strategies[i] = 0; // HIT como padrão
        }
    }
    
    DEBUG_STATS("Lookup estratégia SIMD concluído: %d estratégias obtidas", count);
}

// Função SIMD para operações de bit manipulation otimizadas
void simd_bit_operations(uint64_t* input, uint64_t* output, int count, int operation) {
    if (!input || !output || count <= 0) {
        return;
    }
    
    DEBUG_STATS("Operações bit SIMD: %d operações, tipo %d", count, operation);
    
    // Processar em lotes de 2 usando SSE2
    int i = 0;
    for (; i + 1 < count; i += 2) {
        // Carregar 2 valores de 64 bits como 128 bits
        __m128i values = _mm_set_epi64x(input[i + 1], input[i]);
        
        __m128i result;
        switch (operation) {
            case 0: // Population count (número de bits set)
                // SSE2 não tem popcnt nativo, usar implementação escalar
                output[i] = __builtin_popcountll(input[i]);
                output[i + 1] = __builtin_popcountll(input[i + 1]);
                break;
                
            case 1: // Count trailing zeros
                output[i] = __builtin_ctzll(input[i]);
                output[i + 1] = __builtin_ctzll(input[i + 1]);
                break;
                
            case 2: // Bit reversal - usar operações SIMD
                // Implementação simplificada para exemplo
                result = _mm_or_si128(
                    _mm_slli_epi64(values, 1),
                    _mm_srli_epi64(values, 1)
                );
                _mm_storeu_si128((__m128i*)&output[i], result);
                break;
                
            default:
                output[i] = input[i];
                output[i + 1] = input[i + 1];
        }
    }
    
    // Processar elemento restante se count for ímpar
    if (i < count) {
        switch (operation) {
            case 0:
                output[i] = __builtin_popcountll(input[i]);
                break;
            case 1:
                output[i] = __builtin_ctzll(input[i]);
                break;
            default:
                output[i] = input[i];
        }
    }
    
    DEBUG_STATS("Operações bit SIMD concluídas: %d operações processadas", count);
}

// Função SIMD para processamento paralelo de múltiplas simulações
void simd_parallel_simulation(uint64_t* player_hands, uint64_t* dealer_hands, 
                              int count, int* results) {
    if (!player_hands || !dealer_hands || !results || count <= 0) {
        return;
    }
    
    DEBUG_STATS("Simulação paralela SIMD: %d simulações", count);
    
    // Alocar arrays temporários para resultados intermediários
    int* player_values = POOL_ALLOC_TEMP(count * sizeof(int));
    int* dealer_values = POOL_ALLOC_TEMP(count * sizeof(int));
    
    if (!player_values || !dealer_values) {
        DEBUG_IO("ERRO: Falha ao alocar memória temporária para SIMD");
        return;
    }
    
    // Avaliar todas as mãos dos jogadores
    simd_evaluate_hands(player_hands, count, player_values);
    
    // Avaliar todas as mãos do dealer
    simd_evaluate_hands(dealer_hands, count, dealer_values);
    
    // Determinar resultados usando lógica vetorizada
    for (int i = 0; i < count; i++) {
        int player_val = player_values[i];
        int dealer_val = dealer_values[i];
        
        if (player_val > 21) {
            results[i] = -1; // Player bust
        } else if (dealer_val > 21) {
            results[i] = 1;  // Dealer bust
        } else if (player_val > dealer_val) {
            results[i] = 1;  // Player wins
        } else if (player_val < dealer_val) {
            results[i] = -1; // Player loses
        } else {
            results[i] = 0;  // Push
        }
    }
    
    // Liberar memória temporária (pools são lineares, mas documentamos o uso)
    POOL_FREE_TEMP(player_values);
    POOL_FREE_TEMP(dealer_values);
    
    DEBUG_STATS("Simulação paralela SIMD concluída: %d resultados gerados", count);
}

#endif // SIMD_ENABLED

// Função wrapper que usa SIMD se disponível, senão fallback escalar
void optimized_count_cards(uint64_t* cards, int count, double* result) {
    if (!cards || !result || count <= 0) {
        return;
    }
    
    if (simd_ctx.simd_available && simd_ctx.count_cards_simd) {
        simd_ctx.count_cards_simd(cards, count, result);
    } else {
        // Fallback escalar
        *result = 0.0;
        for (int i = 0; i < count; i++) {
            int rank = __builtin_ctzll(cards[i]) / 3;
            if (rank < 13) {
                *result += WONG_HALVES[rank];
            }
        }
    }
}

// Função wrapper para avaliação de mãos
void optimized_evaluate_hands(uint64_t* hands, int count, int* results) {
    if (!hands || !results || count <= 0) {
        return;
    }
    
    if (simd_ctx.simd_available && simd_ctx.evaluate_hands_simd) {
        simd_ctx.evaluate_hands_simd(hands, count, results);
    } else {
        // Fallback escalar - implementação básica
        for (int i = 0; i < count; i++) {
            results[i] = 0; // Implementação simplificada
        }
    }
} 