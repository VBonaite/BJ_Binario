#include "main.h"
#include "simulacao.h"
#include "constantes.h"
#include "jogo.h"
#include "structures.h"  // Usar estruturas centralizadas
#include "realtime_strategy_integration.h"  // Para sistema de EV em tempo real
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdatomic.h>
#include <math.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

// Variável global para debug
bool debug_enabled = false;

// Sistemas de otimização foram removidos pois não eram utilizados no fluxo principal
// O sistema usa apenas estrategia_basica_super_rapida() com tabelas inline otimizadas

// Estruturas movidas para structures.h - removendo duplicações

// Funções para arquivos binários com debugging e validação
void write_dealer_binary(FILE* file, double true_count, int ace_upcard, int dealer_bj) {
    if (!file) {
        DEBUG_IO("ERRO: Tentativa de escrever em arquivo dealer NULL");
        return;
    }
    
    DEBUG_IO("Escrevendo dealer binary: TC=%.3f, Ace=%d, BJ=%d", true_count, ace_upcard, dealer_bj);
    
    DealerBinaryRecord record;
    record.true_count = (float)true_count;
    record.ace_upcard = ace_upcard;
    record.dealer_bj = dealer_bj;
    record.checksum = calculate_dealer_checksum(&record);
    
    size_t written = fwrite(&record, sizeof(record), 1, file);
    if (written != 1) {
        DEBUG_IO("ERRO: Falha ao escrever registro dealer (escrito=%zu, esperado=1)", written);
    } else {
        DEBUG_IO("Registro dealer escrito com sucesso: %zu bytes", sizeof(record));
    }
}

void write_split_binary(FILE* file, double true_count, int lose_lose, int win_win, int push_push,
                       int lose_win, int lose_push, int win_lose, int win_push, 
                       int push_lose, int push_win, int cards_used) {
    if (!file) {
        DEBUG_IO("ERRO: Tentativa de escrever em arquivo split NULL");
        return;
    }
    
    DEBUG_IO("Escrevendo split binary: TC=%.3f, Cards=%d", true_count, cards_used);
    DEBUG_STATS("Split combinations: LL=%d WW=%d PP=%d LW=%d LP=%d WL=%d WP=%d PL=%d PW=%d", 
               lose_lose, win_win, push_push, lose_win, lose_push, win_lose, win_push, push_lose, push_win);
    
    SplitBinaryRecord record;
    record.true_count = (float)true_count;
    record.lose_lose = lose_lose;
    record.win_win = win_win;
    record.push_push = push_push;
    record.lose_win = lose_win;
    record.lose_push = lose_push;
    record.win_lose = win_lose;
    record.win_push = win_push;
    record.push_lose = push_lose;
    record.push_win = push_win;
    record.cards_used = cards_used;
    record.checksum = calculate_split_checksum(&record);
    
    // Validar antes de escrever
    if (!validate_split_record(&record)) {
        DEBUG_IO("ERRO: Registro split inválido, não será escrito");
        return;
    }
    
    size_t written = fwrite(&record, sizeof(record), 1, file);
    if (written != 1) {
        DEBUG_IO("ERRO: Falha ao escrever registro split (escrito=%zu, esperado=1)", written);
    } else {
        DEBUG_IO("Registro split escrito com sucesso: %zu bytes", sizeof(record));
    }
}

void write_freq_binary(FILE* file, double true_count) {
    if (!file) {
        DEBUG_IO("ERRO: Tentativa de escrever em arquivo freq NULL");
        return;
    }
    
    DEBUG_IO("Escrevendo freq binary: TC=%.3f", true_count);
    
    FreqBinaryRecord record;
    record.true_count = (float)true_count;
    record.checksum = calculate_freq_checksum(&record);
    
    size_t written = fwrite(&record, sizeof(record), 1, file);
    if (written != 1) {
        DEBUG_IO("ERRO: Falha ao escrever registro freq (escrito=%zu, esperado=1)", written);
    } else {
        DEBUG_IO("Registro freq escrito com sucesso: %zu bytes", sizeof(record));
    }
}

// Estrutura para passar dados para as threads
typedef struct {
    int log_level;
    int sim_start;
    int sim_end;
    int thread_id;
    const char* output_suffix;
    atomic_int* global_log_count;
    bool dealer_analysis;
    bool freq_analysis_26;
    bool freq_analysis_70;
    bool freq_analysis_A;
    bool split_analysis;
    bool ev_realtime_enabled;
    pthread_mutex_t* dealer_mutex;
    pthread_mutex_t* freq_mutex;
    pthread_mutex_t* split_mutex;
    bool insurance_analysis;
    pthread_mutex_t* insurance_mutex;
    // Cache line padding para evitar false sharing
    char padding[64];
} __attribute__((aligned(64))) ThreadData;

// Variáveis globais para controle de progresso
static atomic_int completed_sims = 0;
static int total_sims = 0;
static struct timeval start_time;

// Constantes movidas para structures.h - removendo duplicações

// Estrutura para dados do bin
typedef struct {
    double tc_min;
    double tc_max;
    int total_ace_upcards;
    int dealer_blackjacks;
    double percentage;
} BinData;

// Função para mostrar barra de progresso
void show_progress(int current, int total, double elapsed_time) {
    int bar_width = 50;
    double progress = (double)current / total;
    int pos = (int)(bar_width * progress);
    
    printf("\r[");
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    
    double rate = current / elapsed_time;
    double eta = (total - current) / rate;
    
    // Converter ETA para formato hh:mm:ss
    int eta_hours = (int)(eta / 3600);
    int eta_minutes = (int)((eta - eta_hours * 3600) / 60);
    int eta_seconds = (int)(eta - eta_hours * 3600 - eta_minutes * 60);
    
    printf("] %d/%d (%.1f%%) - %.1f sim/s - ETA: %02d:%02d:%02d    ", 
           current, total, progress * 100.0, rate, eta_hours, eta_minutes, eta_seconds);
    fflush(stdout);
}

// Função movida para structures.h como get_bin_index_robust

