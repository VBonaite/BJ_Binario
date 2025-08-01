#include "simulacao.h"
#include "baralho.h"
#include "rng.h"
#include "constantes.h"
#include "jogo.h"
#include "saidas.h"
#include "tabela_estrategia.h"
#include "structures.h"  // Usar estruturas centralizadas
#include "shoe_counter.h"  // Para ShoeCounter
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdatomic.h>

// Declarações das funções binárias do main.c
void write_dealer_binary(FILE* file, double true_count, int ace_upcard, int dealer_bj);

void write_split_binary(FILE* file, double true_count, int lose_lose, int win_win, int push_push, int lose_win, 
                       int lose_push, int win_lose, int win_push, int push_lose, int push_win, int cards_used);
void write_freq_binary(FILE* file, double true_count);

// Sufixo para arquivos binários
#define BINARY_SUFFIX ".bin"

// Estrutura para buffer local de dados de frequência por thread
typedef struct {
    double true_count;
    int upcard;
    int final_result;
} FreqBufferEntry;

// Estrutura para buffer de dados de dealer
typedef struct {
    double true_count;
    int ace_upcard;
    int dealer_bj;
} DealerBufferEntry;

static void adicionar_carta(uint64_t *mao, Carta c) {
    *mao += c; // incrementa o contador de 3 bits para o rank
}

static void atualizar_counts(double *running_count, double *true_count, Carta c, size_t cartas_restantes) {
    *running_count += WONG_HALVES[carta_para_rank_idx(c)];
    double decks_restantes = (double)cartas_restantes / 52.0;
    if (decks_restantes < 1.0) decks_restantes = 1.0;
    *true_count = *running_count / decks_restantes;
    
    DEBUG_STATS("Counts atualizados: carta=%d, RC=%.3f, TC=%.3f, cartas_restantes=%zu", 
               carta_para_rank_idx(c), *running_count, *true_count, cartas_restantes);
}

// Função identificar_split_10_tipo removida - não utilizada no sistema atual

