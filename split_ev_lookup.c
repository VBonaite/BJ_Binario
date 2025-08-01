#include "split_ev_lookup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Tabela de lookup global
double split_ev_table[NUM_PAIRS][NUM_UPCARDS][MAX_BINS];
bool split_ev_table_loaded = false;

// Mapeamentos para conversão
static const char* pair_names[NUM_PAIRS] = {
    "AA", "1010", "99", "88", "77", "66", "55", "44", "33", "22"
};

static const char* upcard_names[NUM_UPCARDS] = {
    "2", "3", "4", "5", "6", "7", "8", "9", "10", "A"
};

// Função para converter rank do par para índice
int pair_rank_to_index(int pair_rank) {
    switch (pair_rank) {
        case 11: return PAIR_AA;     // Ás
        case 10: return PAIR_1010;   // 10, J, Q, K
        case 9:  return PAIR_99;
        case 8:  return PAIR_88;
        case 7:  return PAIR_77;
        case 6:  return PAIR_66;
        case 5:  return PAIR_55;
        case 4:  return PAIR_44;
        case 3:  return PAIR_33;
        case 2:  return PAIR_22;
        default: return -1; // Inválido
    }
}

// Função para converter upcard para índice
int upcard_to_index(int upcard) {
    if (upcard >= 2 && upcard <= 9) {
        return upcard - 2; // 2->0, 3->1, ..., 9->7
    } else if (upcard == 10) {
        return UPCARD_10; // 8
    } else if (upcard == 11) {
        return UPCARD_A;  // 9
    } else {
        return -1; // Inválido
    }
}

// Função para converter true count para índice de bin
int get_tc_bin_index(double true_count) {
    if (true_count < MIN_TC) return 0;
    if (true_count >= MAX_TC) return MAX_BINS - 1;
    
    int bin_idx = (int)((true_count - MIN_TC) / BIN_WIDTH);
    if (bin_idx < 0) return 0;
    if (bin_idx >= MAX_BINS) return MAX_BINS - 1;
    
    return bin_idx;
}

// Função para verificar se o par é válido
bool is_valid_pair(int pair_rank) {
    return pair_rank_to_index(pair_rank) != -1;
}

// Função para verificar se o upcard é válido
bool is_valid_upcard(int upcard) {
    return upcard_to_index(upcard) != -1;
}

// Função principal de lookup
double get_split_ev(int pair_rank, int dealer_upcard, double true_count) {
    if (!split_ev_table_loaded) {
        fprintf(stderr, "ERRO: Tabela de EV de splits não foi carregada!\n");
        return 0.0;
    }
    
    int pair_idx = pair_rank_to_index(pair_rank);
    int upcard_idx = upcard_to_index(dealer_upcard);
    int tc_bin_idx = get_tc_bin_index(true_count);
    
    if (pair_idx == -1 || upcard_idx == -1) {
        fprintf(stderr, "ERRO: Par (%d) ou upcard (%d) inválido!\n", pair_rank, dealer_upcard);
        return 0.0;
    }
    
    return split_ev_table[pair_idx][upcard_idx][tc_bin_idx];
}

// Função de lookup com valor padrão
double get_split_ev_safe(int pair_rank, int dealer_upcard, double true_count, double default_ev) {
    if (!split_ev_table_loaded) {
        return default_ev;
    }
    
    int pair_idx = pair_rank_to_index(pair_rank);
    int upcard_idx = upcard_to_index(dealer_upcard);
    int tc_bin_idx = get_tc_bin_index(true_count);
    
    if (pair_idx == -1 || upcard_idx == -1) {
        return default_ev;
    }
    
    return split_ev_table[pair_idx][upcard_idx][tc_bin_idx];
}

// Função para carregar a tabela de lookup
bool load_split_ev_table(const char* results_dir) {
    printf("Carregando tabela de EV de splits...\n");
    
    // Inicializar tabela com zeros
    memset(split_ev_table, 0, sizeof(split_ev_table));
    
    int files_loaded = 0;
    int files_total = NUM_PAIRS * NUM_UPCARDS;
    
    // Carregar dados de cada combinação par/upcard
    for (int p = 0; p < NUM_PAIRS; p++) {
        for (int u = 0; u < NUM_UPCARDS; u++) {
            char csv_filename[512];
            snprintf(csv_filename, sizeof(csv_filename), 
                    "%s/splits/split_outcome_%s_vs_%s_3M.csv", 
                    results_dir, pair_names[p], upcard_names[u]);
            
            FILE* csv_file = fopen(csv_filename, "r");
            if (!csv_file) {
                fprintf(stderr, "Aviso: Arquivo não encontrado: %s\n", csv_filename);
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
                int total_splits, total_hands;
                double freq_lose_lose, freq_win_win, freq_push_push;
                double freq_lose_win, freq_lose_push, freq_win_lose;
                double freq_win_push, freq_push_lose, freq_push_win;
                double expected_value, avg_cards, std_cards;
                
                int parsed = sscanf(line, "%lf,%lf,%lf,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
                                  &tc_min, &tc_max, &tc_center, &total_splits, &total_hands,
                                  &freq_lose_lose, &freq_win_win, &freq_push_push,
                                  &freq_lose_win, &freq_lose_push, &freq_win_lose,
                                  &freq_win_push, &freq_push_lose, &freq_push_win,
                                  &expected_value, &avg_cards, &std_cards);
                
                if (parsed == 17) {
                    int tc_bin_idx = get_tc_bin_index(tc_center);
                    split_ev_table[p][u][tc_bin_idx] = expected_value;
                    lines_read++;
                }
            }
            
            fclose(csv_file);
            files_loaded++;
            
            printf("  Carregado: %s (%d linhas)\n", csv_filename, lines_read);
        }
    }
    
    split_ev_table_loaded = (files_loaded > 0);
    
    printf("Tabela de EV de splits carregada: %d/%d arquivos processados\n", 
           files_loaded, files_total);
    
    return split_ev_table_loaded;
}

// Função para descarregar a tabela
void unload_split_ev_table(void) {
    memset(split_ev_table, 0, sizeof(split_ev_table));
    split_ev_table_loaded = false;
    printf("Tabela de EV de splits descarregada\n");
}

// Função de diagnóstico
void print_split_ev_stats(void) {
    if (!split_ev_table_loaded) {
        printf("Tabela de EV de splits não carregada\n");
        return;
    }
    
    printf("\n=== ESTATÍSTICAS DA TABELA DE EV DE SPLITS ===\n");
    
    // Estatísticas por par
    for (int p = 0; p < NUM_PAIRS; p++) {
        double min_ev = 999.0, max_ev = -999.0;
        double sum_ev = 0.0;
        int count = 0;
        
        for (int u = 0; u < NUM_UPCARDS; u++) {
            for (int tc = 0; tc < MAX_BINS; tc++) {
                double ev = split_ev_table[p][u][tc];
                if (ev != 0.0) { // Assumir que 0.0 significa dados não disponíveis
                    if (ev < min_ev) min_ev = ev;
                    if (ev > max_ev) max_ev = ev;
                    sum_ev += ev;
                    count++;
                }
            }
        }
        
        if (count > 0) {
            printf("Par %s: EV mín=%.4f, máx=%.4f, média=%.4f (%d pontos)\n",
                   pair_names[p], min_ev, max_ev, sum_ev/count, count);
        } else {
            printf("Par %s: Sem dados\n", pair_names[p]);
        }
    }
    
    printf("===============================================\n");
} 