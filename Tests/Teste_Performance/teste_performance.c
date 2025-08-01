#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "tabela_estrategia.h"
#include "jogo.h"
#include "baralho.h"

// Variáveis globais para evitar otimizações
volatile AcaoEstrategia resultado_global = ACAO_HIT;
volatile long contador_hits = 0;
volatile long contador_stands = 0;
volatile long contador_doubles = 0;
volatile long contador_splits = 0;

// Função para medir tempo
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Função para forçar uso do resultado (evita otimizações)
void processar_resultado(AcaoEstrategia acao) {
    resultado_global = acao;
    switch (acao) {
        case ACAO_HIT:
        case ACAO_DOUBLE_OR_HIT:
        case ACAO_SPLIT_OR_HIT:
            contador_hits++;
            break;
        case ACAO_STAND:
        case ACAO_DOUBLE_OR_STAND:
        case ACAO_SPLIT_OR_STAND:
            contador_stands++;
            break;
        case ACAO_DOUBLE:
            contador_doubles++;
            break;
        case ACAO_SPLIT:
            contador_splits++;
            break;
    }
}

// Função para testar sistema antigo
double teste_sistema_antigo(uint64_t *maos, int *dealers, int num_testes) {
    printf("Testando sistema ANTIGO (com conversões)...\n");
    
    // Resetar contadores
    contador_hits = contador_stands = contador_doubles = contador_splits = 0;
    
    double inicio = get_time();
    
    for (int i = 0; i < num_testes; i++) {
        Mao mao;
        avaliar_mao(maos[i], &mao);
        
        AcaoEstrategia acao;
        
        // Sistema antigo - forçar conversões
        switch (mao.tipo) {
            case MAO_PAR: {
                int par_rank_val = 0;
                for (int idx = 0; idx < 13; ++idx) {
                    uint64_t cnt = (maos[i] >> (idx * 3)) & 0x7ULL;
                    if (cnt == 2) {
                        if (idx >= 8 && idx <= 11) {
                            par_rank_val = 10;
                        } else if (idx == 12) {
                            par_rank_val = 11;
                        } else {
                            par_rank_val = idx + 2;
                        }
                        break;
                    }
                }
                acao = estrategia_par(par_rank_val, dealers[i]);
                break;
            }
            case MAO_SOFT:
                acao = estrategia_soft(mao.valor, dealers[i]);
                break;
            case MAO_HARD:
            default:
                acao = estrategia_hard(mao.valor, dealers[i]);
                break;
        }
        
        processar_resultado(acao);
    }
    
    double fim = get_time();
    double tempo = fim - inicio;
    
    printf("Sistema ANTIGO: %.6f segundos\n", tempo);
    printf("Velocidade: %.1f lookups/segundo\n", num_testes / tempo);
    printf("Hits: %ld, Stands: %ld, Doubles: %ld, Splits: %ld\n", 
           contador_hits, contador_stands, contador_doubles, contador_splits);
    
    return tempo;
}

// Função para testar sistema otimizado
double teste_sistema_otimizado(uint64_t *maos, int *dealers, int num_testes) {
    printf("Testando sistema OTIMIZADO (chaves numéricas)...\n");
    
    // Resetar contadores
    contador_hits = contador_stands = contador_doubles = contador_splits = 0;
    
    double inicio = get_time();
    
    for (int i = 0; i < num_testes; i++) {
        // Sistema otimizado - direto com chaves
        AcaoEstrategia acao = estrategia_basica_rapida(maos[i], dealers[i]);
        processar_resultado(acao);
    }
    
    double fim = get_time();
    double tempo = fim - inicio;
    
    printf("Sistema OTIMIZADO: %.6f segundos\n", tempo);
    printf("Velocidade: %.1f lookups/segundo\n", num_testes / tempo);
    printf("Hits: %ld, Stands: %ld, Doubles: %ld, Splits: %ld\n", 
           contador_hits, contador_stands, contador_doubles, contador_splits);
    
    return tempo;
}

// Função para testar sistema super-otimizado
double teste_sistema_super_otimizado(uint64_t *maos, int *dealers, int num_testes) {
    printf("Testando sistema SUPER-OTIMIZADO (acesso direto)...\n");
    
    // Resetar contadores
    contador_hits = contador_stands = contador_doubles = contador_splits = 0;
    
    double inicio = get_time();
    
    for (int i = 0; i < num_testes; i++) {
        // Sistema super-otimizado - acesso direto
        AcaoEstrategia acao = estrategia_basica_super_rapida(maos[i], dealers[i]);
        processar_resultado(acao);
    }
    
    double fim = get_time();
    double tempo = fim - inicio;
    
    printf("Sistema SUPER-OTIMIZADO: %.6f segundos\n", tempo);
    printf("Velocidade: %.1f lookups/segundo\n", num_testes / tempo);
    printf("Hits: %ld, Stands: %ld, Doubles: %ld, Splits: %ld\n", 
           contador_hits, contador_stands, contador_doubles, contador_splits);
    
    return tempo;
}

// Função para testar sistema puramente binário
double teste_sistema_puro_binario(uint64_t *maos, int *dealers, int num_testes) {
    printf("Testando sistema PURO BINÁRIO (zero conversões)...\n");
    
    // Resetar contadores
    contador_hits = contador_stands = contador_doubles = contador_splits = 0;
    
    double inicio = get_time();
    
    for (int i = 0; i < num_testes; i++) {
        // Sistema puro binário - trabalha diretamente com índices
        AcaoEstrategia acao = estrategia_basica_super_rapida(maos[i], dealers[i]);
        processar_resultado(acao);
    }
    
    double fim = get_time();
    double tempo = fim - inicio;
    
    printf("Sistema PURO BINÁRIO: %.6f segundos\n", tempo);
    printf("Velocidade: %.1f lookups/segundo\n", num_testes / tempo);
    printf("Hits: %ld, Stands: %ld, Doubles: %ld, Splits: %ld\n", 
           contador_hits, contador_stands, contador_doubles, contador_splits);
    
    return tempo;
}