void simulacao_completa(int log_level, int sim_id, const char* output_suffix, atomic_int* global_log_count, bool dealer_analysis, bool freq_analysis_26, bool freq_analysis_70, bool freq_analysis_A, bool split_analysis, bool ev_realtime_enabled, pthread_mutex_t* dealer_mutex, pthread_mutex_t* freq_mutex, pthread_mutex_t* split_mutex, bool insurance_analysis, pthread_mutex_t* insurance_mutex) {
    DEBUG_PRINT("Iniciando simulação %d", sim_id);
    
    FILE *log_file = NULL;
    FILE *dealer_file = NULL;
    
    // Inicializar logging apenas se log_level > 0
    if (log_level > 0) {
        // Criar diretório se não existir
        struct stat st = {0};
        if (stat(OUT_DIR, &st) == -1) {
            if (mkdir(OUT_DIR, 0755) != 0 && errno != EEXIST) {
                perror("mkdir");
                exit(EXIT_FAILURE);
            }
        }
        
        // Abrir arquivo de log com sufixo personalizado
        char filepath[512];
        if (output_suffix && strlen(output_suffix) > 0) {
            snprintf(filepath, sizeof(filepath), "%s/log_%s_%d.csv", OUT_DIR, output_suffix, sim_id);
        } else {
            snprintf(filepath, sizeof(filepath), "%s/log_sim_%d.csv", OUT_DIR, sim_id);
        }
        
        log_file = fopen(filepath, "w");
        if (!log_file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        // Escrever header
        fprintf(log_file, "Inicial,Upcard,Acoes,Final,Valor,DealerFinal,Resultado,Aposta,PNL,Double,Split,BJ_Jogador,BJ_Dealer\n");
        DEBUG_IO("Arquivo de log criado: %s", filepath);
    }
    
    // Inicializar arquivo de dealer se análise está ativada
    if (dealer_analysis) {
        // Calcular qual lote esta simulação pertence (CORRIGIDO: usar DEALER_BATCH_SIZE)
        int batch_id = sim_id / DEALER_BATCH_SIZE;  
        char dealer_filepath[512];
        snprintf(dealer_filepath, sizeof(dealer_filepath), "%s%d%s", DEALER_TEMP_FILE_PREFIX, batch_id, BINARY_SUFFIX);
        
        DEBUG_IO("Criando arquivo dealer para sim %d, batch %d: %s", sim_id, batch_id, dealer_filepath);
        
        dealer_file = fopen(dealer_filepath, "ab");  // Modo append binário para múltiplas simulações
        if (!dealer_file) {
            perror("fopen dealer_file");
            if (log_file) fclose(log_file);
            return;
        }
        // Arquivos binários não precisam de cabeçalho
    }
    
    // Inicializar arquivos de frequência se análise está ativada
    FILE* freq_total_files[11]; // Para contar total de cada upcard
    FILE* freq_result_files[11][7]; // [upcard][final_value] - para contar resultados específicos (0-6: 17,18,19,20,21,BJ,BUST)
    memset(freq_total_files, 0, sizeof(freq_total_files));
    memset(freq_result_files, 0, sizeof(freq_result_files));
    
    // Inicializar arquivos de split se análise está ativada
    FILE* split_files[10][10]; // [pair][upcard] - 10 pares x 10 upcards (removidos JJ, QQ, KK)
    memset(split_files, 0, sizeof(split_files));
    
    bool any_freq_analysis = freq_analysis_26 || freq_analysis_70 || freq_analysis_A;
    if (any_freq_analysis) {
        // Calcular qual lote esta simulação pertence (CORRIGIDO: usar FREQ_BATCH_SIZE)
        int batch_id = sim_id / FREQ_BATCH_SIZE;  
        
        DEBUG_PRINT("Configurando análise de frequência para sim %d, batch %d", sim_id, batch_id);
        
        // Determinar quais upcards precisam ser analisados
        int start_upcard = 11, end_upcard = 1; // Valores inválidos = nenhum upcard por padrão
        bool include_ace = false;
        
        if (freq_analysis_26 && freq_analysis_70) {
            // Ambos ativos: 2-6 e 7-10 = 2-10
            start_upcard = 2; end_upcard = 10;
            DEBUG_PRINT("Analisando upcards 2-10 (freq_analysis_26 + freq_analysis_70)");
        } else if (freq_analysis_26) {
            // Apenas 2-6
            start_upcard = 2; end_upcard = 6;
            DEBUG_PRINT("Analisando upcards 2-6");
        } else if (freq_analysis_70) {
            // Apenas 7-10
            start_upcard = 7; end_upcard = 10;
            DEBUG_PRINT("Analisando upcards 7-10");
        }
        
        if (freq_analysis_A) {
            include_ace = true;
            DEBUG_PRINT("Analisando upcard A");
        }
        
        // Criar arquivos de total para cada upcard
        for (int upcard = start_upcard; upcard <= end_upcard; upcard++) {
            char total_filepath[512];
            snprintf(total_filepath, sizeof(total_filepath), "temp_total_upcard_%d_batch_%d%s", upcard, batch_id, BINARY_SUFFIX);
            
            freq_total_files[upcard-2] = fopen(total_filepath, "ab");
            if (!freq_total_files[upcard-2]) {
                perror("fopen freq_total_file");
                continue;
            }
            DEBUG_IO("Arquivo freq total criado: %s", total_filepath);
            // Arquivos binários não precisam de cabeçalho
        }
        
        // Criar arquivos de resultado para cada combinação upcard/final
        for (int upcard = start_upcard; upcard <= end_upcard; upcard++) {
            for (int final_val = 0; final_val < 7; final_val++) {
                // Criar arquivo BJ apenas para upcards que podem ter blackjack (10 e A não se aplicam aqui pois start_upcard <= 10)
                if (final_val == 5 && upcard != 10) {  // BJ apenas para upcard 10
                    continue; // Pular criação de arquivo BJ para upcards 2-9
                }
                
                char result_filepath[512];
                const char* final_names[] = {"17", "18", "19", "20", "21", "BJ", "BUST"};
                snprintf(result_filepath, sizeof(result_filepath), "temp_result_upcard_%d_final_%s_batch_%d%s", upcard, final_names[final_val], batch_id, BINARY_SUFFIX);
                
                freq_result_files[upcard-2][final_val] = fopen(result_filepath, "ab");
                if (!freq_result_files[upcard-2][final_val]) {
                    perror("fopen freq_result_file");
                    continue;
                }
                DEBUG_IO("Arquivo freq result criado: %s", result_filepath);
                // Arquivos binários não precisam de cabeçalho
            }
        }
        
        // Criar arquivos para upcard A se necessário
        if (include_ace) {
            char total_filepath[512];
            snprintf(total_filepath, sizeof(total_filepath), "temp_total_upcard_A_batch_%d%s", batch_id, BINARY_SUFFIX);
            
            freq_total_files[9] = fopen(total_filepath, "ab"); // Índice 9 para Ás
            if (!freq_total_files[9]) {
                perror("fopen freq_total_file A");
            } else {
                DEBUG_IO("Arquivo freq total A criado: %s", total_filepath);
            }
            // Arquivos binários não precisam de cabeçalho
            
            for (int final_val = 0; final_val < 7; final_val++) {
                // Upcard A pode ter blackjack, então criamos todos os arquivos incluindo BJ e BUST
                char result_filepath[512];
                const char* final_names[] = {"17", "18", "19", "20", "21", "BJ", "BUST"};
                snprintf(result_filepath, sizeof(result_filepath), "temp_result_upcard_A_final_%s_batch_%d%s", final_names[final_val], batch_id, BINARY_SUFFIX);
                
                freq_result_files[9][final_val] = fopen(result_filepath, "ab");
                if (!freq_result_files[9][final_val]) {
                    perror("fopen freq_result_file A");
                    continue;
                }
                DEBUG_IO("Arquivo freq result A criado: %s", result_filepath);
                // Arquivos binários não precisam de cabeçalho
            }
        }
    }
    
    // Inicializar arquivos de split se análise está ativada
    if (split_analysis) {
        // Calcular qual lote esta simulação pertence (CORRIGIDO: usar SPLIT_BATCH_SIZE)
        int batch_id = sim_id / SPLIT_BATCH_SIZE;  
        
        DEBUG_PRINT("Configurando análise de split para sim %d, batch %d", sim_id, batch_id);
        
        // Pares a analisar (10 pares rank-descendente): AA, 1010, 99, 88, 77, 66, 55, 44, 33, 22
        // Removidos JJ, QQ, KK conforme solicitado
        const char* pairs[] = {"AA", "1010", "99", "88", "77", "66", "55", "44", "33", "22"};
        
        // Upcards do dealer (10 possibilidades): 2-10, A
        const char* upcards[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "A"};
        
        // Criar arquivos para cada combinação de par e upcard
        for (int p = 0; p < 10; p++) {
            for (int u = 0; u < 10; u++) {
                char split_filepath[512];
                snprintf(split_filepath, sizeof(split_filepath), "%s%d_%s_vs_%s%s", SPLIT_TEMP_FILE_PREFIX, batch_id, pairs[p], upcards[u], BINARY_SUFFIX);
                
                split_files[p][u] = fopen(split_filepath, "ab");
                if (!split_files[p][u]) {
                    perror("fopen split_file");
                    continue;
                }
                DEBUG_IO("Arquivo split criado: %s", split_filepath);
                // Arquivos binários não precisam de cabeçalho
            }
        }
    }
    
    rng_init();
    
    // Variáveis para controle do bankroll e estatísticas
    double bankroll = BANKROLL_INICIAL;
    int vitorias = 0;
    int maos_jogadas = 0;
    double pnl_shoe = 0.0;      // PNL acumulado do shoe atual
    double loss_shoe = 0.0;     // Unidades perdidas no shoe atual
    double unidade_atual = UNIDADE_INICIAL;
    
    int shoes_jogados = 0;
    double running_count = 0.0;
    double true_count = 0.0;
    
    // Memory pool para evitar malloc/free frequentes
    #define MAX_MAOS_PER_ROUND 100  // Máximo de mãos por rodada
    static __thread uint64_t maos_bits_pool[MAX_MAOS_PER_ROUND];
    static __thread bool pool_initialized = false;
    if (!pool_initialized) {
        memset(maos_bits_pool, 0, sizeof(maos_bits_pool));
        pool_initialized = true;
    }

    // Buffers locais para análise (por thread) - todos os tipos de análise
    static __thread FreqBufferEntry freq_buffer[FREQ_BUFFER_SIZE];
    static __thread int freq_buffer_count = 0;
    static __thread DealerBufferEntry dealer_buffer[DEALER_BUFFER_SIZE];
    static __thread int dealer_buffer_count = 0;

    // Função otimizada para flush do buffer de frequência - escreve tudo em lotes grandes
    void flush_freq_buffer() {
        if (freq_buffer_count == 0) return;
        
        DEBUG_MUTEX("Tentando obter mutex de frequência para flush");
        
        // Obter lock apenas uma vez para todo o buffer
        if (freq_mutex) {
            pthread_mutex_lock(freq_mutex);
            DEBUG_MUTEX("Mutex de frequência obtido, flushing %d registros", freq_buffer_count);
            
            // Agrupar entries por arquivo para reduzir operações de I/O
            int batch_id = sim_id / FREQ_BATCH_SIZE;  // CORRIGIDO: usar FREQ_BATCH_SIZE
            
            // Arrays para agrupar dados por arquivo
            FreqBinaryRecord* records_by_file[11][7]; // [upcard][final_result]
            int counts_by_file[11][7];
            memset(counts_by_file, 0, sizeof(counts_by_file));
            
            // Alocar buffers para cada arquivo
            for (int u = 0; u < 11; u++) {
                for (int f = 0; f < 7; f++) {
                    records_by_file[u][f] = malloc(freq_buffer_count * sizeof(FreqBinaryRecord));
                    if (!records_by_file[u][f]) {
                        DEBUG_IO("ERRO: Falha ao alocar buffer para upcard %d, final %d", u, f);
                    }
                }
            }
            
            // Agrupar todos os entries por arquivo
            for (int i = 0; i < freq_buffer_count; i++) {
                FreqBufferEntry* entry = &freq_buffer[i];
                
                int upcard_idx = (entry->upcard == 11) ? 9 : entry->upcard - 2;
                if (upcard_idx >= 0 && upcard_idx < 11 && entry->final_result >= 0 && entry->final_result < 7) {
                    int count = counts_by_file[upcard_idx][entry->final_result];
                    if (records_by_file[upcard_idx][entry->final_result] && count < freq_buffer_count) {
                        records_by_file[upcard_idx][entry->final_result][count].true_count = (float)entry->true_count;
                        records_by_file[upcard_idx][entry->final_result][count].checksum = 
                            calculate_freq_checksum(&records_by_file[upcard_idx][entry->final_result][count]);
                        counts_by_file[upcard_idx][entry->final_result]++;
                    }
                }
            }
            
            // Escrever todos os dados agrupados em lotes grandes
            const char* final_names[] = {"17", "18", "19", "20", "21", "BJ", "BUST"};
            for (int u = 0; u < 11; u++) {
                for (int f = 0; f < 7; f++) {
                    if (counts_by_file[u][f] > 0) {
                        char result_filepath[512];
                        int upcard_value = (u == 9) ? 11 : u + 2; // Converter índice de volta para upcard
                        if (upcard_value == 11) {
                            snprintf(result_filepath, sizeof(result_filepath), 
                                    "temp_result_upcard_A_final_%s_batch_%d%s", 
                                    final_names[f], batch_id, BINARY_SUFFIX);
                        } else {
                            snprintf(result_filepath, sizeof(result_filepath), 
                                    "temp_result_upcard_%d_final_%s_batch_%d%s", 
                                    upcard_value, final_names[f], batch_id, BINARY_SUFFIX);
                        }
                        
                        FILE* result_file = fopen(result_filepath, "ab");
                        if (result_file) {
                            // Escrever tudo de uma vez
                            size_t written = fwrite(records_by_file[u][f], sizeof(FreqBinaryRecord), counts_by_file[u][f], result_file);
                            DEBUG_IO("Flush freq: escreveu %zu registros para %s", written, result_filepath);
                            fclose(result_file);
                        } else {
                            DEBUG_IO("ERRO: Não foi possível abrir arquivo para flush: %s", result_filepath);
                        }
                    }
                }
            }
            
            // Liberar buffers
            for (int u = 0; u < 11; u++) {
                for (int f = 0; f < 7; f++) {
                    free(records_by_file[u][f]);
                }
            }
            
            pthread_mutex_unlock(freq_mutex);
            DEBUG_MUTEX("Mutex de frequência liberado");
        }
        
        freq_buffer_count = 0;  // Reset buffer
    }

    // Função para flush do buffer de dealer - otimizada
    void flush_dealer_buffer() {
        if (dealer_buffer_count == 0) return;
        
        DEBUG_MUTEX("Flushing dealer buffer com %d registros", dealer_buffer_count);
        
        if (dealer_mutex && dealer_file) {
            pthread_mutex_lock(dealer_mutex);
            
            for (int i = 0; i < dealer_buffer_count; i++) {
                DealerBufferEntry* entry = &dealer_buffer[i];
                write_dealer_binary(dealer_file, entry->true_count, entry->ace_upcard, entry->dealer_bj);
            }
            
            pthread_mutex_unlock(dealer_mutex);
        }
        
        dealer_buffer_count = 0;
    }

    // Variável para controlar coleta duplicada de dados de frequência
    bool freq_data_collected_this_round = false;

    DEBUG_PRINT("Iniciando loop principal de shoes para simulação %d", sim_id);

    while (shoes_jogados < NUM_SHOES) {
        DEBUG_PRINT("Iniciando shoe %d de %d", shoes_jogados + 1, NUM_SHOES);
        
        Shoe shoe;
        baralho_criar(&shoe);
        baralho_embaralhar(&shoe);
        
        // Inicializar ShoeCounter para este shoe
        ShoeCounter shoe_counter;
        shoe_counter_init(&shoe_counter, 8);  // 8 decks - já inicializa corretamente!
        
        DEBUG_STATS("ShoeCounter inicializado: %d cartas totais", shoe_counter.total_cards);
        
        // Jogar até atingir a penetração
        size_t limite_penetracao = (size_t)(shoe.total * PENETRACAO);
        DEBUG_STATS("Shoe criado: %zu cartas, limite penetração: %zu", shoe.total, limite_penetracao);
        
        while (shoe.topo <= limite_penetracao) {
            // Reset da flag para nova rodada
            freq_data_collected_this_round = false;
            
            // Verificar se já atingiu o limite global de logs
            if (log_level > 0 && atomic_load(global_log_count) >= log_level) {
                DEBUG_PRINT("Limite de logs atingido, encerrando simulação");
                // Interromper loop se limite atingido
                goto finish_simulation;
            }
            
            // Calcular mãos contabilizadas baseado no true count atual
            int maos_contabilizadas = calcular_maos_contabilizadas(true_count);
            int total_maos = NUM_JOGADORES + maos_contabilizadas;
            
            DEBUG_STATS("Rodada: TC=%.3f, mãos_contab=%d, total_maos=%d", true_count, maos_contabilizadas, total_maos);
            
            // Atualizar unidade baseada no bankroll
            unidade_atual = calcular_unidade(bankroll);
            
            // Calcular aposta usando o sistema de progressão
            size_t cartas_restantes = shoe.total - shoe.topo;
            int bet = definir_aposta(cartas_restantes, vitorias, true_count, maos_jogadas, loss_shoe, unidade_atual);
            
            DEBUG_STATS("Aposta calculada: %d unidades (%.2f), bankroll=%.2f", bet, unidade_atual, bankroll);
            
            // Uma rodada completa - usar memory pool para evitar malloc/free
            uint64_t *maos_bits;
            if (total_maos <= MAX_MAOS_PER_ROUND) {
                // Usar pool thread-local para casos pequenos
                maos_bits = maos_bits_pool;
                memset(maos_bits, 0, total_maos * sizeof(uint64_t));
            } else {
                // Usar calloc para casos grandes
                maos_bits = (uint64_t*)calloc(total_maos, sizeof(uint64_t));
                if (!maos_bits) {
                    DEBUG_IO("ERRO: Falha ao alocar memória para %d mãos", total_maos);
                }
            }
            
            uint64_t dealer_mao = 0;
            Carta dealer_hole_card = 0; // Guardar para contabilizar depois

            DEBUG_PRINT("Distribuindo cartas - primeira rodada");
            
            // Primeira rodada de distribuição - primeiro jogadores normais, depois mãos contabilizadas
            // Distribuir para jogadores normais (índices 0 a NUM_JOGADORES-1)
            for (int i = 0; i < NUM_JOGADORES; ++i) {
                Carta c = baralho_comprar(&shoe);
                adicionar_carta(&maos_bits[i], c);
                atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
                
                // Atualizar ShoeCounter
                int rank_idx = carta_para_rank_idx(c);
                if (rank_idx >= 0 && rank_idx < NUM_RANKS && shoe_counter.counts[rank_idx] > 0) {
                    shoe_counter.counts[rank_idx]--;
                    shoe_counter.total_cards--;
                }
            }
            // Distribuir para mãos contabilizadas (índices NUM_JOGADORES a total_maos-1)
            for (int i = NUM_JOGADORES; i < total_maos; ++i) {
                Carta c = baralho_comprar(&shoe);
                adicionar_carta(&maos_bits[i], c);
                atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
                
                // Atualizar ShoeCounter
                int rank_idx = carta_para_rank_idx(c);
                if (rank_idx >= 0 && rank_idx < NUM_RANKS && shoe_counter.counts[rank_idx] > 0) {
                    shoe_counter.counts[rank_idx]--;
                    shoe_counter.total_cards--;
                }
            }
            
            // Dealer recebe upcard
            Carta c = baralho_comprar(&shoe);
            adicionar_carta(&dealer_mao, c);
            Carta dealer_upcard = c;
            atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
            
            // Atualizar ShoeCounter com dealer upcard
            int dealer_rank_idx = carta_para_rank_idx(c);
            if (dealer_rank_idx >= 0 && dealer_rank_idx < NUM_RANKS && shoe_counter.counts[dealer_rank_idx] > 0) {
                shoe_counter.counts[dealer_rank_idx]--;
                shoe_counter.total_cards--;
            }

            DEBUG_PRINT("Distribuindo cartas - segunda rodada");
            
            // Segunda rodada de distribuição - primeiro jogadores normais, depois mãos contabilizadas
            // Distribuir para jogadores normais (índices 0 a NUM_JOGADORES-1)
            for (int i = 0; i < NUM_JOGADORES; ++i) {
                c = baralho_comprar(&shoe);
                adicionar_carta(&maos_bits[i], c);
                atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
                
                // Atualizar ShoeCounter
                int rank_idx = carta_para_rank_idx(c);
                if (rank_idx >= 0 && rank_idx < NUM_RANKS && shoe_counter.counts[rank_idx] > 0) {
                    shoe_counter.counts[rank_idx]--;
                    shoe_counter.total_cards--;
                }
            }
            // Distribuir para mãos contabilizadas (índices NUM_JOGADORES a total_maos-1)
            for (int i = NUM_JOGADORES; i < total_maos; ++i) {
                c = baralho_comprar(&shoe);
                adicionar_carta(&maos_bits[i], c);
                atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
                
                // Atualizar ShoeCounter
                int rank_idx = carta_para_rank_idx(c);
                if (rank_idx >= 0 && rank_idx < NUM_RANKS && shoe_counter.counts[rank_idx] > 0) {
                    shoe_counter.counts[rank_idx]--;
                    shoe_counter.total_cards--;
                }
            }
            
            // *** PONTO CRÍTICO: CAPTURAR TRUE COUNT PARA ESTATÍSTICAS ***
            // Este é o momento exato onde o true count deve ser capturado:
            // - Todos jogadores receberam 2 cartas (TC atualizado)
            // - Dealer recebeu upcard (TC atualizado)
            // - Dealer ainda NÃO recebeu hole card
            double true_count_for_stats = true_count;
            
            DEBUG_STATS("TC capturado para estatísticas: %.3f (antes do hole card)", true_count_for_stats);
            
            // Dealer recebe hole card (NÃO CONTABILIZAR AINDA)
            c = baralho_comprar(&shoe);
            adicionar_carta(&dealer_mao, c);
            dealer_hole_card = c;
            
            // Calcular rank do upcard do dealer - otimizado
            int dealer_up_rank;
#if defined(__GNUC__)
            int bit_pos = __builtin_ctzll(dealer_upcard) / 3;
            // Lookup table para conversão rápida
            static const int rank_table[13] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10, 11};
            dealer_up_rank = rank_table[bit_pos];
#else
            dealer_up_rank = 10;
#endif
            
            DEBUG_STATS("Dealer upcard: rank=%d, bits=%llu", dealer_up_rank, (unsigned long long)dealer_upcard);
            
            Mao dealer_info;
            avaliar_mao(dealer_mao, &dealer_info);
            
            char dealer_final_str[32];
            dealer_final_str[0] = '\0'; // Inicializar para evitar lixo
            
            // Calcular upcard_char apenas se necessário para logging
            char upcard_char = '\0';
            if (log_level > 0) {
                upcard_char = carta_para_char(dealer_upcard);
            }

            // Calcular insurance se aplicável
            double insurance_bet = 0.0;
            bool made_insurance = false;
            int maos_contabilizadas_count = 0;
            
            // Contar mãos contabilizadas
            for (int pj = NUM_JOGADORES; pj < total_maos; ++pj) {
                maos_contabilizadas_count++;
            }
            
            // Calcular porcentagem de Áses no baralho
            int aces_count = shoe_counter.counts[12]; // Ás = índice 12
            double aces_percentage = (double)aces_count / shoe_counter.total_cards;
            
            if (dealer_up_rank == 11 && true_count >= MIN_COUNT_INS && maos_contabilizadas_count > 0 && aces_percentage >= A_perc) {
                insurance_bet = (bet / 2.0) * maos_contabilizadas_count;
                made_insurance = true;
                DEBUG_STATS("Insurance feito: %.2f unidades (A%%=%.2f%%)", insurance_bet, aces_percentage * 100.0);
            }

            // Verificar se upcard é Ás
            if (dealer_up_rank == 11) {
                DEBUG_PRINT("Dealer tem upcard Ás - verificando blackjack");
                
                // Coletar dados de insurance se análise está ativada
                if (insurance_analysis) {
                    // Calcular qual lote esta simulação pertence
                    int batch_id = sim_id / 20000; // Usar batch size de 20k como outros
                    char insurance_filepath[512];
                    snprintf(insurance_filepath, sizeof(insurance_filepath), "%s%d%s", INSURANCE_TEMP_FILE_PREFIX, batch_id, BINARY_SUFFIX);
                    
                    FILE* insurance_file = fopen(insurance_filepath, "ab");
                    if (insurance_file) {
                        InsuranceBinaryRecord record;
                        record.aces_percentage = (float)aces_percentage;
                        record.dealer_blackjack = dealer_info.blackjack ? 1 : 0;
                        record.checksum = calculate_insurance_checksum(&record);
                        
                        // Proteger escrita com mutex
                        if (insurance_mutex) {
                            pthread_mutex_lock(insurance_mutex);
                        }
                        
                        fwrite(&record, sizeof(record), 1, insurance_file);
                        
                        if (insurance_mutex) {
                            pthread_mutex_unlock(insurance_mutex);
                        }
                        
                        fclose(insurance_file);
                        
                        DEBUG_STATS("Dados insurance salvos: A%%=%.2f%%, BJ=%d", 
                                   aces_percentage * 100.0, dealer_info.blackjack ? 1 : 0);
                    }
                }
                
                // Coletar dados do dealer se análise está ativada - usando buffer
                // IMPORTANTE: Usar true_count NO MOMENTO DA DECISÃO DE INSURANCE
                // (sem conhecer o hole card do dealer)
                if (dealer_analysis && dealer_buffer_count < DEALER_BUFFER_THRESHOLD) {
                    dealer_buffer[dealer_buffer_count].true_count = true_count;
                    dealer_buffer[dealer_buffer_count].ace_upcard = 1;
                    dealer_buffer[dealer_buffer_count].dealer_bj = dealer_info.blackjack ? 1 : 0;
                    dealer_buffer_count++;
                    
                    DEBUG_STATS("Dados dealer adicionados ao buffer: TC=%.3f, BJ=%d", 
                               true_count, dealer_info.blackjack ? 1 : 0);
                    
                    // Flush buffer quando quase cheio
                    if (dealer_buffer_count >= DEALER_BUFFER_THRESHOLD) {
                        flush_dealer_buffer();
                    }
                }
                
                // Upcard é Ás - verificar blackjack primeiro
                if (dealer_info.blackjack) {
                    DEBUG_PRINT("Dealer tem BLACKJACK");
                    
                    // Dealer tem BJ - contabilizar hole card
                    atualizar_counts(&running_count, &true_count, dealer_hole_card, shoe.total - shoe.topo);
                    
                    // Atualizar ShoeCounter com a hole card revelada
                    int hole_rank_idx = carta_para_rank_idx(dealer_hole_card);
                    if (hole_rank_idx >= 0 && hole_rank_idx < NUM_RANKS && shoe_counter.counts[hole_rank_idx] > 0) {
                        shoe_counter.counts[hole_rank_idx]--;
                        shoe_counter.total_cards--;
                    }
                    
                    // Aplicar ganho do insurance se foi feito
                    if (made_insurance) {
                        bankroll += insurance_bet * 2.0; // Insurance paga 2:1
                        DEBUG_STATS("Insurance ganho: +%.2f", insurance_bet * 2.0);
                    }
                    
                    // Preencher string do dealer ANTES de usar (apenas se necessário)
                    if (log_level > 0) {
                        mao_para_string(dealer_info.bits, dealer_final_str);
                    }
                    
                    // Variável para acumular PNL total da rodada (mãos contabilizadas + insurance)
                    double pnl_rodada_total = 0.0;
                    
                    // Incluir PNL do insurance se foi feito
                    if (made_insurance) {
                        pnl_rodada_total += insurance_bet * 2.0; // Ganho do insurance
                    }
                    
                    // Jogadores não jogam - apenas processar resultados
                    for (int pj = 0; pj < total_maos; ++pj) {
                        Mao mao_jogador;
                        avaliar_mao(maos_bits[pj], &mao_jogador);
                        mao_jogador.aposta = bet;
                        mao_jogador.finalizada = true; // Não jogam
                        
                        // Marcar mãos contabilizadas
                        if (pj >= NUM_JOGADORES) {
                            mao_jogador.contabilizada = true;
                        }
                        
                        verificar_mao(&mao_jogador, &dealer_info);
                        calcular_pnl(&mao_jogador);
                        
                        // Atualizar estatísticas apenas para mãos contabilizadas
                        if (mao_jogador.contabilizada) {
                            bankroll += mao_jogador.pnl;
                            pnl_shoe += mao_jogador.pnl;
                            pnl_rodada_total += mao_jogador.pnl; // Acumular PNL da rodada
                            maos_jogadas++;
                            if (mao_jogador.resultado == 'V') {
                                vitorias++;
                            }
                            // Calcular loss_shoe baseado no PNL acumulado do shoe
                            if (pnl_shoe >= 0.0) {
                                loss_shoe = 0.0;
                            } else {
                                loss_shoe = -pnl_shoe / unidade_atual;
                            }
                        }
                        
                        // Log apenas se necessário e dentro do limite global
                        if (log_file && atomic_load(global_log_count) < log_level) {
                            char init_str[32];
                            char final_str[32];
                            mao_para_string(mao_jogador.initial_bits, init_str);
                            mao_para_string(mao_jogador.bits, final_str);
                            
                            fprintf(log_file, "%s,%c,%s,%s,%d,%s,%c,%.1f,%.1f,%c,%c,%c,%c\n", 
                                   init_str, upcard_char, 
                                   "-", final_str, mao_jogador.valor, 
                                   dealer_final_str, mao_jogador.resultado, mao_jogador.aposta, mao_jogador.pnl,
                                   'N', 'N', mao_jogador.blackjack ? 'S' : 'N', 'S');
                            atomic_fetch_add(global_log_count, 1);
                        }
                    }
                    
                    // Adicionar unidades da rodada à variável global
                    if (pnl_rodada_total != 0.0) {
                        double unidades_rodada = pnl_rodada_total / unidade_atual;
                        pthread_mutex_lock(&unidades_mutex);
                        unidades_total_global += unidades_rodada;
                        pthread_mutex_unlock(&unidades_mutex);
                    }
                    
                    // Coletar dados de frequência apenas UMA VEZ quando dealer tem BJ
                    if (any_freq_analysis && !freq_data_collected_this_round) {
                        // Determinar se deve coletar dados baseado nas flags (otimizado)
                        bool should_collect = false;
                        
                        if (freq_analysis_26 && freq_analysis_70) {
                            should_collect = (dealer_up_rank >= 2 && dealer_up_rank <= 10);
                        } else if (freq_analysis_26) {
                            should_collect = (dealer_up_rank >= 2 && dealer_up_rank <= 6);
                        } else if (freq_analysis_70) {
                            should_collect = (dealer_up_rank >= 7 && dealer_up_rank <= 10);
                        }
                        
                        if (freq_analysis_A && dealer_up_rank == 11) {
                            should_collect = true;
                        }
                        
                        if (should_collect && freq_buffer_count < FREQ_BUFFER_THRESHOLD) {
                            // Determinar resultado final do dealer - otimizado
                            int final_result = -1;
                            if (dealer_info.blackjack) {
                                final_result = (dealer_up_rank == 10 || dealer_up_rank == 11) ? 5 : 4; // BJ ou 21
                            } else if (dealer_info.valor >= 17 && dealer_info.valor <= 21) {
                                final_result = dealer_info.valor - 17; // 17->0, 18->1, 19->2, 20->3, 21->4
                            } else if (dealer_info.valor > 21) {
                                final_result = 6; // BUST
                            }
                            
                            // Adicionar ao buffer local - CORREÇÃO: usar true_count_for_stats
                            freq_buffer[freq_buffer_count].true_count = true_count_for_stats;
                            freq_buffer[freq_buffer_count].upcard = dealer_up_rank;
                            freq_buffer[freq_buffer_count].final_result = final_result;
                            freq_buffer_count++;

                            // NOVO: registrar ocorrência total deste upcard - CORREÇÃO: usar true_count_for_stats
                            int upcard_index = (dealer_up_rank == 11) ? 9 : dealer_up_rank - 2;
                            if (upcard_index >= 0 && upcard_index < 11 && freq_total_files[upcard_index]) {
                                write_freq_binary(freq_total_files[upcard_index], true_count_for_stats);
                            }
                            
                            freq_data_collected_this_round = true; // Marcar como coletado
                            
                            DEBUG_STATS("Dados freq coletados (dealer BJ): TC=%.3f, upcard=%d, final=%d", 
                                       true_count_for_stats, dealer_up_rank, final_result);
                            
                            // Flush buffer quando quase cheio para maior eficiência
                            if (freq_buffer_count >= FREQ_BUFFER_THRESHOLD) {
                                flush_freq_buffer();
                            }
                        }
                    }
                    
                    // Liberar memória apenas se não estiver usando pool thread-local
                    if (total_maos > MAX_MAOS_PER_ROUND) {
                        free(maos_bits);
                    }
                    continue; // Próxima rodada
                }
            }

            // Se chegou aqui: ou upcard não é Ás, ou é Ás mas dealer não tem BJ
            // Aplicar perda do insurance se foi feito (dealer não tem BJ)
            if (made_insurance) {
                bankroll -= insurance_bet;
                DEBUG_STATS("Insurance perdido: -%.2f", insurance_bet);
            }
            
            DEBUG_PRINT("Jogadores vão jogar suas mãos");
            
            // Primeiro, todos os jogadores jogam
            Mao all_hands[70]; // máximo 7 jogadores x 10 mãos cada
            int total_hands = 0;
            
            for (int pj = 0; pj < total_maos; ++pj) {
                DEBUG_PRINT("Jogador %d jogando", pj);
                
                Mao hands[10];
                int hand_count = 1;
                avaliar_mao(maos_bits[pj], &hands[0]);
                hands[0].aposta = bet;
                
                // Marcar mãos contabilizadas
                if (pj >= NUM_JOGADORES) {
                    hands[0].contabilizada = true;
                }
                
                for (int h = 0; h < hand_count; ++h) {
                    Mao *nova = &hands[hand_count];
                    Mao *m = &hands[h];
                    Mao *split_result = jogar_mao(m, &shoe, dealer_up_rank, nova, &running_count, &true_count, &shoe_counter, ev_realtime_enabled);
                    if (split_result) {
                        split_result->aposta = bet;
                        // Mãos split herdam o status de contabilizada
                        split_result->contabilizada = m->contabilizada;
                        hand_count++;
                        DEBUG_PRINT("Split detectado para jogador %d", pj);
                    }
                }
                
                // Coletar dados de split se análise está ativada e houve split
                if (split_analysis && hand_count >= 2) {
                    DEBUG_PRINT("Processando dados de split para jogador %d", pj);
                    
                    // Verificar se as mãos vieram de split
                    if (hands[0].from_split && hands[1].from_split) {
                        // Verificar se é um dos pares que analisamos (10 pares)
                        int pair_index = -1;
                        int upcard_index = -1;

                        // Rank do par salvo durante o split (0=2 … 12=A)
                        int rank_idx = hands[0].split_rank_idx;
                        if (rank_idx >= 0 && rank_idx <= 12) {
                            // Mapeamento para 10 pares: AA(12)->0, 1010(8)->1, 99(7)->2, 88(6)->3, 77(5)->4, 66(4)->5, 55(3)->6, 44(2)->7, 33(1)->8, 22(0)->9
                            // Removidos JJ(9), QQ(10), KK(11)
                            if (rank_idx == 12) pair_index = 0;      // AA
                            else if (rank_idx == 8) pair_index = 1;   // 1010
                            else if (rank_idx == 7) pair_index = 2;   // 99
                            else if (rank_idx == 6) pair_index = 3;   // 88
                            else if (rank_idx == 5) pair_index = 4;   // 77
                            else if (rank_idx == 4) pair_index = 5;   // 66
                            else if (rank_idx == 3) pair_index = 6;   // 55
                            else if (rank_idx == 2) pair_index = 7;   // 44
                            else if (rank_idx == 1) pair_index = 8;   // 33
                            else if (rank_idx == 0) pair_index = 9;   // 22
                            else pair_index = -1; // JJ, QQ, KK não são analisados
                        }

                        // Identificar o índice da upcard (0-9)
                        if (dealer_up_rank >= 2 && dealer_up_rank <= 11) {
                            upcard_index = dealer_up_rank - 2; // 2->0 … 10->8, A(11)->9
                        }
                        
                        DEBUG_STATS("Split analisado: rank_idx=%d, pair_index=%d, upcard_index=%d", 
                                   rank_idx, pair_index, upcard_index);
                        
                        // Se é um par/upcard que analisamos, registrar os dados
                        if (pair_index >= 0 && pair_index < 10 && upcard_index >= 0 && upcard_index < 10 && split_files[pair_index][upcard_index]) {
                            DEBUG_MUTEX("Obtendo mutex de split");
                            pthread_mutex_lock(split_mutex);
                            
                            // Contar cartas usadas (4 iniciais + cartas adicionais)
                            int total_cards_initial = 4; // 2 cartas por mão inicialmente
                            int cards_mao1 = __builtin_popcountll(hands[0].bits) - __builtin_popcountll(hands[0].initial_bits);
                            int cards_mao2 = __builtin_popcountll(hands[1].bits) - __builtin_popcountll(hands[1].initial_bits);
                            int total_cards_used = total_cards_initial + cards_mao1 + cards_mao2;
                            
                            // As mãos ainda não têm resultado calculado, então vamos aguardar
                            // Marcar as mãos para processamento posterior
                            hands[0].split_pair_index = pair_index;
                            hands[0].split_upcard_index = upcard_index;
                            hands[0].split_cards_used = total_cards_used;
                            hands[1].split_pair_index = pair_index;
                            hands[1].split_upcard_index = upcard_index;
                            hands[1].split_cards_used = total_cards_used;
                            
                            DEBUG_STATS("Split configurado para análise posterior: %d cartas usadas", total_cards_used);
                            
                            pthread_mutex_unlock(split_mutex);
                            DEBUG_MUTEX("Mutex de split liberado");
                        }
                    }
                }
                
                // Copiar todas as mãos para o array global
                for (int h = 0; h < hand_count; ++h) {
                    all_hands[total_hands++] = hands[h];
                }
            }
            
            DEBUG_PRINT("Dealer vai jogar - contabilizando hole card");
            
            // Agora que todos jogaram, dealer conta hole card e joga
            atualizar_counts(&running_count, &true_count, dealer_hole_card, shoe.total - shoe.topo);
            
            // Atualizar ShoeCounter com a hole card revelada
            int hole_rank_idx = carta_para_rank_idx(dealer_hole_card);
            if (hole_rank_idx >= 0 && hole_rank_idx < NUM_RANKS && shoe_counter.counts[hole_rank_idx] > 0) {
                shoe_counter.counts[hole_rank_idx]--;
                shoe_counter.total_cards--;
            }
            
            avaliar_mao_dealer(&dealer_info, &shoe, &running_count, &true_count);
            if (log_level > 0) {
                mao_para_string(dealer_info.bits, dealer_final_str);
            }
            
            DEBUG_STATS("Dealer final: valor=%d, BJ=%s, bust=%s", 
                       dealer_info.valor, dealer_info.blackjack ? "sim" : "não", 
                       dealer_info.valor > 21 ? "sim" : "não");
            
            // Coletar dados de frequência do dealer se análise está ativada - otimizado para usar buffer
            // APENAS SE NÃO FOI COLETADO ANTERIORMENTE
            if (any_freq_analysis && !freq_data_collected_this_round) {
                // Determinar se este upcard deve ser analisado (otimizado)
                bool should_collect = false;
                
                if ((dealer_up_rank >= 2 && dealer_up_rank <= 6 && freq_analysis_26) ||
                    (dealer_up_rank >= 7 && dealer_up_rank <= 10 && freq_analysis_70)) {
                    should_collect = true;
                } else if (dealer_up_rank == 11 && freq_analysis_A) {
                    should_collect = true;
                }
                
                if (should_collect && freq_buffer_count < FREQ_BUFFER_THRESHOLD) {
                    // Determinar resultado final do dealer - otimizado
                    int final_result = -1;
                    if (dealer_info.blackjack) {
                        final_result = (dealer_up_rank == 10 || dealer_up_rank == 11) ? 5 : 4; // BJ ou 21
                    } else if (dealer_info.valor >= 17 && dealer_info.valor <= 21) {
                        final_result = dealer_info.valor - 17; // 17->0, 18->1, 19->2, 20->3, 21->4
                    } else if (dealer_info.valor > 21) {
                        final_result = 6; // BUST
                    }
                    
                    // Adicionar ao buffer local - CORREÇÃO: usar true_count_for_stats
                    freq_buffer[freq_buffer_count].true_count = true_count_for_stats;
                    freq_buffer[freq_buffer_count].upcard = dealer_up_rank;
                    freq_buffer[freq_buffer_count].final_result = final_result;
                    freq_buffer_count++;

                    // NOVO: registrar ocorrência total deste upcard - CORREÇÃO: usar true_count_for_stats
                    int upcard_index = (dealer_up_rank == 11) ? 9 : dealer_up_rank - 2;
                    if (upcard_index >= 0 && upcard_index < 11 && freq_total_files[upcard_index]) {
                        write_freq_binary(freq_total_files[upcard_index], true_count_for_stats);
                    }
                    
                    freq_data_collected_this_round = true; // Marcar como coletado
                    
                    DEBUG_STATS("Dados freq coletados (dealer final): TC=%.3f, upcard=%d, final=%d", 
                               true_count_for_stats, dealer_up_rank, final_result);
                    
                    // Flush buffer quando quase cheio
                    if (freq_buffer_count >= FREQ_BUFFER_THRESHOLD) {
                        flush_freq_buffer();
                    }
                }
            }
            
            // Variável para acumular PNL total da rodada (mãos contabilizadas + insurance)
            double pnl_rodada_total = 0.0;
            
            // Incluir perda do insurance se foi feito (dealer não tem BJ)
            if (made_insurance) {
                pnl_rodada_total -= insurance_bet; // Perda do insurance
            }
            
            DEBUG_PRINT("Verificando resultados de todas as mãos");
            
            // Agora salvar todos os resultados
            for (int i = 0; i < total_hands; ++i) {
                Mao *m = &all_hands[i];
                
                verificar_mao(m, &dealer_info);
                calcular_pnl(m);

                // Atualizar estatísticas apenas para mãos contabilizadas
                if (m->contabilizada) {
                    bankroll += m->pnl;
                    pnl_shoe += m->pnl;
                    pnl_rodada_total += m->pnl; // Acumular PNL da rodada
                    maos_jogadas++;
                    if (m->resultado == 'V') {
                        vitorias++;
                    }
                    // Calcular loss_shoe baseado no PNL acumulado do shoe
                    if (pnl_shoe >= 0.0) {
                        loss_shoe = 0.0;
                    } else {
                        loss_shoe = -pnl_shoe / unidade_atual;
                    }
                    
                    DEBUG_STATS("Mão contabilizada %d: resultado=%c, PNL=%.2f", i, m->resultado, m->pnl);
                }

                // Log apenas se necessário e dentro do limite global
                if (log_file && atomic_load(global_log_count) < log_level) {
                    // Fazer conversões string apenas quando necessário para logging
                    char init_str[32];
                    char final_str[32];
                    mao_para_string(m->initial_bits, init_str);
                    mao_para_string(m->bits, final_str);
                    
                    fprintf(log_file, "%s,%c,%s,%s,%d,%s,%c,%.1f,%.1f,%c,%c,%c,%c\n", 
                           init_str, upcard_char, 
                           (m->hist_len>0?m->historico:"-"), final_str, m->valor, 
                           dealer_final_str, m->resultado, m->aposta, m->pnl,
                           m->isdouble ? 'S' : 'N',
                           m->from_split ? 'S' : 'N',
                           m->blackjack ? 'S' : 'N',
                           dealer_info.blackjack ? 'S' : 'N');
                    atomic_fetch_add(global_log_count, 1);
                }
            }
            
            // Processar dados de split após calcular todos os resultados
            if (split_analysis && split_mutex) {
                DEBUG_PRINT("Processando dados de split finais");
                
                // Buscar pares de mãos de split para registrar dados
                for (int i = 0; i < total_hands - 1; i++) {
                    Mao *m1 = &all_hands[i];
                    
                    // Verificar se é uma mão de split
                    if (m1->from_split && m1->split_pair_index >= 0) {
                        // Buscar a mão par correspondente
                        for (int j = i + 1; j < total_hands; j++) {
                            Mao *m2 = &all_hands[j];
                            
                            if (m2->from_split && 
                                m2->split_pair_index == m1->split_pair_index &&
                                m2->split_upcard_index == m1->split_upcard_index) {
                                
                                DEBUG_PRINT("Registrando dados de split para pair_index=%d, upcard_index=%d", 
                                           m1->split_pair_index, m1->split_upcard_index);
                                
                                // Registrar dados do split
                                pthread_mutex_lock(split_mutex);
                                
                                if (m1->split_pair_index >= 0 && m1->split_pair_index < 10 && 
                                    m1->split_upcard_index >= 0 && m1->split_upcard_index < 10 &&
                                    split_files[m1->split_pair_index][m1->split_upcard_index]) {
                                    
                                    // Calcular combinações reais dos resultados
                                    int mao1_win = (m1->resultado == 'V') ? 1 : 0;
                                    int mao1_push = (m1->resultado == 'E') ? 1 : 0;
                                    int mao1_lose = (m1->resultado == 'D') ? 1 : 0;
                                    
                                    int mao2_win = (m2->resultado == 'V') ? 1 : 0;
                                    int mao2_push = (m2->resultado == 'E') ? 1 : 0;
                                    int mao2_lose = (m2->resultado == 'D') ? 1 : 0;
                                    
                                    // Calcular combinações reais de resultados
                                    int lose_lose = (mao1_lose && mao2_lose) ? 1 : 0;
                                    int win_win = (mao1_win && mao2_win) ? 1 : 0;
                                    int push_push = (mao1_push && mao2_push) ? 1 : 0;
                                    int lose_win = (mao1_lose && mao2_win) ? 1 : 0;
                                    int lose_push = (mao1_lose && mao2_push) ? 1 : 0;
                                    int win_lose = (mao1_win && mao2_lose) ? 1 : 0;
                                    int win_push = (mao1_win && mao2_push) ? 1 : 0;
                                    int push_lose = (mao1_push && mao2_lose) ? 1 : 0;
                                    int push_win = (mao1_push && mao2_win) ? 1 : 0;
                                    
                                    DEBUG_STATS("Split resultado: mão1=%c, mão2=%c, combinação LL=%d WW=%d PP=%d", 
                                               m1->resultado, m2->resultado, lose_lose, win_win, push_push);
                                    
                                    write_split_binary(split_files[m1->split_pair_index][m1->split_upcard_index],
                                                     true_count, lose_lose, win_win, push_push, lose_win, 
                                                     lose_push, win_lose, win_push, push_lose, push_win, 
                                                     m1->split_cards_used);
                                }
                                
                                pthread_mutex_unlock(split_mutex);
                                
                                // Marcar mãos como processadas para evitar duplicação
                                m2->split_pair_index = -1;
                                break;
                            }
                        }
                        
                        // Marcar mão como processada
                        m1->split_pair_index = -1;
                    }
                }
            }
            
            // Adicionar unidades da rodada à variável global
            if (pnl_rodada_total != 0.0) {
                double unidades_rodada = pnl_rodada_total / unidade_atual;
                pthread_mutex_lock(&unidades_mutex);
                unidades_total_global += unidades_rodada;
                pthread_mutex_unlock(&unidades_mutex);
                
                DEBUG_STATS("PNL rodada: %.4f unidades", unidades_rodada);
            }
            
            // Liberar memória apenas se não estiver usando pool thread-local
            if (total_maos > MAX_MAOS_PER_ROUND) {
                free(maos_bits);
            }
        }
        
        baralho_destruir(&shoe);
        shoes_jogados++;
        running_count = 0.0; // Reset para novo shoe
        true_count = 0.0;    // Reset true count também
        pnl_shoe = 0.0;      // Reset PNL do shoe
        loss_shoe = 0.0;     // Reset perdas do shoe
        
        DEBUG_PRINT("Shoe %d concluído, resetando counts", shoes_jogados);
    }
    
    finish_simulation:
    DEBUG_PRINT("Finalizando simulação %d", sim_id);
    
    if (log_file) {
        fclose(log_file);
        DEBUG_IO("Arquivo de log fechado");
    }
    
    if (dealer_file) {
        fclose(dealer_file);
        DEBUG_IO("Arquivo de dealer fechado");
    }
    
    // Fechar arquivos de frequência
    if (any_freq_analysis) {
        // Fechar arquivos de total
        for (int i = 0; i < 11; i++) {
            if (freq_total_files[i]) {
                fclose(freq_total_files[i]);
            }
        }
        
        // Fechar arquivos de resultado
        for (int i = 0; i < 11; i++) {
            for (int j = 0; j < 7; j++) {
                if (freq_result_files[i][j]) {
                    fclose(freq_result_files[i][j]);
                }
            }
        }
        DEBUG_IO("Arquivos de frequência fechados");
    }
    
    // Fechar arquivos de split
    if (split_analysis) {
        for (int p = 0; p < 10; p++) {
            for (int u = 0; u < 10; u++) {
                if (split_files[p][u]) {
                    fclose(split_files[p][u]);
                }
            }
        }
        DEBUG_IO("Arquivos de split fechados");
    }

    // Flush todos os buffers no final da simulação
    if (freq_buffer_count > 0) {
        DEBUG_PRINT("Flush final do buffer de frequência");
        flush_freq_buffer();
    }
    if (dealer_buffer_count > 0) {
        DEBUG_PRINT("Flush final do buffer de dealer");
        flush_dealer_buffer();
    }
    
    DEBUG_PRINT("Simulação %d concluída com sucesso", sim_id);
}
