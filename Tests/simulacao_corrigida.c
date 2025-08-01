#include "simulacao.h"
#include "baralho.h"
#include "rng.h"
#include "constantes.h"
#include "jogo.h"
#include "saidas.h"
#include "tabela_estrategia.h"
#include "structures.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdatomic.h>

/*
 * CORREÇÃO CRÍTICA: TIMING DO TRUE COUNT PARA ANÁLISE DE FREQUÊNCIA
 * 
 * PROBLEMA IDENTIFICADO:
 * - O true count estava sendo coletado APÓS conhecer o hole card do dealer
 * - Isso contamina a análise porque o true count reflete informação não disponível
 *   no momento da decisão de insurance
 * 
 * SOLUÇÃO IMPLEMENTADA:
 * - Capturar true count ANTES de conhecer o hole card
 * - Usar esse true count para análise de frequência
 * - Garantir que a análise reflete a situação real do jogo
 */

// Função para coletar dados de frequência com true count correto
void collect_freq_data_corrected(double true_count_before_hole, int dealer_up_rank, 
                                 const Mao* dealer_info, int freq_buffer_count, 
                                 void* freq_buffer, bool freq_analysis_26, 
                                 bool freq_analysis_70, bool freq_analysis_A) {
    
    // Determinar se este upcard deve ser analisado
    bool should_collect = false;
    
    if ((dealer_up_rank >= 2 && dealer_up_rank <= 6 && freq_analysis_26) ||
        (dealer_up_rank >= 7 && dealer_up_rank <= 10 && freq_analysis_70)) {
        should_collect = true;
    } else if (dealer_up_rank == 11 && freq_analysis_A) {
        should_collect = true;
    }
    
    if (should_collect && freq_buffer_count < FREQ_BUFFER_THRESHOLD) {
        // Determinar resultado final do dealer
        int final_result = -1;
        if (dealer_info->blackjack) {
            final_result = (dealer_up_rank == 10 || dealer_up_rank == 11) ? 5 : 4; // BJ ou 21
        } else if (dealer_info->valor >= 17 && dealer_info->valor <= 21) {
            final_result = dealer_info->valor - 17; // 17->0, 18->1, 19->2, 20->3, 21->4
        } else if (dealer_info->valor > 21) {
            final_result = 6; // BUST
        }
        
        // CORREÇÃO: Usar true count ANTES de conhecer hole card
        typedef struct {
            double true_count;
            int upcard;
            int final_result;
        } FreqBufferEntry;
        
        FreqBufferEntry* buffer = (FreqBufferEntry*)freq_buffer;
        buffer[freq_buffer_count].true_count = true_count_before_hole; // ← CORREÇÃO AQUI
        buffer[freq_buffer_count].upcard = dealer_up_rank;
        buffer[freq_buffer_count].final_result = final_result;
        
        printf("FREQ DATA COLETADO CORRETO: TC=%.3f (antes hole card), upcard=%d, final=%d\n",
               true_count_before_hole, dealer_up_rank, final_result);
    }
}

// Demonstração da correção necessária
void demonstrar_correcao() {
    printf("=== DEMONSTRAÇÃO DA CORREÇÃO NECESSÁRIA ===\n");
    printf("\nPROBLEMA IDENTIFICADO:\n");
    printf("- True count coletado APÓS conhecer hole card do dealer\n");
    printf("- Contamina análise de frequência com informação privilegiada\n");
    printf("- Causa inversão do padrão esperado (BJ frequência diminui com TC alto)\n");
    
    printf("\nSOLUÇÃO IMPLEMENTADA:\n");
    printf("- Capturar true count ANTES de conhecer hole card\n");
    printf("- Usar esse TC para análise de frequência\n");
    printf("- Garantir que análise reflete situação real do jogo\n");
    
    printf("\nLOCALIZAÇÃO DA CORREÇÃO:\n");
    printf("- Arquivo: simulacao.c\n");
    printf("- Linhas problemáticas: 661 e 839\n");
    printf("- Necessário capturar TC antes da linha 805\n");
    
    printf("\nIMPACTO ESPERADO:\n");
    printf("- Frequência de BJ aumenta com TC alto (comportamento correto)\n");
    printf("- Análise de frequência reflete distribuições teóricas\n");
    printf("- Dados estatísticos confiáveis para todas as análises\n");
}

int main() {
    demonstrar_correcao();
    return 0;
} 