// Função para processar dados do dealer e gerar CSV
void process_dealer_data(int num_sims, const char* output_suffix) {
    DEBUG_PRINT("Iniciando processamento de dados do dealer");
    DEBUG_STATS("Total de simulações: %d", num_sims);
    
    BinData bins[MAX_BINS];
    
    // Criar diretório Resultados se não existir
    struct stat st = {0};
    if (stat("./Resultados", &st) == -1) {
        if (mkdir("./Resultados", 0755) != 0 && errno != EEXIST) {
            perror("mkdir Resultados");
            return;
        }
        DEBUG_IO("Diretório ./Resultados criado");
    }
    
    // Inicializar bins
    for (int i = 0; i < MAX_BINS; i++) {
        bins[i].tc_min = MIN_TC + i * BIN_WIDTH;
        bins[i].tc_max = bins[i].tc_min + BIN_WIDTH;
        bins[i].total_ace_upcards = 0;
        bins[i].dealer_blackjacks = 0;
        bins[i].percentage = 0.0;
    }
    DEBUG_STATS("Inicializados %d bins de %.1f a %.1f com largura %.1f", 
               MAX_BINS, MIN_TC, MAX_TC, BIN_WIDTH);
    
    // Processar arquivos temporários binários por lote (CORRIGIDO: usar DEALER_BATCH_SIZE)
    int num_batches = (num_sims + DEALER_BATCH_SIZE - 1) / DEALER_BATCH_SIZE;  
    DEBUG_STATS("Processando %d lotes de %d simulações cada", num_batches, DEALER_BATCH_SIZE);
    
    for (int batch = 0; batch < num_batches; batch++) {
        char temp_filename[256];
        snprintf(temp_filename, sizeof(temp_filename), "%s%d%s", DEALER_TEMP_FILE_PREFIX, batch, BINARY_SUFFIX);
        
        DEBUG_IO("Processando lote %d: %s", batch, temp_filename);
        
        // Verificar integridade do arquivo
        if (!verify_file_integrity(temp_filename, sizeof(DealerBinaryRecord))) {
            DEBUG_IO("Arquivo %s falhou na verificação de integridade, pulando", temp_filename);
            continue;
        }
        
        FILE* temp_file = fopen(temp_filename, "rb");
        if (!temp_file) {
            DEBUG_IO("Não foi possível abrir arquivo: %s", temp_filename);
            continue;
        }
        
        int records_processed = 0;
        int valid_records = 0;
        DealerBinaryRecord record;
        
        while (fread(&record, sizeof(record), 1, temp_file) == 1) {
            records_processed++;
            
            // Validar registro
            if (!validate_dealer_record(&record)) {
                DEBUG_STATS("Registro inválido no lote %d, posição %d", batch, records_processed);
                continue;
            }
            
            valid_records++;
            
            if (record.ace_upcard == 1) { // Só processar quando upcard é Ás
                int bin_idx = get_bin_index_robust(record.true_count);
                if (bin_idx >= 0 && bin_idx < MAX_BINS) {
                    bins[bin_idx].total_ace_upcards++;
                    bins[bin_idx].dealer_blackjacks += record.dealer_bj;
                }
            }
        }
        
        fclose(temp_file);
        unlink(temp_filename); // Remover arquivo temporário
        
        DEBUG_STATS("Lote %d processado: %d registros lidos, %d válidos", 
                   batch, records_processed, valid_records);
    }
    
    // Calcular percentuais
    int total_ace_situations = 0;
    int total_dealer_bjs = 0;
    
    for (int i = 0; i < MAX_BINS; i++) {
        if (bins[i].total_ace_upcards > 0) {
            bins[i].percentage = (double)bins[i].dealer_blackjacks / bins[i].total_ace_upcards * 100.0;
            total_ace_situations += bins[i].total_ace_upcards;
            total_dealer_bjs += bins[i].dealer_blackjacks;
        }
    }
    
    DEBUG_STATS("Total de situações com upcard Ás: %d", total_ace_situations);
    DEBUG_STATS("Total de dealer blackjacks: %d", total_dealer_bjs);
    if (total_ace_situations > 0) {
        DEBUG_STATS("Percentual geral de dealer BJ com upcard Ás: %.2f%%", 
                   (double)total_dealer_bjs / total_ace_situations * 100.0);
    }
    
    // Gerar CSV
    char csv_filename[256];
    snprintf(csv_filename, sizeof(csv_filename), "./Resultados/dealer_blackjack_%s.csv", output_suffix ? output_suffix : "sim");
    
    DEBUG_IO("Gerando CSV: %s", csv_filename);
    
    FILE* csv_file = fopen(csv_filename, "w");
    if (!csv_file) {
        fprintf(stderr, "Erro ao criar arquivo CSV de dealer\n");
        return;
    }
    
    fprintf(csv_file, "true_count_min,true_count_max,true_count_center,total_ace_upcards,dealer_blackjacks,percentage\n");
    
    int bins_with_data = 0;
    for (int i = 0; i < MAX_BINS; i++) {
        if (bins[i].total_ace_upcards > 0) {
            double tc_center = bins[i].tc_min + BIN_WIDTH / 2.0;
            fprintf(csv_file, "%.2f,%.2f,%.2f,%d,%d,%.4f\n",
                   bins[i].tc_min, bins[i].tc_max, tc_center,
                   bins[i].total_ace_upcards, bins[i].dealer_blackjacks, bins[i].percentage);
            bins_with_data++;
        }
    }
    
    fclose(csv_file);
    
    DEBUG_STATS("CSV gerado com %d bins contendo dados", bins_with_data);
    printf("Análise de dealer concluída!\n");
    printf("  CSV gerado: %s\n", csv_filename);
    printf("  Bins com dados: %d de %d\n", bins_with_data, MAX_BINS);
}

