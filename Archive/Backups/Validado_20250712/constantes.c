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
const double MIN_COUNT_INS = 3.5;

// Constantes para sistema de apostas
const double BANKROLL_INICIAL = 7500.0;
const double UNIDADE_INICIAL = 5.0;

// Parâmetros para definir_aposta (12 blocos de condições)
const double MIN_PCT[12] = {
    12.0, 14.0, 16.0, 18.0, 20.0, 22.0, 24.0, 26.0, 30.0, 32.0, 34.0, 36.0
};

const int MIN_LEN_SHOE[12] = {
    269, 277, 294, 312, 327, 344, 
    360, 370, 380, 390, 400, 415
};

const double MIN_TRUE_COUNT[12] = {
    4.45, 4.45, 3.85, 3.35, 2.85, 2.35, 1.65, 1.65, 0.75, 0.75, 0.35, 0.35
};

// Novos arrays de constantes
const double TC_MAOS_CONTAB[3] = {
    0.21, 1.07, 1.93
};

const double TC_AJUSTA_UNIDADES[10] = {
    0.0, 0.35, 0.75, 1.65, 2.35, 2.85, 3.35, 3.85, 4.45, 4.45
};

const double APOSTAS_BASE_AJUSTE[10] = {
    1.0, 2.0, 3.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0
};

const double APOSTAS_BASE[12] = {
    32.0, 22.0, 15.0, 12.0, 10.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0
};

const int CARTAS_RESTANTES_LIMITE = 401;
const int CARTAS_RESTANTES_SHOE_OK = 286;

// Variável global para acumular unidades totais (protegida por mutex)
double unidades_total_global = 0.0;
pthread_mutex_t unidades_mutex = PTHREAD_MUTEX_INITIALIZER;
