// CORREÇÕES PROPOSTAS PARA O CÓDIGO DE SIMULAÇÃO DE BLACKJACK

// ============================================================================
// CORREÇÃO 1: Expandir range de True Count e melhorar função get_bin_index
// ============================================================================

// Em structures.h - SUBSTITUIR as definições atuais:
#define MAX_BINS 300               // Expandido de 130 para 300
#define BIN_WIDTH 0.1
#define MIN_TC -15.0               // Expandido de -6.5 para -15.0
#define MAX_TC 15.0                // Expandido de 6.5 para 15.0

// Função melhorada para cálculo de bins
static inline int get_bin_index_robust(double true_count) {
    // Clampar valores extremos
    if (true_count < MIN_TC) {
        DEBUG_STATS("TC %.6f abaixo do mínimo %.1f, usando bin 0", true_count, MIN_TC);
        return 0;
    }
    if (true_count >= MAX_TC) {
        DEBUG_STATS("TC %.6f acima do máximo %.1f, usando bin %d", true_count, MAX_TC, MAX_BINS-1);
        return MAX_BINS - 1;
    }
    
    int bin_idx = (int)((true_count - MIN_TC) / BIN_WIDTH);
    
    // Verificação adicional de segurança
    if (bin_idx < 0) bin_idx = 0;
    if (bin_idx >= MAX_BINS) bin_idx = MAX_BINS - 1;
    
    DEBUG_STATS("TC %.6f -> bin %d (range: %.2f a %.2f)", 
               true_count, bin_idx, 
               MIN_TC + bin_idx * BIN_WIDTH, 
               MIN_TC + (bin_idx + 1) * BIN_WIDTH);
    
    return bin_idx;
}

// ============================================================================
// CORREÇÃO 2: Garantir flush de buffers no final da simulação
// ============================================================================

// Em simulacao.c - ADICIONAR no final da função simulacao_completa():
void force_flush_all_buffers() {
    // Forçar flush de todos os buffers antes de terminar a simulação
    if (freq_buffer_count > 0) {
        DEBUG_STATS("Forçando flush de %d registros de frequência", freq_buffer_count);
        flush_freq_buffer();
    }
    
    if (dealer_buffer_count > 0) {
        DEBUG_STATS("Forçando flush de %d registros de dealer", dealer_buffer_count);
        flush_dealer_buffer();
    }
    
    if (log_buffer_count > 0) {
        DEBUG_STATS("Forçando flush de %d registros de log", log_buffer_count);
        flush_log_buffer(log_mutex);
    }
}

// ============================================================================
// CORREÇÃO 3: Melhorar verificação de arquivos temporários
// ============================================================================

// Em main.c - SUBSTITUIR a lógica de abertura de arquivos:
CompressedReader* open_temp_file_with_retry(const char* base_filename, int max_retries) {
    CompressedReader* reader = NULL;
    
    // Tentar diferentes extensões
    const char* extensions[] = {"", ".bin", ".lz4", ".tmp"};
    int num_extensions = sizeof(extensions) / sizeof(extensions[0]);
    
    for (int retry = 0; retry < max_retries && reader == NULL; retry++) {
        for (int ext = 0; ext < num_extensions && reader == NULL; ext++) {
            char full_filename[512];
            snprintf(full_filename, sizeof(full_filename), "%s%s", base_filename, extensions[ext]);
            
            DEBUG_IO("Tentativa %d/%d: tentando abrir %s", retry + 1, max_retries, full_filename);
            
            // Verificar se arquivo existe
            if (access(full_filename, F_OK) == 0) {
                reader = compressed_file_open_read(full_filename);
                if (reader) {
                    DEBUG_IO("Arquivo aberto com sucesso: %s", full_filename);
                    return reader;
                } else {
                    DEBUG_IO("Falha ao abrir arquivo existente: %s", full_filename);
                }
            } else {
                DEBUG_IO("Arquivo não existe: %s", full_filename);
            }
        }
        
        if (reader == NULL && retry < max_retries - 1) {
            DEBUG_IO("Aguardando 100ms antes da próxima tentativa...");
            usleep(100000); // 100ms
        }
    }
    
    if (reader == NULL) {
        DEBUG_IO("ERRO: Não foi possível abrir arquivo após %d tentativas: %s", max_retries, base_filename);
    }
    
    return reader;
}

// ============================================================================
// CORREÇÃO 4: Melhorar coleta de dados de frequência
// ============================================================================

