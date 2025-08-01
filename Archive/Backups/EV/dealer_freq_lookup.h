#ifndef DEALER_FREQ_LOOKUP_H
#define DEALER_FREQ_LOOKUP_H

#include "structures.h"
#include <stdbool.h>

// Constantes para indexação
#define NUM_DEALER_UPCARDS 10
#define NUM_FINAL_RESULTS 7

// Enum para upcards do dealer (ordem: 2, 3, 4, 5, 6, 7, 8, 9, 10, A)
typedef enum {
    DEALER_UPCARD_2 = 0,
    DEALER_UPCARD_3 = 1,
    DEALER_UPCARD_4 = 2,
    DEALER_UPCARD_5 = 3,
    DEALER_UPCARD_6 = 4,
    DEALER_UPCARD_7 = 5,
    DEALER_UPCARD_8 = 6,
    DEALER_UPCARD_9 = 7,
    DEALER_UPCARD_10 = 8,
    DEALER_UPCARD_A = 9
} DealerUpcardType;

// Enum para resultados finais do dealer (ordem: 17, 18, 19, 20, 21, BJ, BUST)
typedef enum {
    DEALER_RESULT_17 = 0,
    DEALER_RESULT_18 = 1,
    DEALER_RESULT_19 = 2,
    DEALER_RESULT_20 = 3,
    DEALER_RESULT_21 = 4,
    DEALER_RESULT_BJ = 5,
    DEALER_RESULT_BUST = 6
} DealerResultType;

// Tabela de lookup global: [upcard][resultado][tc_bin] -> frequência
extern double dealer_freq_table[NUM_DEALER_UPCARDS][NUM_FINAL_RESULTS][MAX_BINS];
extern bool dealer_freq_table_loaded;

// Funções de inicialização
bool load_dealer_freq_table(const char* results_dir);
void unload_dealer_freq_table(void);

// Funções de conversão
int dealer_upcard_to_index(int upcard);
int dealer_result_to_index(int result);
const char* dealer_result_to_name(int result);
int dealer_name_to_result(const char* name);
int get_tc_bin_index(double true_count);

// Função principal de lookup
double get_dealer_freq(int dealer_upcard, int final_result, double true_count);

// Funções auxiliares
bool is_valid_dealer_upcard(int upcard);
bool is_valid_dealer_result(int result);
double get_dealer_freq_safe(int dealer_upcard, int final_result, double true_count, double default_freq);

// Funções de conveniência
double get_dealer_freq_17(int dealer_upcard, double true_count);
double get_dealer_freq_18(int dealer_upcard, double true_count);
double get_dealer_freq_19(int dealer_upcard, double true_count);
double get_dealer_freq_20(int dealer_upcard, double true_count);
double get_dealer_freq_21(int dealer_upcard, double true_count);
double get_dealer_freq_bj(int dealer_upcard, double true_count);
double get_dealer_freq_bust(int dealer_upcard, double true_count);

// Função para obter todas as frequências de uma vez
typedef struct {
    double freq_17;
    double freq_18;
    double freq_19;
    double freq_20;
    double freq_21;
    double freq_bj;
    double freq_bust;
    double total_freq; // Soma de todas (deve ser ~100%)
} DealerFreqAll;

DealerFreqAll get_dealer_freq_all(int dealer_upcard, double true_count);

// Função de debug/diagnóstico
void print_dealer_freq_stats(void);

#endif // DEALER_FREQ_LOOKUP_H 