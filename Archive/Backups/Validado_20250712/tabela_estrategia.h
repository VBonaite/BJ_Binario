#ifndef TABELA_ESTRATEGIA_H
#define TABELA_ESTRATEGIA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ACAO_HIT = 0,
    ACAO_STAND,
    ACAO_DOUBLE,
    ACAO_SPLIT,
    ACAO_DOUBLE_OR_HIT,
    ACAO_DOUBLE_OR_STAND,
    ACAO_SPLIT_OR_HIT,
    ACAO_SPLIT_OR_STAND
} AcaoEstrategia;

// Estrutura para estratégia básica com chaves numéricas
typedef struct {
    uint32_t chave;        // Chave numérica (valor*100 + dealer_up)
    AcaoEstrategia acao;   // Ação da estratégia básica
} EstrategiaBasicaChave;

// Funções da estratégia básica (antigas - compatibilidade)
AcaoEstrategia estrategia_hard(int valor, int dealer_up);
AcaoEstrategia estrategia_soft(int valor, int dealer_up);
AcaoEstrategia estrategia_par(int par_rank, int dealer_up);

// Função da estratégia básica SUPER-OTIMIZADA (única função utilizada)
AcaoEstrategia estrategia_basica_super_rapida(uint64_t mao_bits, int dealer_up_rank);

// NOTA: buscar_estrategia_por_chave() foi removida pois não era utilizada

#ifdef __cplusplus
}
#endif

#endif // TABELA_ESTRATEGIA_H 