#include "dealer_freq_lookup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Tabela de lookup global
double dealer_freq_table[NUM_DEALER_UPCARDS][NUM_FINAL_RESULTS][MAX_BINS];
bool dealer_freq_table_loaded = false;

// Mapeamentos para conversão
static const char* dealer_upcard_names[NUM_DEALER_UPCARDS] = {
    "2", "3", "4", "5", "6", "7", "8", "9", "10", "A"
};

static const char* dealer_result_names[NUM_FINAL_RESULTS] = {
    "17", "18", "19", "20", "21", "BJ", "BUST"
};

// Mapeamento de diretórios por upcard
static const char* get_freq_dir_for_upcard(int upcard) {
    if (upcard >= 2 && upcard <= 6) {
        return "freq_2_6";
    } else if (upcard >= 7 && upcard <= 10) {
        return "freq_7_0";
    } else if (upcard == 11) {
        return "freq_A";
    } else {
        return NULL;
    }
}

// Função para converter upcard do dealer para índice
int dealer_upcard_to_index(int upcard) {
    if (upcard >= 2 && upcard <= 9) {
        return upcard - 2; // 2->0, 3->1, ..., 9->7
    } else if (upcard == 10) {
        return DEALER_UPCARD_10; // 8
    } else if (upcard == 11) {
        return DEALER_UPCARD_A;  // 9
    } else {
        return -1; // Inválido
    }
}

// Função para converter resultado final para índice
int dealer_result_to_index(int result) {
    switch (result) {
        case 17: return DEALER_RESULT_17;
        case 18: return DEALER_RESULT_18;
        case 19: return DEALER_RESULT_19;
        case 20: return DEALER_RESULT_20;
        case 21: return DEALER_RESULT_21;
        case 22: return DEALER_RESULT_BJ;   // Usando 22 para BJ
        case 23: return DEALER_RESULT_BUST; // Usando 23 para BUST
        default: return -1; // Inválido
    }
}

// Função para converter nome do resultado para índice
int dealer_name_to_result(const char* name) {
    if (strcmp(name, "17") == 0) return 17;
    if (strcmp(name, "18") == 0) return 18;
    if (strcmp(name, "19") == 0) return 19;
    if (strcmp(name, "20") == 0) return 20;
    if (strcmp(name, "21") == 0) return 21;
    if (strcmp(name, "BJ") == 0) return 22;
    if (strcmp(name, "BUST") == 0) return 23;
    return -1;
}

// Função para converter índice de resultado para nome
const char* dealer_result_to_name(int result) {
    if (result >= 17 && result <= 21) {
        static char buffer[8];
        snprintf(buffer, sizeof(buffer), "%d", result);
        return buffer;
    } else if (result == 22) {
        return "BJ";
    } else if (result == 23) {
        return "BUST";
    } else {
        return "UNKNOWN";
    }
}

// Função para verificar se o upcard é válido
bool is_valid_dealer_upcard(int upcard) {
    return dealer_upcard_to_index(upcard) != -1;
}

// Função para verificar se o resultado é válido
bool is_valid_dealer_result(int result) {
    return dealer_result_to_index(result) != -1;
}

// Função principal de lookup
double get_dealer_freq(int dealer_upcard, int final_result, double true_count) {
    if (!dealer_freq_table_loaded) {
        fprintf(stderr, "ERRO: Tabela de frequências do dealer não foi carregada!\n");
        return 0.0;
    }
    
    int upcard_idx = dealer_upcard_to_index(dealer_upcard);
    int result_idx = dealer_result_to_index(final_result);
    int tc_bin_idx = get_tc_bin_index(true_count);
    
    if (upcard_idx == -1 || result_idx == -1) {
        fprintf(stderr, "ERRO: Upcard (%d) ou resultado (%d) inválido!\n", dealer_upcard, final_result);
        return 0.0;
    }
    
    return dealer_freq_table[upcard_idx][result_idx][tc_bin_idx];
}

// Função de lookup com valor padrão
double get_dealer_freq_safe(int dealer_upcard, int final_result, double true_count, double default_freq) {
    if (!dealer_freq_table_loaded) {
        return default_freq;
    }
    
    int upcard_idx = dealer_upcard_to_index(dealer_upcard);
    int result_idx = dealer_result_to_index(final_result);
    int tc_bin_idx = get_tc_bin_index(true_count);
    
    if (upcard_idx == -1 || result_idx == -1) {
        return default_freq;
    }
    
    return dealer_freq_table[upcard_idx][result_idx][tc_bin_idx];
}

// Implementação movida para split_ev_lookup.c para evitar duplicação

// Funções de conveniência
double get_dealer_freq_17(int dealer_upcard, double true_count) {
    return get_dealer_freq(dealer_upcard, 17, true_count);
}

double get_dealer_freq_18(int dealer_upcard, double true_count) {
    return get_dealer_freq(dealer_upcard, 18, true_count);
}

double get_dealer_freq_19(int dealer_upcard, double true_count) {
    return get_dealer_freq(dealer_upcard, 19, true_count);
}

double get_dealer_freq_20(int dealer_upcard, double true_count) {
    return get_dealer_freq(dealer_upcard, 20, true_count);
}

double get_dealer_freq_21(int dealer_upcard, double true_count) {
    return get_dealer_freq(dealer_upcard, 21, true_count);
}

double get_dealer_freq_bj(int dealer_upcard, double true_count) {
    return get_dealer_freq(dealer_upcard, 22, true_count);
}

double get_dealer_freq_bust(int dealer_upcard, double true_count) {
    return get_dealer_freq(dealer_upcard, 23, true_count);
}

