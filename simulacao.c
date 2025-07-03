#include "simulacao.h"
#include "baralho.h"
#include "rng.h"
#include "constantes.h"
#include "jogo.h"
#include "saidas.h"
#include "tabela_estrategia.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static void adicionar_carta(uint64_t *mao, Carta c) {
    *mao += c; // incrementa o contador de 3 bits para o rank
}

void simulacao_uma_distribuicao(void) {
    rng_init();

    Shoe shoe;
    baralho_criar(&shoe);
    baralho_embaralhar(&shoe);

    // Debug simplificado: apenas header + resultados

    uint64_t *maos_bits = (uint64_t*)calloc(NUM_JOGADORES, sizeof(uint64_t));
    uint64_t dealer_mao = 0;

    // Primeira rodada de distribuição
    for (int i = 0; i < NUM_JOGADORES; ++i) {
        Carta c = baralho_comprar(&shoe);
        adicionar_carta(&maos_bits[i], c);
        /* printf("Distribuindo %c para Jogador %d\n", carta_para_char(c), i + 1); */
    }

    // Dealer recebe upcard
    Carta c = baralho_comprar(&shoe);
    adicionar_carta(&dealer_mao, c);
    /* printf("Distribuindo %c para Dealer (upcard)\n", carta_para_char(c)); */
    Carta dealer_upcard = c;

    // Segunda rodada de distribuição
    for (int i = 0; i < NUM_JOGADORES; ++i) {
        c = baralho_comprar(&shoe);
        adicionar_carta(&maos_bits[i], c);
        /* printf("Distribuindo %c para Jogador %d\n", carta_para_char(c), i + 1); */
    }

    // Dealer recebe hole card
    c = baralho_comprar(&shoe);
    adicionar_carta(&dealer_mao, c);
    /* printf("Distribuindo %c para Dealer (hole)\n\n", carta_para_char(c)); */

    int dealer_up_rank;
#if defined(__GNUC__)
    dealer_up_rank = (__builtin_ctzll(dealer_upcard) / 3);
    if (dealer_up_rank <= 7) dealer_up_rank += 2;
    else if (dealer_up_rank == 8) dealer_up_rank = 10;
    else if (dealer_up_rank <= 11) dealer_up_rank = 10;
    else dealer_up_rank = 11;
#else
    // fallback simple method using function in jogo
    dealer_up_rank = 10; // placeholder (should not compile path generally)
#endif

    Mao dealer_info;
    avaliar_mao(dealer_mao, &dealer_info);
    avaliar_mao_dealer(&dealer_info, &shoe);

    char dealer_final_str[32];
    mao_para_string(dealer_info.bits, dealer_final_str);

    char upcard_char = carta_para_char(dealer_upcard);

    printf("Inicial Upcard Acoes Final Valor DealerFinal Resultado\n");

    // Processar cada jogador
    for (int pj = 0; pj < NUM_JOGADORES; ++pj) {
        Mao hands[10];
        int hand_count = 1;
        avaliar_mao(maos_bits[pj], &hands[0]);

        for (int h = 0; h < hand_count; ++h) {
            Mao *nova = &hands[hand_count];
            Mao *m = &hands[h];
            Mao *split_result = jogar_mao(m, &shoe, dealer_up_rank, nova);
            if (split_result) {
                hand_count++;
            }
        }

        // imprimir resultado de cada mão deste jogador
        for (int h = 0; h < hand_count; ++h) {
            Mao *m = &hands[h];
            char init_str[32];
            char final_str[32];
            mao_para_string(m->initial_bits, init_str);
            mao_para_string(m->bits, final_str);

            verificar_mao(m, &dealer_info);
            printf("%s %c %s %s %d %s %c\n", init_str, upcard_char, (m->hist_len>0?m->historico:"-"), final_str, m->valor, dealer_final_str, m->resultado);
        }
    }

    /* Dealer summary no longer required */

    baralho_destruir(&shoe);
    free(maos_bits);
} 