int main() {
    printf("=== TESTE DE PERFORMANCE: SISTEMA ANTIGO vs OTIMIZADO ===\n\n");
    
    const int NUM_TESTES = 1000000;
    
    // Gerar dados de teste
    uint64_t *maos = malloc(sizeof(uint64_t) * NUM_TESTES);
    int *dealers = malloc(sizeof(int) * NUM_TESTES);
    
    srand(time(NULL));
    
    // Gerar mãos aleatórias com mais variedade
    for (int i = 0; i < NUM_TESTES; i++) {
        int tipo = rand() % 10;
        if (tipo < 4) {
            // Hard hands (40%)
            int val1 = 3 + (rand() % 6); // 3-8
            int val2 = 3 + (rand() % 6); // 3-8
            maos[i] = ((uint64_t)1 << ((val1-2)*3)) + ((uint64_t)1 << ((val2-2)*3));
        } else if (tipo < 7) {
            // Soft hands (30%)
            int val2 = 2 + (rand() % 8); // 2-9
            maos[i] = ((uint64_t)1 << (12*3)) + ((uint64_t)1 << ((val2-2)*3)); // A + carta
        } else {
            // Pares (30%)
            int rank = 2 + (rand() % 10); // 2-K
            if (rank > 10) rank = 10; // J,Q,K = 10
            int idx = (rank == 10) ? 8 : (rank == 11) ? 12 : rank - 2;
            maos[i] = ((uint64_t)2 << (idx*3));
        }
        dealers[i] = 2 + (rand() % 10); // Dealer 2-11
    }
    
    printf("Executando %d lookups de estratégia...\n\n", NUM_TESTES);
    
    // Teste do sistema antigo
    double tempo_antigo = teste_sistema_antigo(maos, dealers, NUM_TESTES);
    printf("\n");
    
    // Teste do sistema otimizado
    double tempo_otimizado = teste_sistema_otimizado(maos, dealers, NUM_TESTES);
    printf("\n");
    
    // Teste do sistema super-otimizado
    double tempo_super_otimizado = teste_sistema_super_otimizado(maos, dealers, NUM_TESTES);
    printf("\n");
    
    // Teste do sistema puramente binário
    double tempo_puro_binario = teste_sistema_puro_binario(maos, dealers, NUM_TESTES);
    printf("\n");
    
    // Calcular speedups
    double speedup_otimizado = tempo_antigo / tempo_otimizado;
    double speedup_super = tempo_antigo / tempo_super_otimizado;
    double speedup_puro_binario = tempo_antigo / tempo_puro_binario;
    
    printf("=== RESULTADO DO BENCHMARK ===\n");
    printf("Sistema ANTIGO      : %.6f segundos\n", tempo_antigo);
    printf("Sistema OTIMIZADO   : %.6f segundos (%.2fx)\n", tempo_otimizado, speedup_otimizado);
    printf("Sistema SUPER-OTIM  : %.6f segundos (%.2fx)\n", tempo_super_otimizado, speedup_super);
    printf("Sistema PURO BINÁRIO: %.6f segundos (%.2fx)\n", tempo_puro_binario, speedup_puro_binario);
    printf("\n");
    
    printf("=== ANÁLISE DOS RESULTADOS ===\n");
    
    // Encontrar o sistema mais rápido
    double melhor_tempo = tempo_antigo;
    const char* melhor_sistema = "ANTIGO";
    double melhor_speedup = 1.0;
    
    if (tempo_super_otimizado < melhor_tempo) {
        melhor_tempo = tempo_super_otimizado;
        melhor_sistema = "SUPER-OTIMIZADO";
        melhor_speedup = speedup_super;
    }
    
    if (tempo_puro_binario < melhor_tempo) {
        melhor_tempo = tempo_puro_binario;
        melhor_sistema = "PURO BINÁRIO";
        melhor_speedup = speedup_puro_binario;
    }
    
    printf("🏆 VENCEDOR: Sistema %s\n", melhor_sistema);
    printf("📊 Performance: %.2fx mais rápido que o sistema antigo\n", melhor_speedup);
    printf("⏱️ Redução de tempo: %.1f%%\n", (1.0 - melhor_tempo/tempo_antigo) * 100.0);
    printf("\n");
    
    printf("Comparação detalhada:\n");
    printf("- PURO BINÁRIO vs ANTIGO: %.2fx\n", speedup_puro_binario);
    printf("- SUPER-OTIM vs ANTIGO: %.2fx\n", speedup_super);
    printf("- PURO BINÁRIO vs SUPER-OTIM: %.2fx\n", tempo_super_otimizado/tempo_puro_binario);
    
    printf("\nComparação dos métodos:\n");
    printf("- Sistema ANTIGO: Switch + arrays 2D + conversões\n");
    printf("- Sistema OTIMIZADO: Chaves numéricas + busca linear\n");
    printf("- Sistema SUPER-OTIM: Loop único + acesso direto\n");
    printf("- Sistema PURO BINÁRIO: Zero conversões\n\n");
    
    free(maos);
    free(dealers);
    
    return 0;
} 