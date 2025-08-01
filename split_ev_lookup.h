#ifndef SPLIT_EV_LOOKUP_H
#define SPLIT_EV_LOOKUP_H

#include "structures.h"
#include <stdbool.h>

// Constantes para indexação
#define NUM_PAIRS 10
#define NUM_UPCARDS 10

// Enum para tipos de pares (ordem: AA, 1010, 99, 88, 77, 66, 55, 44, 33, 22)
typedef enum {
    PAIR_AA = 0,
    PAIR_1010 = 1, 
    PAIR_99 = 2,
    PAIR_88 = 3,
    PAIR_77 = 4,
    PAIR_66 = 5,
    PAIR_55 = 6,
    PAIR_44 = 7,
    PAIR_33 = 8,
    PAIR_22 = 9
} PairType;

// Enum para upcards (ordem: 2, 3, 4, 5, 6, 7, 8, 9, 10, A)
typedef enum {
    UPCARD_2 = 0,
    UPCARD_3 = 1,
    UPCARD_4 = 2,
    UPCARD_5 = 3,
    UPCARD_6 = 4,
    UPCARD_7 = 5,
    UPCARD_8 = 6,
    UPCARD_9 = 7,
    UPCARD_10 = 8,
    UPCARD_A = 9
} UpcardType;

// Tabela de lookup global: [par][upcard][tc_bin] -> EV
extern double split_ev_table[NUM_PAIRS][NUM_UPCARDS][MAX_BINS];
extern bool split_ev_table_loaded;

// Funções de inicialização
bool load_split_ev_table(const char* results_dir);
void unload_split_ev_table(void);

// Funções de conversão
int pair_rank_to_index(int pair_rank);
int upcard_to_index(int upcard);
int get_tc_bin_index(double true_count);

// Função principal de lookup
double get_split_ev(int pair_rank, int dealer_upcard, double true_count);

// Funções auxiliares
bool is_valid_pair(int pair_rank);
bool is_valid_upcard(int upcard);
double get_split_ev_safe(int pair_rank, int dealer_upcard, double true_count, double default_ev);

// Função de debug/diagnóstico
void print_split_ev_stats(void);

#endif // SPLIT_EV_LOOKUP_H 