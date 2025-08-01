#include "otimizacoes.h"
#include "tabela_estrategia.h"
#include <stdint.h>

// Tabelas de lookup pré-calculadas para estratégia básica
static AcaoEstrategia hard_table[22][11];   // [valor][dealer_up]
static AcaoEstrategia soft_table[11][11];   // [valor-11][dealer_up] 
static AcaoEstrategia pair_table[11][11];   // [pair_value][dealer_up]
static bool tabelas_inicializadas = false;

// Cache simples para mãos (últimas 16 calculadas)
#define CACHE_SIZE 16
static CacheMao cache_maos[CACHE_SIZE];
static int cache_index = 0;

// ========== INICIALIZAÇÃO ==========

void init_otimizacoes() {
    if (tabelas_inicializadas) return;
    
    // Inicializar tabelas de estratégia básica com chaves numéricas
    for (int valor = 5; valor <= 21; valor++) {
        for (int dealer = 2; dealer <= 11; dealer++) {
            hard_table[valor][dealer-2] = estrategia_hard(valor, dealer);
        }
    }
    
    for (int soft_val = 12; soft_val <= 21; soft_val++) {
        for (int dealer = 2; dealer <= 11; dealer++) {
            soft_table[soft_val-12][dealer-2] = estrategia_soft(soft_val, dealer);
        }
    }
    
    for (int pair_val = 2; pair_val <= 11; pair_val++) {
        for (int dealer = 2; dealer <= 11; dealer++) {
            pair_table[pair_val-2][dealer-2] = estrategia_par(pair_val, dealer);
        }
    }
    
    // Limpar cache
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_maos[i].valid = false;
    }
    
    tabelas_inicializadas = true;
}

void clear_cache_otimizacoes() {
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache_maos[i].valid = false;
    }
    cache_index = 0;
}

// ========== CÁLCULOS OTIMIZADOS ==========

int calcular_valor_mao_fast(uint64_t mao) {
    // Verificar cache primeiro
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache_maos[i].valid && cache_maos[i].mao_bits == mao) {
            return cache_maos[i].valor;
        }
    }
    
    int valor = 0;
    int ases = 0;
    
    // Otimização: verificar se há Ás primeiro
    if (has_ace(mao)) {
        ases = (mao & MASK_ACE) >> 36;  // Extrair count de Ases diretamente
    }
    
    // Calcular valor das outras cartas
    // Usar unroll para cartas 2-9 (mais comuns)
    valor += ((mao >> 0) & 0x7ULL) * 2;   // 2s
    valor += ((mao >> 3) & 0x7ULL) * 3;   // 3s
    valor += ((mao >> 6) & 0x7ULL) * 4;   // 4s
    valor += ((mao >> 9) & 0x7ULL) * 5;   // 5s
    valor += ((mao >> 12) & 0x7ULL) * 6;  // 6s
    valor += ((mao >> 15) & 0x7ULL) * 7;  // 7s
    valor += ((mao >> 18) & 0x7ULL) * 8;  // 8s
    valor += ((mao >> 21) & 0x7ULL) * 9;  // 9s
    
    // Cartas de valor 10
    if (has_ten_value(mao)) {
        valor += ((mao >> 24) & 0x7ULL) * 10; // 10s
        valor += ((mao >> 27) & 0x7ULL) * 10; // Js
        valor += ((mao >> 30) & 0x7ULL) * 10; // Qs  
        valor += ((mao >> 33) & 0x7ULL) * 10; // Ks
    }
    
    // Processar Ases
    valor += ases * 11;
    while (valor > 21 && ases > 0) {
        valor -= 10;
        --ases;
    }
    
    // Armazenar no cache
    cache_maos[cache_index].mao_bits = mao;
    cache_maos[cache_index].valor = valor;
    cache_maos[cache_index].valid = true;
    cache_index = (cache_index + 1) % CACHE_SIZE;
    
    return valor;
}

