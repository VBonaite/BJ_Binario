#include "constantes.h"

// Definições das constantes globais
const int NUM_JOGADORES = 4;
const int DECKS = 8;
const double PENETRACAO = 0.5; 
const double UNIDADE = 1.0;
const int NUM_SHOES = 1000;
const int NUM_SIMS = 1000;
const char* OUT_DIR = "/mnt/dados/BJ_Binario/Resultados";
const double WONG_HALVES[13] = {
    0.5,  // 2
    1.0,  // 3
    1.0,  // 4
    1.5,  // 5
    1.0,  // 6
    0.5,  // 7
    0.0,  // 8
    -0.5, // 9
    -1.0, // 10
    -1.0, // J
    -1.0, // Q
    -1.0, // K
    -1.0  // A
};

// Constantes para sistema de apostas
const double BANKROLL_INICIAL = 7500.0;
const double UNIDADE_INICIAL = 5.0;

const double MIN_COUNT_INS = 3.7;
const double A_perc = 0.10;  // 10% - porcentagem mínima de Áses para insurance

const int CARTAS_RESTANTES_LIMITE = 412;
const int CARTAS_RESTANTES_SHOE_OK = 296;

// Parâmetros para definir_aposta (12 blocos de condições)
const double MIN_PCT[12] = {
    9.96, 17.92, 19.83, 26.72, 27.94, 23.51, 28.06, 30.923, 33.81, 36.37, 40.70, 45.32
};

const int MIN_LEN_SHOE[12] = {
    292, 316, 318, 328, 353, 384, 390,400, 417, 428, 448, 452
};

const double MIN_TRUE_COUNT[12] = {
    4.88, 3.11, 2.01, 1.52, 0.99, 0.79, 0.65, 0.37, 0.34, 0.32, 0.29, 0.078
};

// Novos arrays de constantes
const double TC_MAOS_CONTAB[3] = {
    0.95, 1.95, 2.87
};

const double TC_AJUSTA_UNIDADES[10] = {
    0.16, 0.61, 0.71, 1.66, 2.01, 2.27, 3.43, 3.76, 3.65, 4.52
};

const double APOSTAS_BASE_AJUSTE[10] = {
    1.01, 3.84, 4.81, 7.18, 8.51, 11.60, 17.03, 16.05, 14.44, 19.33
};

const double APOSTAS_BASE[12] = {
    35.00, 25.00, 17.00, 16.00, 12.00, 12.00, 12.00, 5.00, 4.00, 3.00, 2.00, 1.00
};

// Variável global para acumular unidades totais (protegida por mutex)
double unidades_total_global = 0.0;
pthread_mutex_t unidades_mutex = PTHREAD_MUTEX_INITIALIZER;