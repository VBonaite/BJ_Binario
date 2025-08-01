#ifndef OTIMIZACOES_H
#define OTIMIZACOES_H

#include <stdint.h>
#include <stdbool.h>
#include "jogo.h"

// Máscaras binárias para operações otimizadas
// Cada carta ocupa 3 bits: posições 0-38 (13 ranks * 3 bits)
// Ranks: 2=0, 3=1, 4=2, 5=3, 6=4, 7=5, 8=6, 9=7, 10=8, J=9, Q=10, K=11, A=12

// Máscaras para verificações diretas
#define MASK_ACE         0x1C0000000ULL  // Bits 36-38: Ás (rank 12)
#define MASK_TEN_VALUE   0x3F000000ULL   // Bits 24-35: 10,J,Q,K (ranks 8-11)
#define MASK_TEN_ONLY    0x7000000ULL    // Bits 24-26: apenas 10 (rank 8)
#define MASK_FIGURES     0x38000000ULL   // Bits 27-35: J,Q,K (ranks 9-11)
#define MASK_ALL_TENS    0x3F000000ULL   // Todos valores 10: 10,J,Q,K

// Verificações rápidas de blackjack
#define MASK_BLACKJACK_CHECK (MASK_ACE | MASK_TEN_VALUE)

// ========== FUNÇÕES OTIMIZADAS ==========

// Verificações diretas usando operações binárias
static inline bool is_ace_card(uint64_t card) {
    return (card & MASK_ACE) != 0;
}

static inline bool has_ace(uint64_t mao) {
    return (mao & MASK_ACE) != 0;
}

static inline bool has_ten_value(uint64_t mao) {
    return (mao & MASK_TEN_VALUE) != 0;
}

static inline bool is_blackjack_fast(uint64_t mao) {
    // Verifica se tem exatamente 2 cartas, 1 Ás e 1 carta de valor 10
    if (__builtin_popcountll(mao) != 2) return false; // Exatamente 2 cartas
    return has_ace(mao) && has_ten_value(mao);
}

static inline bool is_ace_upcard(uint64_t upcard) {
    return is_ace_card(upcard);
}

// Conversão rápida de upcard para rank (para tabelas)
static inline int upcard_to_rank_fast(uint64_t upcard) {
    if (upcard & MASK_ACE) return 11;           // Ás
    if (upcard & MASK_TEN_VALUE) return 10;     // 10, J, Q, K
    
    // Para 2-9, encontrar posição do bit
    int pos = __builtin_ctzll(upcard) / 3;
    return pos + 2;  // ranks 0-7 correspondem a valores 2-9
}

// Verificação rápida de par
static inline bool is_pair_fast(uint64_t mao) {
    if (__builtin_popcountll(mao) != 2) return false;
    
    // Verificar se ambas as cartas são do mesmo rank
    for (int rank = 0; rank < 13; rank++) {
        uint64_t mask = 0x7ULL << (rank * 3);
        uint64_t count = (mao & mask) >> (rank * 3);
        if (count == 2) return true;
    }
    return false;
}

// Cálculo rápido do valor da mão (otimizado)
int calcular_valor_mao_fast(uint64_t mao);

// Determinação rápida do tipo de mão
TipoMao tipo_mao_fast(uint64_t mao);

// Verificação rápida de blackjack considerando split
bool is_blackjack_considerando_split(uint64_t mao, bool from_split);

// ========== ESTRATÉGIA COM CHAVES NUMÉRICAS ==========

// Chaves para estratégia básica (similar aos desvios)
typedef struct {
    uint32_t chave;
    AcaoEstrategia acao;
} EntradaEstrategiaRapida;

// Funções para estratégia com chaves numéricas
AcaoEstrategia estrategia_hard_rapida(int valor, int dealer_up);
AcaoEstrategia estrategia_soft_rapida(int valor, int dealer_up);  
AcaoEstrategia estrategia_par_rapida(int par_value, int dealer_up);

// Função principal de estratégia otimizada
AcaoEstrategia determinar_acao_rapida(uint64_t mao_bits, uint64_t dealer_upcard);

// ========== OTIMIZAÇÕES GERAIS ==========

// Cache para valores frequentemente calculados
typedef struct {
    uint64_t mao_bits;
    int valor;
    TipoMao tipo;
    bool blackjack;
    bool valid;
} CacheMao;

// Inicializar sistema de otimizações
void init_otimizacoes();

// Limpar cache se necessário
void clear_cache_otimizacoes();

// Função de avaliação otimizada
void avaliar_mao_fast(uint64_t bits, Mao *out);

#endif // OTIMIZACOES_H 