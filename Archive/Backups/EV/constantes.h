#ifndef CONSTANTES_H
#define CONSTANTES_H

#include <stdatomic.h>
#include <pthread.h>

extern const int NUM_JOGADORES;
extern const int DECKS;
extern const double PENETRACAO;
extern const double UNIDADE;
extern const int NUM_SHOES;
extern const int NUM_SIMS;
extern const char* OUT_DIR;
extern const double WONG_HALVES[13];
extern const double MIN_COUNT_INS;

// Constantes para sistema de apostas
extern const double BANKROLL_INICIAL;
extern const double UNIDADE_INICIAL;
extern const double MIN_PCT[12];
extern const int MIN_LEN_SHOE[12];
extern const double MIN_TRUE_COUNT[12];

// Novos arrays de constantes para os items solicitados
extern const double TC_MAOS_CONTAB[3];
extern const double TC_AJUSTA_UNIDADES[10];
extern const double APOSTAS_BASE_AJUSTE[10];
extern const double APOSTAS_BASE[12];
extern const int CARTAS_RESTANTES_LIMITE;
extern const int CARTAS_RESTANTES_SHOE_OK;

// Variável global para acumular unidades totais (protegida por mutex)
extern double unidades_total_global;
extern pthread_mutex_t unidades_mutex;

#endif // CONSTANTES_H 
