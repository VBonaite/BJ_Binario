#include "baralho.h"
#include "constantes.h"
#include "rng.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static bool verificar_total_cartas(Shoe *shoe) {
    size_t esperado = (size_t)DECKS * 52;
    if (shoe->total != esperado) {
        fprintf(stderr, "FALHA: Total de cartas esperado %zu, obtido %zu\n", esperado, shoe->total);
        return false;
    }
    return true;
}

static bool verificar_distribuicao_por_rank(Shoe *shoe) {
    int contagem[13] = {0};
    for (size_t i = 0; i < shoe->total; ++i) {
        int idx = carta_para_rank_idx(shoe->cartas[i]);
        if (idx < 0 || idx >= 13) {
            fprintf(stderr, "FALHA: Rank idx fora do intervalo: %d\n", idx);
            return false;
        }
        contagem[idx]++;
    }
    bool ok = true;
    int esperado_por_rank = 4 * DECKS; // 4 cartas de cada rank por baralho (naipes irrelevantes)
    for (int rank = 0; rank < 13; ++rank) {
        if (contagem[rank] != esperado_por_rank) {
            fprintf(stderr, "FALHA: Rank %d esperado %d, obtido %d\n", rank, esperado_por_rank, contagem[rank]);
            ok = false;
        }
    }
    return ok;
}

int main(void) {
    rng_init();
    Shoe shoe;
    baralho_criar(&shoe);

    bool total_ok = verificar_total_cartas(&shoe);
    bool dist_ok = verificar_distribuicao_por_rank(&shoe);

    if (total_ok && dist_ok) {
        printf("✓ Teste de integridade do baralho passou. Total e distribuição corretos.\n");
        baralho_destruir(&shoe);
        return 0;
    } else {
        printf("✗ Teste de integridade do baralho falhou.\n");
        baralho_destruir(&shoe);
        return 1;
    }
} 