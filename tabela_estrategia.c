#include "tabela_estrategia.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define H  ACAO_HIT
#define S  ACAO_STAND
#define DH ACAO_DOUBLE_OR_HIT
#define DS ACAO_DOUBLE_OR_STAND
#define PH ACAO_SPLIT_OR_HIT
#define PS ACAO_SPLIT_OR_STAND
#define D  ACAO_DOUBLE
#define P  ACAO_SPLIT

// Funções de estratégia básica (mantidas para compatibilidade com otimizacoes.c)
AcaoEstrategia estrategia_hard(int total, int upcard) {    
    // Tabela hard totals 3-21 inline para evitar duplicação
    static const AcaoEstrategia hard_table[19][10] = {
        /*3*/ { H,H,H,H,H,H,H,H,H,H },
        /*4*/ { H,H,H,H,H,H,H,H,H,H },
        /*5*/ { H,H,H,H,H,H,H,H,H,H },
        /*6*/ { H,H,H,H,H,H,H,H,H,H },
        /*7*/ { H,H,H,H,H,H,H,H,H,H },
        /*8*/ { H,H,H,H,H,H,H,H,H,H },
        /*9*/ { H,H,DH,DH,DH,H,H,H,H,H },
        /*10*/{ DH,DH,DH,DH,DH,DH,DH,DH,H,H },
        /*11*/{ DH,DH,DH,DH,DH,DH,DH,DH,DH,DH },
        /*12*/{ H,H,S,S,S,H,H,H,H,H },
        /*13*/{ S,S,S,S,S,H,H,H,H,H },
        /*14*/{ S,S,S,S,S,H,H,H,H,H },
        /*15*/{ S,S,S,S,S,H,H,H,H,H },
        /*16*/{ S,S,S,S,S,H,H,H,H,H },
        /*17*/{ S,S,S,S,S,S,S,S,S,S },
        /*18*/{ S,S,S,S,S,S,S,S,S,S },
        /*19*/{ S,S,S,S,S,S,S,S,S,S },
        /*20*/{ S,S,S,S,S,S,S,S,S,S },
        /*21*/{ S,S,S,S,S,S,S,S,S,S }
    };
    return hard_table[total - 3][upcard - 2];
}

AcaoEstrategia estrategia_soft(int total, int upcard) {
    
    // Tabela soft totals 13-21 inline para evitar duplicação
    static const AcaoEstrategia soft_table[9][10] = {
        /*A,2*/ { H,H,H,DH,DH,H,H,H,H,H },
        /*A,3*/ { H,H,H,DH,DH,H,H,H,H,H },
        /*A,4*/ { H,H,DH,DH,DH,H,H,H,H,H },
        /*A,5*/ { H,H,DH,DH,DH,H,H,H,H,H },
        /*A,6*/ { H,DH,DH,DH,DH,H,H,H,H,H },
        /*A,7*/ { S,DS,DS,DS,DS,S,S,H,H,H },
        /*A,8*/ { S,S,S,S,S,S,S,S,S,S },
        /*A,9*/ { S,S,S,S,S,S,S,S,S,S },
        /*A,10*/{ S,S,S,S,S,S,S,S,S,S }
    };
    return soft_table[total - 13][upcard - 2];
}

AcaoEstrategia estrategia_par(int par_rank, int upcard) {
    
    // Tabela de pares 2-A inline para evitar duplicação
    static const AcaoEstrategia pair_table[10][10] = {
        /*2,2*/ { H,H,PH,PH,PH,PH,H,H,H,H },
        /*3,3*/ { H,H,PH,PH,PH,PH,H,H,H,H },
        /*4,4*/ { H,H,H,H,H,H,H,H,H,H },
        /*5,5*/ { DH,DH,DH,DH,DH,DH,DH,DH,H,H },
        /*6,6*/ { H,PS,PS,PS,PS,H,H,H,H,H },
        /*7,7*/ { PS,PS,PS,PS,PS,PH,H,H,H,H },
        /*8,8*/ { PS,PS,PS,PS,PS,PH,PH,PH,PH,PH },
        /*9,9*/ { PS,PS,PS,PS,PS,S,PS,PS,S,S },
        /*10,10*/{ S,S,S,S,S,S,S,S,S,S },
        /*A,A*/{ PS,PS,PS,PS,PS,PS,PS,PS,PS,PS }
    };
    return pair_table[par_rank - 2][upcard - 2];
}

