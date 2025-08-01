#ifndef SIMULACAO_H
#define SIMULACAO_H

#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

void simulacao_completa(int log_level, int sim_id, const char* output_suffix, atomic_int* global_log_count, bool dealer_analysis, bool freq_analysis_26, bool freq_analysis_70, bool freq_analysis_A, bool split_analysis, bool ev_realtime_enabled, pthread_mutex_t* dealer_mutex, pthread_mutex_t* freq_mutex, pthread_mutex_t* split_mutex);

#endif // SIMULACAO_H 