// Função executada por cada thread
void* worker_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    // Cache local para reduzir acesso à memória compartilhada
    int local_completed = 0;
    const int update_interval = 100; // Atualizar progresso a cada 100 simulações
    
    for (int i = data->sim_start; i < data->sim_end; ++i) {
        simulacao_completa(data->log_level, i, data->output_suffix, data->global_log_count, data->dealer_analysis, data->freq_analysis_26, data->freq_analysis_70, data->freq_analysis_A, data->split_analysis, data->ev_realtime_enabled, data->dealer_mutex, data->freq_mutex, data->split_mutex, data->insurance_analysis, data->insurance_mutex);
        
        local_completed++;
        
        // Atualizar progresso periodicamente para reduzir contenção
        if (local_completed % update_interval == 0) {
            int current = atomic_fetch_add(&completed_sims, update_interval) + update_interval;
            
            // Mostrar progresso a cada 500 simulações para reduzir overhead mas manter visibilidade
            if (current % 500 == 0) {
                struct timeval current_time;
                gettimeofday(&current_time, NULL);
                double elapsed_time = (current_time.tv_sec - start_time.tv_sec) + 
                                    (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
                
                if (elapsed_time > 0) {
                    show_progress(current, total_sims, elapsed_time);
                }
            }
        }
    }
    
    // Atualizar progresso final para simulações restantes
    if (local_completed % update_interval != 0) {
        int remaining = local_completed % update_interval;
        atomic_fetch_add(&completed_sims, remaining);
    }
    
    // Garantir que pelo menos uma barra de progresso seja mostrada para simulações pequenas
    if (data->sim_end - data->sim_start <= 500) {
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        double elapsed_time = (current_time.tv_sec - start_time.tv_sec) + 
                            (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
        
        if (elapsed_time > 0) {
            int current = atomic_load(&completed_sims);
            show_progress(current, total_sims, elapsed_time);
        }
    }
    
    return NULL;
}

void print_usage(const char* program_name) {
    printf("Uso: %s [OPTIONS]\n", program_name);
    printf("Simulador de Blackjack de Alta Performance\n\n");
    printf("Opções:\n");
    printf("  -l <num>    Número total de linhas de log (0 = sem log) [default: 0]\n");
    printf("  -n <num>    Número de simulações [default: 1000]\n");
    printf("  -t <num>    Número de threads [default: número de CPUs]\n");
    printf("  -o <suffix> Sufixo para arquivos de saída (log_<suffix>.csv) [default: sim]\n");
    printf("  -debug      Ativar debug extensivo (desativado por padrão)\n");
    printf("  -hist26     Ativar análise de frequência para upcards 2-6 do dealer\n");
    printf("  -hist70     Ativar análise de frequência para upcards 7-10 do dealer\n");
    printf("  -histA      Ativar análise de frequência para upcard A do dealer\n");
    printf("  -split      Ativar análise de resultados de splits\n");
    printf("  -ev         Ativar EV em tempo real (desativado por padrão)\n");
    printf("  -ins        Ativar análise de insurance\n");
    printf("  -h          Mostrar esta ajuda\n\n");
    printf("Exemplos:\n");
    printf("  %s -l 0 -n 1000        # Rodar 1000 simulações sem log\n", program_name);
    printf("  %s -l 1000 -n 100      # Rodar 100 simulações salvando 1000 linhas total\n", program_name);
    printf("  %s -n 10000 -t 8       # Rodar 10000 simulações com 8 threads\n", program_name);
    printf("  %s -l 500 -o teste     # Salvar 500 linhas total como log_teste.csv\n", program_name);
    // Comentado: exemplo de uso -dealer
    printf("  %s -hist26 -n 10000 -o analysis # Análise de frequência 2-6 vs TC\n", program_name);
    printf("  %s -hist70 -n 10000 -o analysis # Análise de frequência 7-10 vs TC\n", program_name);
    printf("  %s -histA -n 10000 -o analysis # Análise de frequência A vs TC\n", program_name);
    printf("  %s -split -n 50000 -o split_test # Análise de resultados de splits\n", program_name);
    printf("  %s -ev -n 10000 -o ev_test # Usar EV em tempo real\n", program_name);
    printf("  %s -ins -n 10000 -o ins_test # Análise de insurance\n", program_name);
}

// Função para concatenar arquivos de log e limpar arquivos individuais
// Função para processar dados de bust do jogador


// Função para processar dados de split
void process_split_data(int num_sims, const char* output_suffix) {
    DEBUG_PRINT("Iniciando processamento de dados de split");
    DEBUG_STATS("Total de simulações: %d", num_sims);
    
    // Criar diretório Resultados se não existir
    struct stat st = {0};
    if (stat("./Resultados", &st) == -1) {
        if (mkdir("./Resultados", 0755) != 0 && errno != EEXIST) {
            perror("mkdir Resultados");
            return;
        }
        DEBUG_IO("Diretório ./Resultados criado");
    }
    
    // Todos os pares: AA, 1010, 99, 88, 77, 66, 55, 44, 33, 22
    // Removidos JJ, QQ, KK conforme solicitado
    const char* pairs[] = {"AA", "1010", "99", "88", "77", "66", "55", "44", "33", "22"};
    const int pair_ranks[] = {12, 8, 7, 6, 5, 4, 3, 2, 1, 0}; // índices 0-9 para A,10,9,8,7,6,5,4,3,2
    (void)pair_ranks; // evitar warning de variável não usada
    
    // Todas as upcards: 2, 3, 4, 5, 6, 7, 8, 9, 10, A
    const char* upcards[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "A"};
    const int upcard_ranks[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    (void)upcard_ranks;
    
    // Processar cada combinação de par e upcard
    for (int p = 0; p < 10; p++) {
        for (int u = 0; u < 10; u++) {
            // Estrutura para acumular dados por bin (CORRIGIDA)
            typedef struct {
                double tc_min, tc_max;
                int total_splits;
                // Combinações reais de resultados
                int lose_lose, win_win, push_push;
                int lose_win, lose_push, win_lose;
                int win_push, push_lose, push_win;
                int total_cards_used;
                double total_cards_squared; // Para desvio padrão
            } SplitBinData;
            
            SplitBinData bins[MAX_BINS];
            
            // Inicializar bins
            for (int i = 0; i < MAX_BINS; i++) {
                bins[i].tc_min = MIN_TC + i * BIN_WIDTH;
                bins[i].tc_max = bins[i].tc_min + BIN_WIDTH;
                bins[i].total_splits = 0;
                bins[i].lose_lose = bins[i].win_win = bins[i].push_push = 0;
                bins[i].lose_win = bins[i].lose_push = bins[i].win_lose = 0;
                bins[i].win_push = bins[i].push_lose = bins[i].push_win = 0;
                bins[i].total_cards_used = 0;
                bins[i].total_cards_squared = 0.0;
            }
            
            // Processar arquivos temporários binários por lote (CORRIGIDO: usar SPLIT_BATCH_SIZE)
            int num_batches = (num_sims + SPLIT_BATCH_SIZE - 1) / SPLIT_BATCH_SIZE;
            DEBUG_STATS("Processando %d lotes de splits para %s vs %s", num_batches, pairs[p], upcards[u]);
            
            for (int batch = 0; batch < num_batches; batch++) {
                char temp_filename[512];
                snprintf(temp_filename, sizeof(temp_filename), "%s%d_%s_vs_%s%s", 
                        SPLIT_TEMP_FILE_PREFIX, batch, pairs[p], upcards[u], BINARY_SUFFIX);
                
                DEBUG_IO("Processando arquivo split: %s", temp_filename);
                
                // Verificar integridade do arquivo
                if (!verify_file_integrity(temp_filename, sizeof(SplitBinaryRecord))) {
                    DEBUG_IO("Arquivo %s falhou na verificação de integridade, pulando", temp_filename);
                    continue;
                }
                
                FILE* temp_file = fopen(temp_filename, "rb");
                if (!temp_file) {
                    DEBUG_IO("Não foi possível abrir arquivo: %s", temp_filename);
                    continue;
                }
                
                int records_processed = 0;
                int valid_records = 0;
                SplitBinaryRecord record;
                
                while (fread(&record, sizeof(record), 1, temp_file) == 1) {
                    records_processed++;
                    
                    // Validar usando a função robusta
                    if (!validate_split_record(&record)) {
                        DEBUG_STATS("Registro split inválido no lote %d, posição %d para %s vs %s", 
                                   batch, records_processed, pairs[p], upcards[u]);
                        continue;
                    }
                    
                    valid_records++;
                    
                    int bin_idx = get_bin_index_robust(record.true_count);
                    if (bin_idx >= 0 && bin_idx < MAX_BINS) {
                        bins[bin_idx].total_splits++;
                        bins[bin_idx].lose_lose += record.lose_lose;
                        bins[bin_idx].win_win += record.win_win;
                        bins[bin_idx].push_push += record.push_push;
                        bins[bin_idx].lose_win += record.lose_win;
                        bins[bin_idx].lose_push += record.lose_push;
                        bins[bin_idx].win_lose += record.win_lose;
                        bins[bin_idx].win_push += record.win_push;
                        bins[bin_idx].push_lose += record.push_lose;
                        bins[bin_idx].push_win += record.push_win;
                        bins[bin_idx].total_cards_used += record.cards_used;
                        bins[bin_idx].total_cards_squared += (double)(record.cards_used * record.cards_used);
                    }
                }
                
                fclose(temp_file);
                unlink(temp_filename); // Remover arquivo temporário
                
                DEBUG_STATS("Lote %d de split %s vs %s processado: %d registros lidos, %d válidos", 
                           batch, pairs[p], upcards[u], records_processed, valid_records);
            }
            
            // Gerar CSV final
            char csv_filename[512];
            if (output_suffix) {
                snprintf(csv_filename, sizeof(csv_filename), "./Resultados/split_outcome_%s_vs_%s_%s.csv", 
                        pairs[p], upcards[u], output_suffix);
            } else {
                snprintf(csv_filename, sizeof(csv_filename), "./Resultados/split_outcome_%s_vs_%s_sim.csv", 
                        pairs[p], upcards[u]);
            }
            
            FILE* csv_file = fopen(csv_filename, "w");
            if (!csv_file) {
                fprintf(stderr, "Erro ao criar arquivo CSV de split: %s\n", csv_filename);
                continue;
            }
            
            // Escrever cabeçalho
            fprintf(csv_file, "true_count_min,true_count_max,true_count_center,total_splits,total_hands,mao1_lose&mao2_lose_frequency,mao1_win&mao2_win_frequency,mao1_push&mao2_push_frequency,mao1_lose&mao2_win_frequency,mao1_lose&mao2_push_frequency,mao1_win&mao2_lose_frequency,mao1_win&mao2_push_frequency,mao1_push&mao2_lose_frequency,mao1_push&mao2_win_frequency,expected_value,avg_cards_used,std_cards_used\n");
            
            // Processar cada bin
            for (int i = 0; i < MAX_BINS; i++) {
                if (bins[i].total_splits >= 1) { // Amostra mínima
                    // Calcular totais das duas mãos
                    int total_hands = bins[i].total_splits * 2; // Cada split produz 2 mãos
                    double tc_center = bins[i].tc_min + BIN_WIDTH / 2.0;
                    
                    // Calcular frequências das combinações (MÉTODO CORRETO)
                    int total_splits = bins[i].total_splits;
                    if (total_splits > 0) {
                        // Usar contagens reais das combinações ao invés de multiplicar probabilidades
                        double freq_lose_lose = (double)bins[i].lose_lose / total_splits;
                        double freq_win_win = (double)bins[i].win_win / total_splits;
                        double freq_push_push = (double)bins[i].push_push / total_splits;
                        double freq_lose_win = (double)bins[i].lose_win / total_splits;
                        double freq_lose_push = (double)bins[i].lose_push / total_splits;
                        double freq_win_lose = (double)bins[i].win_lose / total_splits;
                        double freq_win_push = (double)bins[i].win_push / total_splits;
                        double freq_push_lose = (double)bins[i].push_lose / total_splits;
                        double freq_push_win = (double)bins[i].push_win / total_splits;
                        
                        // Calcular expected value CORRIGIDO: considerar todas as 9 combinações
                        // EV = -2*P(lose/lose) + 2*P(win/win) + 0*P(push/push) + 0*P(lose/win) 
                        //      + (-1)*P(lose/push) + 0*P(win/lose) + 1*P(win/push) + (-1)*P(push/lose) + 1*P(push/win)
                        double expected_value = -2.0 * freq_lose_lose + 2.0 * freq_win_win 
                                               - freq_lose_push + freq_win_push 
                                               - freq_push_lose + freq_push_win;
                        
                        // Calcular estatísticas de cartas
                        double avg_cards = (double)bins[i].total_cards_used / bins[i].total_splits;
                        double variance = (bins[i].total_cards_squared / bins[i].total_splits) - (avg_cards * avg_cards);
                        double std_cards = sqrt(variance > 0 ? variance : 0);
                        
                        fprintf(csv_file, "%.2f,%.2f,%.2f,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f,%.2f\n",
                               bins[i].tc_min, bins[i].tc_max, tc_center, total_splits, total_hands,
                               freq_lose_lose, freq_win_win, freq_push_push, freq_lose_win, freq_lose_push,
                               freq_win_lose, freq_win_push, freq_push_lose, freq_push_win, expected_value,
                               avg_cards, std_cards);
                    }
                }
            }
            
            fclose(csv_file);
        }
    }
    
    printf("Análise de splits concluída!\n");
}

// Função para processar dados de frequência do dealer
void process_frequency_data(int num_sims, const char* output_suffix, bool freq_analysis_26, bool freq_analysis_70, bool freq_analysis_A) {
    if (!freq_analysis_26 && !freq_analysis_70 && !freq_analysis_A) {
        return; // Nenhuma análise de frequência ativada
    }
    
    // Criar diretório Resultados se não existir
    struct stat st = {0};
    if (stat("./Resultados", &st) == -1) {
        if (mkdir("./Resultados", 0755) != 0 && errno != EEXIST) {
            perror("mkdir Resultados");
            return;
        }
    }
    
    const char* upcard_names[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "A"};
    const char* final_names[] = {"17", "18", "19", "20", "21", "BJ", "BUST"};
    
    // Determinar quais upcards processar
    int start_upcard = 11, end_upcard = 1; // Valores inválidos = nenhum upcard por padrão
    bool include_ace = false;
    
    if (freq_analysis_26 && freq_analysis_70) {
        // Ambos ativos: 2-6 e 7-10 = 2-10
        start_upcard = 2; end_upcard = 10;
    } else if (freq_analysis_26) {
        // Apenas 2-6
        start_upcard = 2; end_upcard = 6;
    } else if (freq_analysis_70) {
        // Apenas 7-10
        start_upcard = 7; end_upcard = 10;
    }
    
    if (freq_analysis_A) {
        include_ace = true;
    }
    
    // Processar cada upcard
    for (int upcard = start_upcard; upcard <= end_upcard; upcard++) {
        // Estrutura para acumular dados de total por bin para este upcard
        typedef struct {
            double tc_min, tc_max;
            int total_upcard_count;
        } TotalBinData;
        
        TotalBinData total_bins[MAX_BINS];
        
        // Inicializar bins de total
        for (int i = 0; i < MAX_BINS; i++) {
            total_bins[i].tc_min = MIN_TC + i * BIN_WIDTH;
            total_bins[i].tc_max = total_bins[i].tc_min + BIN_WIDTH;
            total_bins[i].total_upcard_count = 0;
        }
        
        // Carregar dados de total para este upcard
        int num_batches = (num_sims + 19999) / 20000;  // Ceil division para 20k por lote
        for (int batch = 0; batch < num_batches; batch++) {
            char total_filename[512];
            snprintf(total_filename, sizeof(total_filename), "temp_total_upcard_%d_batch_%d%s", upcard, batch, BINARY_SUFFIX);
            
            FILE* total_file = fopen(total_filename, "rb");
            if (!total_file) continue;
            
            FreqBinaryRecord record;
            while (fread(&record, sizeof(record), 1, total_file) == 1) {
                int bin_idx = (int)((record.true_count - MIN_TC) / BIN_WIDTH);
                if (bin_idx >= 0 && bin_idx < MAX_BINS) {
                    total_bins[bin_idx].total_upcard_count++;
                }
            }
            
            fclose(total_file);
        }
        
        // Agora processar cada valor final para este upcard
        for (int final_val = 0; final_val < 7; final_val++) {
            // Estrutura para acumular dados por bin
            typedef struct {
                double tc_min, tc_max;
                int final_count;
                double frequency;
            } FreqBinData;
            
            FreqBinData bins[MAX_BINS];
            
            // Inicializar bins usando dados de total já carregados
            for (int i = 0; i < MAX_BINS; i++) {
                bins[i].tc_min = total_bins[i].tc_min;
                bins[i].tc_max = total_bins[i].tc_max;
                bins[i].final_count = 0;
                bins[i].frequency = 0.0;
            }
            
            // Processar arquivo de resultado específico para este upcard/final
            // Pular arquivos BJ para upcards que não podem ter blackjack (2-9)
            if (final_val == 5 && upcard != 10) {
                continue; // Não processar arquivo BJ inexistente para upcards 2-9
            }
            
            for (int batch = 0; batch < num_batches; batch++) {
                char result_filename[512];
                snprintf(result_filename, sizeof(result_filename), "temp_result_upcard_%d_final_%s_batch_%d%s", upcard, final_names[final_val], batch, BINARY_SUFFIX);
                
                FILE* result_file = fopen(result_filename, "rb");
                if (!result_file) continue;
                
                FreqBinaryRecord record;
                while (fread(&record, sizeof(record), 1, result_file) == 1) {
                    int bin_idx = (int)((record.true_count - MIN_TC) / BIN_WIDTH);
                    if (bin_idx >= 0 && bin_idx < MAX_BINS) {
                        bins[bin_idx].final_count++;
                    }
                }
                
                fclose(result_file);
                unlink(result_filename); // Remover arquivo temporário de resultado
            }
            
            // Calcular frequências
            for (int i = 0; i < MAX_BINS; i++) {
                if (total_bins[i].total_upcard_count > 0) {
                    bins[i].frequency = (double)bins[i].final_count / total_bins[i].total_upcard_count * 100.0;
                }
            }
            
            // Gerar CSV final apenas se há dados ou se não é arquivo BJ inválido
            if (final_val == 5 && upcard != 10) {
                // Não gerar arquivo CSV BJ para upcards 2-9 (sempre vazios)
                continue;
            }
            
            char csv_filename[512];
            if (output_suffix) {
                snprintf(csv_filename, sizeof(csv_filename), "./Resultados/freq_%s_%s_%s.csv", upcard_names[upcard-2], final_names[final_val], output_suffix);
            } else {
                snprintf(csv_filename, sizeof(csv_filename), "./Resultados/freq_%s_%s_sim.csv", upcard_names[upcard-2], final_names[final_val]);
            }
            
            FILE* csv_file = fopen(csv_filename, "w");
            if (!csv_file) {
                fprintf(stderr, "Erro ao criar arquivo CSV de frequência: %s\n", csv_filename);
                continue;
            }
            
            fprintf(csv_file, "true_count_min,true_count_max,true_count_center,total_upcard_count,final_count,frequency\n");
            
            for (int i = 0; i < MAX_BINS; i++) {
                if (total_bins[i].total_upcard_count > 0) {
                    double tc_center = bins[i].tc_min + BIN_WIDTH / 2.0;
                    fprintf(csv_file, "%.2f,%.2f,%.2f,%d,%d,%.4f\n",
                           bins[i].tc_min, bins[i].tc_max, tc_center,
                           total_bins[i].total_upcard_count, bins[i].final_count, bins[i].frequency);
                }
            }
            
            fclose(csv_file);
        }
    }
    
    // Remover arquivos de total para upcards 2-10 após processar todos os finais
    for (int upcard = start_upcard; upcard <= end_upcard; upcard++) {
        int num_batches = (num_sims + 19999) / 20000;
        for (int batch = 0; batch < num_batches; batch++) {
            char total_filename[512];
            snprintf(total_filename, sizeof(total_filename), "temp_total_upcard_%d_batch_%d%s", upcard, batch, BINARY_SUFFIX);
            unlink(total_filename);
        }
    }
    
    // Processar upcard A se necessário
    if (include_ace) {
        // Estrutura para acumular dados de total por bin para upcard A
        typedef struct {
            double tc_min, tc_max;
            int total_upcard_count;
        } TotalBinData;
        
        TotalBinData total_bins[MAX_BINS];
        
        // Inicializar bins de total
        for (int i = 0; i < MAX_BINS; i++) {
            total_bins[i].tc_min = MIN_TC + i * BIN_WIDTH;
            total_bins[i].tc_max = total_bins[i].tc_min + BIN_WIDTH;
            total_bins[i].total_upcard_count = 0;
        }
        
        // Carregar dados de total para upcard A
        int num_batches = (num_sims + 19999) / 20000;  // Ceil division para 20k por lote
        for (int batch = 0; batch < num_batches; batch++) {
            char total_filename[512];
            snprintf(total_filename, sizeof(total_filename), "temp_total_upcard_A_batch_%d%s", batch, BINARY_SUFFIX);
            
            FILE* total_file = fopen(total_filename, "rb");
            if (!total_file) continue;
            
            FreqBinaryRecord record;
            while (fread(&record, sizeof(record), 1, total_file) == 1) {
                int bin_idx = (int)((record.true_count - MIN_TC) / BIN_WIDTH);
                if (bin_idx >= 0 && bin_idx < MAX_BINS) {
                    total_bins[bin_idx].total_upcard_count++;
                }
            }
            
            fclose(total_file);
        }
        
        // Agora processar cada valor final para upcard A
        for (int final_val = 0; final_val < 7; final_val++) {
            // Estrutura para acumular dados por bin
            typedef struct {
                double tc_min, tc_max;
                int final_count;
                double frequency;
            } FreqBinData;
            
            FreqBinData bins[MAX_BINS];
            
            // Inicializar bins usando dados de total já carregados
            for (int i = 0; i < MAX_BINS; i++) {
                bins[i].tc_min = total_bins[i].tc_min;
                bins[i].tc_max = total_bins[i].tc_max;
                bins[i].final_count = 0;
                bins[i].frequency = 0.0;
            }
            
            // Processar arquivo de resultado específico para upcard A/final
            for (int batch = 0; batch < num_batches; batch++) {
                char result_filename[512];
                snprintf(result_filename, sizeof(result_filename), "temp_result_upcard_A_final_%s_batch_%d%s", final_names[final_val], batch, BINARY_SUFFIX);
                
                FILE* result_file = fopen(result_filename, "rb");
                if (!result_file) continue;
                
                FreqBinaryRecord record;
                while (fread(&record, sizeof(record), 1, result_file) == 1) {
                    int bin_idx = (int)((record.true_count - MIN_TC) / BIN_WIDTH);
                    if (bin_idx >= 0 && bin_idx < MAX_BINS) {
                        bins[bin_idx].final_count++;
                    }
                }
                
                fclose(result_file);
                unlink(result_filename); // Remover arquivo temporário de resultado
            }
            
            // Calcular frequências
            for (int i = 0; i < MAX_BINS; i++) {
                if (total_bins[i].total_upcard_count > 0) {
                    bins[i].frequency = (double)bins[i].final_count / total_bins[i].total_upcard_count * 100.0;
                }
            }
            
            // Gerar CSV final
            char csv_filename[512];
            if (output_suffix) {
                snprintf(csv_filename, sizeof(csv_filename), "./Resultados/freq_A_%s_%s.csv", final_names[final_val], output_suffix);
            } else {
                snprintf(csv_filename, sizeof(csv_filename), "./Resultados/freq_A_%s_sim.csv", final_names[final_val]);
            }
            
            FILE* csv_file = fopen(csv_filename, "w");
            if (!csv_file) {
                fprintf(stderr, "Erro ao criar arquivo CSV de frequência: %s\n", csv_filename);
                continue;
            }
            
            fprintf(csv_file, "true_count_min,true_count_max,true_count_center,total_upcard_count,final_count,frequency\n");
            
            for (int i = 0; i < MAX_BINS; i++) {
                if (total_bins[i].total_upcard_count > 0) {
                    double tc_center = bins[i].tc_min + BIN_WIDTH / 2.0;
                    fprintf(csv_file, "%.2f,%.2f,%.2f,%d,%d,%.4f\n",
                           bins[i].tc_min, bins[i].tc_max, tc_center,
                           total_bins[i].total_upcard_count, bins[i].final_count, bins[i].frequency);
                }
            }
            
            fclose(csv_file);
        }
        
        // Remover arquivos de total para upcard A após processar todos os finais
        for (int batch = 0; batch < num_batches; batch++) {
            char total_filename[512];
            snprintf(total_filename, sizeof(total_filename), "temp_total_upcard_A_batch_%d%s", batch, BINARY_SUFFIX);
            unlink(total_filename);
        }
    }
    
    printf("Análise de frequência concluída!\n");
}

// Função para salvar análise de constantes
void salvar_analise_constantes(double unidade_media_por_shoe) {
    // Criar timestamp YYYYMMDDHHmmss
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[16];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);
    
    // Abrir arquivo para append
    FILE *file = fopen("/mnt/dados/BJ_Binario/Resultados/analise_constantes.txt", "a");
    if (!file) {
        fprintf(stderr, "Erro ao abrir arquivo analise_constantes.txt\n");
        return;
    }
    
    fprintf(file, "\nSIMULAÇÃO #%s\n\n", timestamp);
    fprintf(file, "MIN_COUNT_INS = %.1f\n\n", MIN_COUNT_INS);
    
    fprintf(file, "MIN_PCT[12] = {\n    ");
    for (int i = 0; i < 12; i++) {
        fprintf(file, "%.1f%s", MIN_PCT[i], (i < 11) ? ", " : "");
        if ((i + 1) % 4 == 0 && i < 11) fprintf(file, "\n    ");
    }
    fprintf(file, "\n}\n\n");
    
    fprintf(file, "MIN_LEN_SHOE[12] = {\n    ");
    for (int i = 0; i < 12; i++) {
        fprintf(file, "%d%s", MIN_LEN_SHOE[i], (i < 11) ? ", " : "");
        if ((i + 1) % 6 == 0 && i < 11) fprintf(file, "\n    ");
    }
    fprintf(file, "\n}\n\n");
    
    fprintf(file, "MIN_TRUE_COUNT[12] = {\n    ");
    for (int i = 0; i < 12; i++) {
        fprintf(file, "%.2f%s", MIN_TRUE_COUNT[i], (i < 11) ? ", " : "");
        if ((i + 1) % 4 == 0 && i < 11) fprintf(file, "\n    ");
    }
    fprintf(file, "\n}\n\n");
    
    fprintf(file, "TC_MAOS_CONTAB[3] = {\n    ");
    for (int i = 0; i < 3; i++) {
        fprintf(file, "%.2f%s", TC_MAOS_CONTAB[i], (i < 2) ? ", " : "");
    }
    fprintf(file, "\n}\n\n");
    
    fprintf(file, "TC_AJUSTA_UNIDADES[10] = {\n    ");
    for (int i = 0; i < 10; i++) {
        fprintf(file, "%.2f%s", TC_AJUSTA_UNIDADES[i], (i < 9) ? ", " : "");
        if ((i + 1) % 5 == 0 && i < 9) fprintf(file, "\n    ");
    }
    fprintf(file, "\n}\n\n");
    
    fprintf(file, "APOSTAS_BASE_AJUSTE[10] = {\n    ");
    for (int i = 0; i < 10; i++) {
        fprintf(file, "%.1f%s", APOSTAS_BASE_AJUSTE[i], (i < 9) ? ", " : "");
        if ((i + 1) % 5 == 0 && i < 9) fprintf(file, "\n    ");
    }
    fprintf(file, "\n}\n\n");
    
    fprintf(file, "APOSTAS_BASE[12] = {\n    ");
    for (int i = 0; i < 12; i++) {
        fprintf(file, "%.1f%s", APOSTAS_BASE[i], (i < 11) ? ", " : "");
        if ((i + 1) % 3 == 0 && i < 11) fprintf(file, "\n    ");
    }
    fprintf(file, "\n}\n\n");
    
    fprintf(file, "CARTAS_RESTANTES_LIMITE = %d\n", CARTAS_RESTANTES_LIMITE);
    fprintf(file, "CARTAS_RESTANTES_SHOE_OK = %d\n\n", CARTAS_RESTANTES_SHOE_OK);
    
    fprintf(file, "Média de unidades por shoe (pnl): %.6f\n\n", unidade_media_por_shoe);
    fprintf(file, "-----------------------------------------------------FIM SIMULAÇÃO #%s------------------------------------------\n\n", timestamp);
    
    fclose(file);
    printf("Análise de constantes salva em: /mnt/dados/BJ_Binario/Resultados/analise_constantes.txt\n");
}

void concatenate_and_cleanup_logs(int num_sims, const char* output_suffix) {
    // Definir nome do arquivo final
    char final_filepath[512];
    if (output_suffix && strlen(output_suffix) > 0) {
        snprintf(final_filepath, sizeof(final_filepath), "%s/log_%s.csv", OUT_DIR, output_suffix);
    } else {
        snprintf(final_filepath, sizeof(final_filepath), "%s/log_sim.csv", OUT_DIR);
    }
    
    // Abrir arquivo final para escrita
    FILE *final_file = fopen(final_filepath, "w");
    if (!final_file) {
        perror("fopen final file");
        return;
    }
    
    // Escrever header uma vez
    fprintf(final_file, "Inicial,Upcard,Acoes,Final,Valor,DealerFinal,Resultado,Aposta,PNL,Double,Split,BJ_Jogador,BJ_Dealer\n");
    
    // Concatenar todos os arquivos individuais
    for (int i = 0; i < num_sims; ++i) {
        char individual_filepath[512];
        if (output_suffix && strlen(output_suffix) > 0) {
            snprintf(individual_filepath, sizeof(individual_filepath), "%s/log_%s_%d.csv", OUT_DIR, output_suffix, i);
        } else {
            snprintf(individual_filepath, sizeof(individual_filepath), "%s/log_sim_%d.csv", OUT_DIR, i);
        }
        
        FILE *individual_file = fopen(individual_filepath, "r");
        if (individual_file) {
            char line[1024];
            int line_count = 0;
            
            // Ler arquivo linha por linha
            while (fgets(line, sizeof(line), individual_file)) {
                line_count++;
                // Pular o header (primeira linha)
                if (line_count > 1) {
                    fputs(line, final_file);
                }
            }
            
            fclose(individual_file);
            
            // Deletar arquivo individual
            if (remove(individual_filepath) != 0) {
                perror("remove individual file");
            }
        }
    }
    
    fclose(final_file);
}

void process_insurance_data(int num_sims, const char* output_suffix) {
    printf("Processando dados de análise de insurance...\n");
    
    // Criar diretório se não existir
    struct stat st = {0};
    if (stat(OUT_DIR, &st) == -1) {
        mkdir(OUT_DIR, 0700);
    }
    
    // Definir bins para porcentagem de Áses (0% a 20% em bins de 0.1%)
    const int NUM_INSURANCE_BINS = 201; // (20% - 0%) / 0.1% = 200 bins + 1
    const double INSURANCE_BIN_WIDTH = 0.001; // 0.1%
    const double MIN_PERCENTAGE = 0.00; // 0%
    const double MAX_PERCENTAGE = 0.20; // 20%
    
    typedef struct {
        double percentage_min;
        double percentage_max;
        int total_ace_upcards;
        int dealer_blackjacks;
        double blackjack_frequency;
    } InsuranceBin;
    
    InsuranceBin bins[NUM_INSURANCE_BINS];
    
    // Inicializar bins
    for (int i = 0; i < NUM_INSURANCE_BINS; i++) {
        bins[i].percentage_min = MIN_PERCENTAGE + i * INSURANCE_BIN_WIDTH;
        bins[i].percentage_max = MIN_PERCENTAGE + (i + 1) * INSURANCE_BIN_WIDTH;
        bins[i].total_ace_upcards = 0;
        bins[i].dealer_blackjacks = 0;
        bins[i].blackjack_frequency = 0.0;
    }
    
    // Calcular número de lotes
    int num_batches = (num_sims + 19999) / 20000; // Arredondar para cima
    
    printf("Processando %d lotes de insurance...\n", num_batches);
    
    // Processar cada lote
    for (int batch = 0; batch < num_batches; batch++) {
        char temp_filename[512];
        snprintf(temp_filename, sizeof(temp_filename), "%s%d%s", INSURANCE_TEMP_FILE_PREFIX, batch, BINARY_SUFFIX);
        
        DEBUG_IO("Processando arquivo insurance: %s", temp_filename);
        
        // Verificar integridade do arquivo
        if (!verify_file_integrity(temp_filename, sizeof(InsuranceBinaryRecord))) {
            DEBUG_IO("Arquivo %s falhou na verificação de integridade, pulando", temp_filename);
            continue;
        }
        
        FILE* temp_file = fopen(temp_filename, "rb");
        if (!temp_file) {
            DEBUG_IO("Não foi possível abrir arquivo: %s", temp_filename);
            continue;
        }
        
        int records_processed = 0;
        int valid_records = 0;
        InsuranceBinaryRecord record;
        
        while (fread(&record, sizeof(record), 1, temp_file) == 1) {
            records_processed++;
            
            // Validar usando a função robusta
            if (!validate_insurance_record(&record)) {
                DEBUG_STATS("Registro insurance inválido no lote %d, posição %d", batch, records_processed);
                continue;
            }
            
            valid_records++;
            
            // Calcular bin baseado na porcentagem de Áses
            double percentage = (double)record.aces_percentage;
            int bin_idx = (int)((percentage - MIN_PERCENTAGE) / INSURANCE_BIN_WIDTH);
            
            if (bin_idx >= 0 && bin_idx < NUM_INSURANCE_BINS) {
                bins[bin_idx].total_ace_upcards++;
                bins[bin_idx].dealer_blackjacks += record.dealer_blackjack;
            }
        }
        
        fclose(temp_file);
        unlink(temp_filename); // Remover arquivo temporário
        
        DEBUG_STATS("Lote %d de insurance processado: %d registros lidos, %d válidos", 
                   batch, records_processed, valid_records);
    }
    
    // Calcular frequências
    for (int i = 0; i < NUM_INSURANCE_BINS; i++) {
        if (bins[i].total_ace_upcards > 0) {
            bins[i].blackjack_frequency = (double)bins[i].dealer_blackjacks / bins[i].total_ace_upcards;
        }
    }
    
    // Gerar CSV final
    char csv_filename[512];
    if (output_suffix) {
        snprintf(csv_filename, sizeof(csv_filename), "%s/insurance_analysis_%s.csv", OUT_DIR, output_suffix);
    } else {
        snprintf(csv_filename, sizeof(csv_filename), "%s/insurance_analysis.csv", OUT_DIR);
    }
    
    FILE* csv_file = fopen(csv_filename, "w");
    if (!csv_file) {
        perror("fopen insurance csv");
        return;
    }
    
    // Cabeçalho
    fprintf(csv_file, "Percentage_Min,Percentage_Max,Total_Ace_Upcards,Dealer_Blackjacks,Blackjack_Frequency\n");
    
    // Dados
    for (int i = 0; i < NUM_INSURANCE_BINS; i++) {
        fprintf(csv_file, "%.3f,%.3f,%d,%d,%.6f\n",
                bins[i].percentage_min * 100.0, // Converter para porcentagem
                bins[i].percentage_max * 100.0, // Converter para porcentagem
                bins[i].total_ace_upcards,
                bins[i].dealer_blackjacks,
                bins[i].blackjack_frequency);
    }
    
    fclose(csv_file);
    
    printf("Análise de insurance salva em: %s\n", csv_filename);
}

int main(int argc, char* argv[]) {
    int log_level = 0;
    int num_sims = NUM_SIMS;
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN); // Número de CPUs
    char* output_suffix = NULL;
    bool dealer_analysis = false; // Análise de dealer desativada por padrão
    bool freq_analysis_26 = false; // Análise frequência upcards 2-6
    bool freq_analysis_70 = false; // Análise frequência upcards 7-10
    bool freq_analysis_A = false;  // Análise frequência upcard A
    bool split_analysis = false;   // Análise de resultados de splits
    bool ev_realtime_enabled = false; // EV em tempo real desativado por padrão
    bool insurance_analysis = false; // Análise de insurance desativada por padrão
    
    // Processar argumentos da linha de comando
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-debug") == 0) {
            debug_enabled = true;
            DEBUG_PRINT("Debug ativado via argumento de linha de comando");
        } else if (strcmp(argv[i], "-hist26") == 0) {
            freq_analysis_26 = true;
            DEBUG_PRINT("Análise de frequência 2-6 ativada");
        } else if (strcmp(argv[i], "-hist70") == 0) {
            freq_analysis_70 = true;
            DEBUG_PRINT("Análise de frequência 7-10 ativada");
        } else if (strcmp(argv[i], "-histA") == 0) {
            freq_analysis_A = true;
            DEBUG_PRINT("Análise de frequência A ativada");
        } else if (strcmp(argv[i], "-split") == 0) {
            split_analysis = true;
            DEBUG_PRINT("Análise de splits ativada");
        } else if (strcmp(argv[i], "-ev") == 0) {
            ev_realtime_enabled = true;
            DEBUG_PRINT("EV em tempo real ativado");
        } else if (strcmp(argv[i], "-ins") == 0) {
            insurance_analysis = true;
            DEBUG_PRINT("Análise de insurance ativada");
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            log_level = atoi(argv[++i]);
            if (log_level < 0) {
                fprintf(stderr, "Erro: Nível de log deve ser >= 0\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            num_sims = atoi(argv[++i]);
            if (num_sims <= 0) {
                fprintf(stderr, "Erro: Número de simulações deve ser > 0\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            num_threads = atoi(argv[++i]);
            if (num_threads <= 0) {
                fprintf(stderr, "Erro: Número de threads deve ser > 0\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_suffix = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Opção inválida: %s. Use -h para ajuda.\n", argv[i]);
            return 1;
        }
    }
    
    // Configurar variáveis globais
    total_sims = num_sims;
    completed_sims = 0;
    
    // Inicializar variável global de unidades totais
    unidades_total_global = 0.0;
    
    // Contador global de linhas de log (thread-safe)
    atomic_int global_log_count = 0;
    
    // Mutex para proteger escritas no arquivo de dealer
    pthread_mutex_t dealer_mutex;
    if (dealer_analysis) {
        if (pthread_mutex_init(&dealer_mutex, NULL) != 0) {
            fprintf(stderr, "Erro ao inicializar mutex\n");
            return 1;
        }
    }
    
    // Mutex para proteger escritas nos arquivos de frequência
    pthread_mutex_t freq_mutex;
    bool any_freq_analysis = freq_analysis_26 || freq_analysis_70 || freq_analysis_A;
    if (any_freq_analysis) {
        if (pthread_mutex_init(&freq_mutex, NULL) != 0) {
            fprintf(stderr, "Erro ao inicializar mutex de frequência\n");
            return 1;
        }
    }
    

    
    // Mutex para proteger escritas nos arquivos de split
    pthread_mutex_t split_mutex;
    if (split_analysis) {
        if (pthread_mutex_init(&split_mutex, NULL) != 0) {
            fprintf(stderr, "Erro ao inicializar mutex de split\n");
            return 1;
        }
    }
    
    // Mutex para proteger escritas nos arquivos de insurance
    pthread_mutex_t insurance_mutex;
    if (insurance_analysis) {
        if (pthread_mutex_init(&insurance_mutex, NULL) != 0) {
            fprintf(stderr, "Erro ao inicializar mutex de insurance\n");
            return 1;
        }
    }
    
    // Mostrar configuração
    printf("Simulador de Blackjack - Configuração:\n");
    printf("  Simulações: %d\n", num_sims);
    printf("  Shoes por simulação: %d\n", NUM_SHOES);
    printf("  Threads: %d\n", num_threads);
    printf("  Estratégia: %s\n", ev_realtime_enabled ? "EV em tempo real" : "Estratégia básica");
    printf("  Linhas de log total: %d\n", log_level);
    printf("  Debug: %s\n", debug_enabled ? "ATIVADO" : "DESATIVADO");
    printf("  Análise frequência 2-6: %s\n", freq_analysis_26 ? "ATIVADA" : "DESATIVADA");
    printf("  Análise frequência 7-10: %s\n", freq_analysis_70 ? "ATIVADA" : "DESATIVADA");
    printf("  Análise frequência A: %s\n", freq_analysis_A ? "ATIVADA" : "DESATIVADA");
    printf("  Análise de splits: %s\n", split_analysis ? "ATIVADA" : "DESATIVADA");
    printf("  Análise de insurance: %s\n", insurance_analysis ? "ATIVADA" : "DESATIVADA");
    if (output_suffix) {
        printf("  Sufixo de saída: %s\n", output_suffix);
    }
    printf("  Total de jogos: %lld\n", (long long)num_sims * NUM_SHOES);
    printf("\n");
    
    DEBUG_PRINT("Configuração de debug ativada");
    DEBUG_STATS("Batch sizes: Dealer=%d, Freq=%d, Split=%d", DEALER_BATCH_SIZE, FREQ_BATCH_SIZE, SPLIT_BATCH_SIZE);
    DEBUG_STATS("Buffers: Freq=%d (threshold=%d), Dealer=%d (threshold=%d)", 
               FREQ_BUFFER_SIZE, FREQ_BUFFER_THRESHOLD, DEALER_BUFFER_SIZE, DEALER_BUFFER_THRESHOLD);
    
    // Sistema de estratégia básica super-otimizada
    printf("Sistema usando estratégia básica otimizada com tabelas inline.\n");
    
    // INICIALIZAR SISTEMA DE EV EM TEMPO REAL
    printf("🚀 Inicializando sistema de EV em tempo real...\n");
    init_realtime_strategy_system();
    printf("\n");
    
    // Iniciar cronômetro
    gettimeofday(&start_time, NULL);
    
    // Criar threads
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData* thread_data = aligned_alloc(64, num_threads * sizeof(ThreadData));
    
    if (!threads || !thread_data) {
        fprintf(stderr, "Erro ao alocar memória para threads\n");
        return 1;
    }
    
    int sims_per_thread = num_sims / num_threads;
    int remaining_sims = num_sims % num_threads;
    
    // Otimizar distribuição para melhor balanceamento
    int sim_offset = 0;
    for (int i = 0; i < num_threads; ++i) {
        thread_data[i].log_level = log_level;
        thread_data[i].sim_start = sim_offset;
        
        // Distribuir simulações restantes de forma mais equilibrada
        int extra_sims = (i < remaining_sims) ? 1 : 0;
        thread_data[i].sim_end = sim_offset + sims_per_thread + extra_sims;
        
        thread_data[i].thread_id = i;
        thread_data[i].output_suffix = output_suffix;
        thread_data[i].global_log_count = &global_log_count;
        thread_data[i].dealer_analysis = dealer_analysis;
        thread_data[i].freq_analysis_26 = freq_analysis_26;
        thread_data[i].freq_analysis_70 = freq_analysis_70;
        thread_data[i].freq_analysis_A = freq_analysis_A;

        thread_data[i].split_analysis = split_analysis;
        thread_data[i].ev_realtime_enabled = ev_realtime_enabled;
        thread_data[i].dealer_mutex = dealer_analysis ? &dealer_mutex : NULL;
        thread_data[i].freq_mutex = any_freq_analysis ? &freq_mutex : NULL;

        thread_data[i].split_mutex = split_analysis ? &split_mutex : NULL;
        thread_data[i].insurance_analysis = insurance_analysis;
        thread_data[i].insurance_mutex = insurance_analysis ? &insurance_mutex : NULL;
        
        sim_offset = thread_data[i].sim_end;
        
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_data[i]) != 0) {
            fprintf(stderr, "Erro ao criar thread %d\n", i);
            return 1;
        }
    }
    
    // Aguardar todas as threads terminarem
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }
    
    // Mostrar progresso final
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    double total_time = (end_time.tv_sec - start_time.tv_sec) + 
                       (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    show_progress(num_sims, num_sims, total_time);
    printf("\n\n");
    
    // Concatenar logs e limpar arquivos individuais se necessário
    if (log_level > 0) {
        printf("Concatenando arquivos de log...\n");
        concatenate_and_cleanup_logs(num_sims, output_suffix);
    }
    
    // Processar dados de frequência se solicitado
    if (freq_analysis_26 || freq_analysis_70 || freq_analysis_A) {
        printf("Processando dados de análise de frequência...\n");
        process_frequency_data(num_sims, output_suffix, freq_analysis_26, freq_analysis_70, freq_analysis_A);
    }
    
    // Processar dados de split se solicitado
    if (split_analysis) {
        printf("Processando dados de análise de splits...\n");
        process_split_data(num_sims, output_suffix);
    }
    
    // Processar dados de insurance se solicitado
    if (insurance_analysis) {
        printf("Processando dados de análise de insurance...\n");
        process_insurance_data(num_sims, output_suffix);
    }
    
    // Análise de bust obsoleta removida
    
    // Estatísticas finais
    printf("Simulação concluída!\n");
    printf("  Tempo total: %.2f segundos\n", total_time);
    printf("  Taxa: %.1f simulações/segundo\n", num_sims / total_time);
    printf("  Jogos processados: %lld\n", (long long)num_sims * NUM_SHOES);
    printf("  Taxa de jogos: %.0f jogos/segundo\n", (num_sims * NUM_SHOES) / total_time);
    
    // Calcular e mostrar média de unidades por shoe
    double unidades_totais = unidades_total_global;
    long long total_shoes = (long long)num_sims * NUM_SHOES;
    double unidade_media_por_shoe = unidades_totais / total_shoes;
    printf("  Média de unidades por shoe: %.4f\n", unidade_media_por_shoe);
    
    if (log_level > 0) {
        int final_log_count = atomic_load(&global_log_count);
        if (output_suffix) {
            printf("  Log final salvo: log_%s.csv em %s/ (%d linhas)\n", output_suffix, OUT_DIR, final_log_count);
        } else {
            printf("  Log final salvo: log_sim.csv em %s/ (%d linhas)\n", OUT_DIR, final_log_count);
        }
    }
    
    // Salvar análise de constantes
    salvar_analise_constantes(unidade_media_por_shoe);
    
    // Destruir mutex se foi inicializado
    if (dealer_analysis) {
        pthread_mutex_destroy(&dealer_mutex);
    }
    if (any_freq_analysis) {
        pthread_mutex_destroy(&freq_mutex);
    }

    if (split_analysis) {
        pthread_mutex_destroy(&split_mutex);
    }

    if (insurance_analysis) {
        pthread_mutex_destroy(&insurance_mutex);
    }
    
    // FINALIZAR SISTEMA DE EV EM TEMPO REAL
    cleanup_realtime_strategy_system();
    
    // Liberar memória
    free(threads);
    free(thread_data);
    
    return 0;
} 