// AcaoEstrategia estrategia_par(int par_rank, int upcard) {
    
//     // Tabela de pares 2-A inline para evitar duplicação
//     static const AcaoEstrategia pair_table[10][10] = {
//         /*2,2*/ { PH,PH,PH,PH,PH,PH,PH,PH,PH,PH },
//         /*3,3*/ { PH,PH,PH,PH,PH,PH,PH,PH,PH,PH },
//         /*4,4*/ { PH,PH,PH,PH,PH,PH,PH,PH,PH,PH},
//         /*5,5*/ { PH,PH,PH,PH,PH,PH,PH,PH,PH,PH },
//         /*6,6*/ { PH,PS,PS,PS,PS,PH,PH,PH,PH,PH },
//         /*7,7*/ { PS,PS,PS,PS,PS,PH,PH,PH,PH,PH },
//         /*8,8*/ {  PS,PS,PS,PS,PS,PH,PH,PH,PH,PH },
//         /*9,9*/ {  PS,PS,PS,PS,PS,PS,PS,PS,PS,PS },
//         /*10,10*/{ PS,PS,PS,PS,PS,PS,PS,PS,PS,PS },
//         /*A,A*/{ PS,PS,PS,PS,PS,PS,PS,PS,PS,PS }
//     };
//     return pair_table[par_rank - 2][upcard - 2];
// }
// ========== TABELA DE CHAVES REMOVIDA ==========
// A tabela estrategia_basica_chaves[] foi removida pois não era utilizada.
// O sistema usa diretamente as funções estrategia_hard/soft/par() com tabelas inline.
// 
// Para modificar a estratégia básica, edite as tabelas nas funções:
// - estrategia_hard() para mãos hard
// - estrategia_soft() para mãos soft  
// - estrategia_par() para pares
//
// NOTA: Função buscar_estrategia_por_chave() removida pois não era chamada.

// Função SUPER-OTIMIZADA com acesso direto (sem busca linear)
AcaoEstrategia estrategia_basica_super_rapida(uint64_t mao_bits, int dealer_up_rank) {
    // Versão super-otimizada da estratégia básica
    // Usa lookups diretos eliminando conversões desnecessárias
    
    // Calcular valor, tipo e rank simultaneamente para máxima eficiência
    int valor = 0;
    int ases = 0;
    int total_cartas = 0;
    int pair_rank = -1;
    
    // Loop único para calcular tudo
    for (int idx = 0; idx < 13; ++idx) {
        uint64_t count = (mao_bits >> (idx * 3)) & 0x7ULL;
        if (count == 0) continue;
        
        total_cartas += (int)count;
        
        if (idx <= 7) { // 2 a 9
            valor += (idx + 2) * count;
            if (count == 2 && total_cartas == 2) {
                pair_rank = idx + 2;
            }
        } else if (idx <= 11) { // 10, J, Q, K
            valor += 10 * count;
            if (count == 2 && total_cartas == 2) {
                pair_rank = 10;
            }
        } else { // Ás
            ases += (int)count;
            if (count == 2 && total_cartas == 2) {
                pair_rank = 11;
            }
        }
    }
    
    // Ajustar valor dos ases
    valor += ases * 11;
    int ases_soft = ases;
    while (valor > 21 && ases_soft > 0) {
        valor -= 10;
        --ases_soft;
    }
    
    // Dealer up ajustado para índice (2-11 -> 0-9)
    int dealer_idx = dealer_up_rank - 2;
    if (dealer_idx < 0 || dealer_idx > 9) return ACAO_HIT;
    
    // Lookups diretos por tipo
    if (pair_rank != -1 && total_cartas == 2) {
        // Mão par
        if (pair_rank < 2 || pair_rank > 11) return ACAO_HIT;
        return estrategia_par(pair_rank, dealer_up_rank);
    } else if (ases_soft > 0 && total_cartas == 2) {
        // Mão soft
        if (valor < 13 || valor > 21) return ACAO_HIT;
        return estrategia_soft(valor, dealer_up_rank);
    } else {
        // Mão hard
        if (valor < 3 || valor > 21) return ACAO_HIT;
        return estrategia_hard(valor, dealer_up_rank);
    }
}

// As funções estrategia_basica_pura_binaria e tabelas relacionadas foram removidas
// pois não estão sendo utilizadas no sistema atual 