TipoMao tipo_mao_fast(uint64_t mao) {
    // Contar total de cartas rapidamente
    uint64_t temp = mao;
    int total_cartas = 0;
    
    // Contar cartas usando bit manipulation
    for (int rank = 0; rank < 13; rank++) {
        total_cartas += (temp & 0x7ULL);
        temp >>= 3;
    }
    
    // Blackjack: exatamente 2 cartas, valor 21
    if (total_cartas == 2) {
        if (is_blackjack_fast(mao)) {
            return MAO_BLACKJACK;
        }
        if (is_pair_fast(mao)) {
            return MAO_PAR;
        }
    }
    
    // Verificar se é soft (tem Ás contando como 11)
    if (has_ace(mao)) {
        int valor = calcular_valor_mao_fast(mao);
        int ases = (mao & MASK_ACE) >> 36;
        
        // Calcular valor sem Ases
        int valor_sem_ases = valor - ases * 11;
        if (valor_sem_ases + ases <= 10) { // Ás pode contar como 11
            return MAO_SOFT;
        }
    }
    
    return MAO_HARD;
}

bool is_blackjack_considerando_split(uint64_t mao, bool from_split) {
    if (from_split) return false;
    return is_blackjack_fast(mao);
}

// ========== ESTRATÉGIA OTIMIZADA ==========

AcaoEstrategia estrategia_hard_rapida(int valor, int dealer_up) {
    if (valor < 5 || valor > 21 || dealer_up < 2 || dealer_up > 11) {
        return ACAO_HIT; // fallback
    }
    return hard_table[valor][dealer_up-2];
}

AcaoEstrategia estrategia_soft_rapida(int valor, int dealer_up) {
    if (valor < 12 || valor > 21 || dealer_up < 2 || dealer_up > 11) {
        return ACAO_HIT; // fallback
    }
    return soft_table[valor-12][dealer_up-2];
}

AcaoEstrategia estrategia_par_rapida(int par_value, int dealer_up) {
    if (par_value < 2 || par_value > 11 || dealer_up < 2 || dealer_up > 11) {
        return ACAO_HIT; // fallback
    }
    return pair_table[par_value-2][dealer_up-2];
}

AcaoEstrategia determinar_acao_rapida(uint64_t mao_bits, uint64_t dealer_upcard, 
                                     double true_count, bool disable_deviations) {
    
    // Verificar se é blackjack
    if (is_blackjack_fast(mao_bits)) {
        return ACAO_STAND;
    }
    
    // Conversão rápida do dealer upcard
    int dealer_up_rank = upcard_to_rank_fast(dealer_upcard);
    
    // Verificar desvios primeiro (se ativos)
    if (!disable_deviations) {
        AcaoEstrategia desvio = verificar_desvio(mao_bits, dealer_up_rank, true_count);
        if (desvio != (AcaoEstrategia)-1) {
            return desvio;
        }
    }
    
    // Determinar tipo de mão e usar estratégia básica otimizada
    TipoMao tipo = tipo_mao_fast(mao_bits);
    int valor = calcular_valor_mao_fast(mao_bits);
    
    switch (tipo) {
        case MAO_PAR: {
            // Encontrar valor do par rapidamente
            for (int rank = 0; rank < 13; rank++) {
                uint64_t count = (mao_bits >> (rank * 3)) & 0x7ULL;
                if (count == 2) {
                    int par_val = (rank <= 7) ? (rank + 2) : 
                                 (rank <= 11) ? 10 : 11;
                    return estrategia_par_rapida(par_val, dealer_up_rank);
                }
            }
            return ACAO_HIT; // fallback
        }
        case MAO_SOFT:
            return estrategia_soft_rapida(valor, dealer_up_rank);
            
        case MAO_HARD:
        default:
            return estrategia_hard_rapida(valor, dealer_up_rank);
    }
}

// ========== FUNÇÕES DE AVALIAÇÃO OTIMIZADAS ==========

void avaliar_mao_fast(uint64_t bits, Mao *out) {
    out->bits = bits;
    out->initial_bits = bits;
    out->valor = calcular_valor_mao_fast(bits);
    out->tipo = tipo_mao_fast(bits);
    out->finalizada = false;
    out->from_split = false;
    out->isdouble = false;
    out->contabilizada = false;
    out->blackjack = is_blackjack_fast(bits);
    out->aposta = 0.0;
    out->pnl = 0.0;
    out->hist_len = 0;
    out->historico[0] = '\0';
    out->resultado = '?';
} 