// Função para obter todas as frequências
DealerFreqAll get_dealer_freq_all(int dealer_upcard, double true_count) {
    DealerFreqAll result = {0};
    
    result.freq_17 = get_dealer_freq_17(dealer_upcard, true_count);
    result.freq_18 = get_dealer_freq_18(dealer_upcard, true_count);
    result.freq_19 = get_dealer_freq_19(dealer_upcard, true_count);
    result.freq_20 = get_dealer_freq_20(dealer_upcard, true_count);
    result.freq_21 = get_dealer_freq_21(dealer_upcard, true_count);
    result.freq_bj = get_dealer_freq_bj(dealer_upcard, true_count);
    result.freq_bust = get_dealer_freq_bust(dealer_upcard, true_count);
    
    result.total_freq = result.freq_17 + result.freq_18 + result.freq_19 + 
                       result.freq_20 + result.freq_21 + result.freq_bj + 
                       result.freq_bust;
    
    return result;
}

// Função para carregar a tabela de lookup
bool load_dealer_freq_table(const char* results_dir) {
    printf("Carregando tabela de frequências do dealer...\n");
    
    // Inicializar tabela com zeros
    memset(dealer_freq_table, 0, sizeof(dealer_freq_table));
    
    int files_loaded = 0;
    int files_total = 0;
    
    // Carregar dados de cada combinação upcard/resultado
    for (int u = 0; u < NUM_DEALER_UPCARDS; u++) {
        int upcard_value = (u <= 7) ? (u + 2) : ((u == 8) ? 10 : 11); // 2-9, 10, A
        const char* freq_dir = get_freq_dir_for_upcard(upcard_value);
        
        if (!freq_dir) {
            continue;
        }
        
        for (int r = 0; r < NUM_FINAL_RESULTS; r++) {
            // Pular BJ para upcards que não podem ter blackjack
            if (r == DEALER_RESULT_BJ && upcard_value != 10 && upcard_value != 11) {
                continue;
            }
            
            files_total++;
            
            char csv_filename[512];
            snprintf(csv_filename, sizeof(csv_filename), 
                    "%s/%s/freq_%s_%s_3M.csv", 
                    results_dir, freq_dir, dealer_upcard_names[u], dealer_result_names[r]);
            
            FILE* csv_file = fopen(csv_filename, "r");
            if (!csv_file) {
                printf("  Aviso: Arquivo não encontrado: %s\n", csv_filename);
                continue;
            }
            
            char line[1024];
            bool header_skipped = false;
            int lines_read = 0;
            
            while (fgets(line, sizeof(line), csv_file)) {
                if (!header_skipped) {
                    header_skipped = true;
                    continue;
                }
                
                // Parse da linha CSV
                double tc_min, tc_max, tc_center;
                int total_upcard_count, final_count;
                double frequency;
                
                int parsed = sscanf(line, "%lf,%lf,%lf,%d,%d,%lf",
                                  &tc_min, &tc_max, &tc_center, 
                                  &total_upcard_count, &final_count, &frequency);
                
                if (parsed == 6) {
                    int tc_bin_idx = get_tc_bin_index(tc_center);
                    // CORREÇÃO CRÍTICA: Converter de percentual para decimal
                    dealer_freq_table[u][r][tc_bin_idx] = frequency / 100.0;
                    lines_read++;
                }
            }
            
            fclose(csv_file);
            files_loaded++;
            
            printf("  Carregado: %s (%d linhas)\n", csv_filename, lines_read);
        }
    }
    
    dealer_freq_table_loaded = (files_loaded > 0);
    
    printf("Tabela de frequências do dealer carregada: %d/%d arquivos processados\n", 
           files_loaded, files_total);
    
    return dealer_freq_table_loaded;
}

// Função para descarregar a tabela
void unload_dealer_freq_table(void) {
    memset(dealer_freq_table, 0, sizeof(dealer_freq_table));
    dealer_freq_table_loaded = false;
    printf("Tabela de frequências do dealer descarregada\n");
}

// Função de diagnóstico
void print_dealer_freq_stats(void) {
    if (!dealer_freq_table_loaded) {
        printf("Tabela de frequências do dealer não carregada\n");
        return;
    }
    
    printf("\n=== ESTATÍSTICAS DA TABELA DE FREQUÊNCIAS DO DEALER ===\n");
    
    // Estatísticas por upcard
    for (int u = 0; u < NUM_DEALER_UPCARDS; u++) {
        double total_freq_sum = 0.0;
        int valid_bins = 0;
        
        for (int tc = 0; tc < MAX_BINS; tc++) {
            double bin_total = 0.0;
            for (int r = 0; r < NUM_FINAL_RESULTS; r++) {
                bin_total += dealer_freq_table[u][r][tc];
            }
            if (bin_total > 0.0) {
                total_freq_sum += bin_total;
                valid_bins++;
            }
        }
        
        if (valid_bins > 0) {
            double avg_total_freq = total_freq_sum / valid_bins;
            printf("Upcard %s: Frequência total média=%.2f%% (%d bins válidos)\n",
                   dealer_upcard_names[u], avg_total_freq, valid_bins);
            
            // Mostrar frequências por resultado para TC neutro (bin central)
            int neutral_bin = get_tc_bin_index(0.0);
            printf("  TC~0: ");
            for (int r = 0; r < NUM_FINAL_RESULTS; r++) {
                double freq = dealer_freq_table[u][r][neutral_bin];
                if (freq > 0.0) {
                    printf("%s=%.1f%% ", dealer_result_names[r], freq);
                }
            }
            printf("\n");
        } else {
            printf("Upcard %s: Sem dados\n", dealer_upcard_names[u]);
        }
    }
    
    printf("=========================================================\n");
} 