// Em simulacao.c - SUBSTITUIR a lógica de coleta de frequência:
void collect_frequency_data_improved(int dealer_up_rank, int dealer_final, double true_count, 
                                    bool freq_analysis_26, bool freq_analysis_70, bool freq_analysis_A) {
    // Verificar se deve coletar dados para este upcard
    bool should_collect = false;
    
    if (dealer_up_rank >= 2 && dealer_up_rank <= 6 && freq_analysis_26) {
        should_collect = true;
    } else if (dealer_up_rank >= 7 && dealer_up_rank <= 10 && freq_analysis_70) {
        should_collect = true;
    } else if (dealer_up_rank == 11 && freq_analysis_A) {
        should_collect = true;
    }
    
    if (!should_collect) {
        DEBUG_STATS("Não coletando dados para upcard %d (flags: 26=%s, 70=%s, A=%s)", 
                   dealer_up_rank,
                   freq_analysis_26 ? "ON" : "OFF",
                   freq_analysis_70 ? "ON" : "OFF", 
                   freq_analysis_A ? "ON" : "OFF");
        return;
    }
    
    DEBUG_STATS("Coletando dados de frequência: upcard=%d, final=%d, TC=%.3f", 
               dealer_up_rank, dealer_final, true_count);
    
    // Adicionar ao buffer de total
    if (freq_buffer_count < FREQ_BUFFER_SIZE - 1) {
        freq_buffer[freq_buffer_count].upcard = dealer_up_rank;
        freq_buffer[freq_buffer_count].final_result = -1; // Marcador para total
        freq_buffer[freq_buffer_count].true_count = true_count;
        freq_buffer_count++;
    }
    
    // Adicionar ao buffer de resultado específico
    if (freq_buffer_count < FREQ_BUFFER_SIZE - 1) {
        freq_buffer[freq_buffer_count].upcard = dealer_up_rank;
        freq_buffer[freq_buffer_count].final_result = map_dealer_final_to_index(dealer_final);
        freq_buffer[freq_buffer_count].true_count = true_count;
        freq_buffer_count++;
    }
    
    // Flush se buffer está quase cheio
    if (freq_buffer_count >= FREQ_BUFFER_THRESHOLD) {
        DEBUG_STATS("Buffer de frequência quase cheio (%d/%d), fazendo flush", 
                   freq_buffer_count, FREQ_BUFFER_SIZE);
        flush_freq_buffer();
    }
}

// Função auxiliar para mapear resultado final do dealer para índice
int map_dealer_final_to_index(int dealer_final) {
    if (dealer_final == 17) return 0;      // "17"
    if (dealer_final == 18) return 1;      // "18"
    if (dealer_final == 19) return 2;      // "19"
    if (dealer_final == 20) return 3;      // "20"
    if (dealer_final == 21) return 4;      // "21"
    if (dealer_final == -1) return 5;      // "BJ" (blackjack)
    if (dealer_final > 21) return 6;       // "BUST"
    
    DEBUG_STATS("AVISO: dealer_final %d não mapeado, usando BUST", dealer_final);
    return 6; // Default para BUST
}

// ============================================================================
// CORREÇÃO 5: Adicionar verificação de integridade de dados
// ============================================================================

// Função para verificar integridade dos dados coletados
void verify_data_integrity(int num_sims) {
    printf("\n=== VERIFICAÇÃO DE INTEGRIDADE DOS DADOS ===\n");
    
    // Verificar arquivos temporários existentes
    int temp_files_found = 0;
    int num_batches = (num_sims + 19999) / 20000;
    
    for (int batch = 0; batch < num_batches; batch++) {
        // Verificar arquivos de frequência
        for (int upcard = 2; upcard <= 11; upcard++) {
            char filename[512];
            if (upcard == 11) {
                snprintf(filename, sizeof(filename), "temp_total_upcard_A_batch_%d", batch);
            } else {
                snprintf(filename, sizeof(filename), "temp_total_upcard_%d_batch_%d", upcard, batch);
            }
            
            // Tentar diferentes extensões
            const char* extensions[] = {"", ".bin", ".lz4", ".tmp"};
            for (int i = 0; i < 4; i++) {
                char full_filename[512];
                snprintf(full_filename, sizeof(full_filename), "%s%s", filename, extensions[i]);
                if (access(full_filename, F_OK) == 0) {
                    temp_files_found++;
                    printf("  Encontrado: %s\n", full_filename);
                    break;
                }
            }
        }
    }
    
    printf("Total de arquivos temporários encontrados: %d\n", temp_files_found);
    
    if (temp_files_found == 0) {
        printf("ERRO: Nenhum arquivo temporário encontrado!\n");
        printf("Possíveis causas:\n");
        printf("  1. Buffers não foram escritos (muito poucos dados)\n");
        printf("  2. Arquivos foram removidos prematuramente\n");
        printf("  3. Falha na criação de arquivos\n");
        printf("  4. Análises não foram ativadas corretamente\n");
    }
    
    printf("===============================================\n\n");
}

// ============================================================================
// CORREÇÃO 6: Melhorar sistema de debug e logging
// ============================================================================

// Adicionar contadores globais para debug
static atomic_int global_freq_records_written = 0;
static atomic_int global_dealer_records_written = 0;
static atomic_int global_split_records_written = 0;

// Função para imprimir estatísticas de debug
void print_debug_statistics() {
    if (!debug_enabled) return;
    
    printf("\n=== ESTATÍSTICAS DE DEBUG ===\n");
    printf("Registros de frequência escritos: %d\n", atomic_load(&global_freq_records_written));
    printf("Registros de dealer escritos: %d\n", atomic_load(&global_dealer_records_written));
    printf("Registros de split escritos: %d\n", atomic_load(&global_split_records_written));
    printf("=============================\n\n");
}

// ============================================================================
// CORREÇÃO 7: Comando de exemplo para execução correta
// ============================================================================

/*
COMANDO CORRETO PARA EXECUTAR A SIMULAÇÃO COM TODAS AS ANÁLISES:

./simulacao -n 15000 -hist26 -hist70 -histA -split -l 10000 -o 15k_SIM -debug

Explicação dos parâmetros:
-n 15000     : 15.000 simulações
-hist26      : Análise de frequência para upcards 2-6
-hist70      : Análise de frequência para upcards 7-10  
-histA       : Análise de frequência para upcard A
-split       : Análise de resultados de splits
-l 10000     : Salvar 10.000 linhas de log
-o 15k_SIM   : Sufixo para arquivos de saída
-debug       : Ativar debug detalhado

NOTA: O parâmetro -dealer foi removido do código atual, mas a lógica
de análise de dealer blackjack ainda existe e pode ser reativada.
*/

