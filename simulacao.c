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
#include <sys/stat.h>
#include <errno.h>


static void adicionar_carta(uint64_t *mao, Carta c) {
    *mao += c; // incrementa o contador de 3 bits para o rank
}


void simulacao_completa(void) {
    // Criar diretório se não existir
    struct stat st = {0};
    if (stat(OUT_DIR, &st) == -1) {
        if (mkdir(OUT_DIR, 0755) != 0 && errno != EEXIST) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }
    
    // Abrir arquivo de log
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/log.csv", OUT_DIR);
    FILE *log_file = fopen(filepath, "w");
    if (!log_file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    
    // Escrever header
	fprintf(log_file, "Inicial,Upcard,Acoes,Final,Valor,DealerFinal,Resultado,Aposta,PNL,Double,Split,BJ_Jogador,BJ_Dealer\n");
    rng_init();
    
    int shoes_jogados = 0;
    double running_count = 0.0;
    double true_count = 0.0;

    while (shoes_jogados < NUM_SHOES) {
        Shoe shoe;
        baralho_criar(&shoe);
        baralho_embaralhar(&shoe);
        
        // Jogar até atingir a penetração
        size_t limite_penetracao = (size_t)(shoe.total * PENETRACAO);
        
        while (shoe.topo <= limite_penetracao) {

			
            // Uma rodada completa
            uint64_t *maos_bits = (uint64_t*)calloc(NUM_JOGADORES, sizeof(uint64_t));
            uint64_t dealer_mao = 0;
            double bet = UNIDADE;
            Carta dealer_hole_card = 0; // Guardar para contabilizar depois

            // Primeira rodada de distribuição
            for (int i = 0; i < NUM_JOGADORES; ++i) {
                Carta c = baralho_comprar(&shoe);
                adicionar_carta(&maos_bits[i], c);
				atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
            }
            
            // Dealer recebe upcard
            Carta c = baralho_comprar(&shoe);
            adicionar_carta(&dealer_mao, c);
            Carta dealer_upcard = c;
            atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);

            // Segunda rodada de distribuição
            for (int i = 0; i < NUM_JOGADORES; ++i) {
                c = baralho_comprar(&shoe);
                adicionar_carta(&maos_bits[i], c);
				atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
            }
            
            // Dealer recebe hole card
            c = baralho_comprar(&shoe);
            adicionar_carta(&dealer_mao, c);
			dealer_hole_card = c;
            
            // Calcular rank do upcard do dealer
            int dealer_up_rank;
#if defined(__GNUC__)
            dealer_up_rank = (__builtin_ctzll(dealer_upcard) / 3);
            if (dealer_up_rank <= 7) dealer_up_rank += 2;
            else if (dealer_up_rank == 8) dealer_up_rank = 10;
            else if (dealer_up_rank <= 11) dealer_up_rank = 10;
            else dealer_up_rank = 11;
#else
            dealer_up_rank = 10;
#endif
            
            Mao dealer_info;
            avaliar_mao(dealer_mao, &dealer_info);

            
            char dealer_final_str[32];            
            char upcard_char = carta_para_char(dealer_upcard);

            // Verificar se upcard é Ás
            if (dealer_up_rank == 11) {
                // Upcard é Ás - verificar blackjack primeiro
                if (dealer_info.blackjack) {
                    // Dealer tem BJ - contabilizar hole card
                    atualizar_counts(&running_count, &true_count, dealer_hole_card, shoe.total - shoe.topo);
                    
                    // Jogadores não jogam - apenas processar resultados
                    for (int pj = 0; pj < NUM_JOGADORES; ++pj) {
                        Mao mao_jogador;
                        avaliar_mao(maos_bits[pj], &mao_jogador);
                        mao_jogador.aposta = bet;
                        mao_jogador.finalizada = true; // Não jogam
                        
                        verificar_mao(&mao_jogador, &dealer_info);
                        calcular_pnl(&mao_jogador);
                        
                        char init_str[32];
                        char final_str[32];
                        mao_para_string(mao_jogador.initial_bits, init_str);
                        mao_para_string(mao_jogador.bits, final_str);
                        mao_para_string(dealer_info.bits, dealer_final_str);
                        
                        fprintf(log_file, "%s,%c,%s,%s,%d,%s,%c,%.1f,%.1f,%c,%c,%c,%c\n", 
                               init_str, upcard_char, 
                               "-", final_str, mao_jogador.valor, 
                               dealer_final_str, mao_jogador.resultado, mao_jogador.aposta, mao_jogador.pnl,
                               'N', 'N', mao_jogador.blackjack ? 'S' : 'N', 'S');
                    }
                    
                    free(maos_bits);
                    continue; // Próxima rodada
                }
            }


            
            // Processar cada jogador
            for (int pj = 0; pj < NUM_JOGADORES; ++pj) {
                Mao hands[10];
                int hand_count = 1;
                avaliar_mao(maos_bits[pj], &hands[0]);
                hands[0].aposta = bet;
                
                for (int h = 0; h < hand_count; ++h) {
                    Mao *nova = &hands[hand_count];
                    Mao *m = &hands[h];
                    Mao *split_result = jogar_mao(m, &shoe, dealer_up_rank, nova, &running_count);
                    if (split_result) {
                        split_result->aposta = bet;
                        hand_count++;
                    }
                }
                
                // Após todos jogadores jogarem, contabilizar hole card e dealer joga
                if (pj == NUM_JOGADORES - 1) {
                    // Contabilizar hole card
                    atualizar_counts(&running_count, &true_count, dealer_hole_card, shoe.total - shoe.topo);
                    
                    // Dealer joga sua mão
                    avaliar_mao_dealer(&dealer_info, &shoe, &running_count, &true_count);
                    mao_para_string(dealer_info.bits, dealer_final_str);
                }

                // Salvar resultado de cada mão no arquivo
                for (int h = 0; h < hand_count; ++h) {
                    Mao *m = &hands[h];
                    char init_str[32];
                    char final_str[32];
                    mao_para_string(m->initial_bits, init_str);
                    mao_para_string(m->bits, final_str);
                    
                    verificar_mao(m, &dealer_info);
                    calcular_pnl(m);

                    fprintf(log_file, "%s,%c,%s,%s,%d,%s,%c,%.1f,%.1f,%c,%c,%c,%c\n", 
                           init_str, upcard_char, 
                           (m->hist_len>0?m->historico:"-"), final_str, m->valor, 
                           dealer_final_str, m->resultado, m->aposta, m->pnl,
                           m->isdouble ? 'S' : 'N',
                           m->from_split ? 'S' : 'N',
                           m->blackjack ? 'S' : 'N',
                           dealer_info.blackjack ? 'S' : 'N');                    


                }
            }
            
            free(maos_bits);
        }
        
        baralho_destruir(&shoe);
        shoes_jogados++;
        running_count = 0.0

        // Opcional: imprimir progresso
        if (shoes_jogados % 10 == 0) {
            printf("Shoes jogados: %d/%d\n", shoes_jogados, NUM_SHOES);
            fflush(stdout);
        }
    }
    
    fclose(log_file);
    printf("Simulação completa. Resultados salvos em: %s/log.csv\n", OUT_DIR);
}

static void atualizar_counts(double *running_count, double *true_count, Carta c, size_t cartas_restantes) {
    *running_count += WONG_HALVES[carta_para_rank_idx(c)];
    int decks_restantes = (cartas_restantes + 26) / 52; // arredondar para cima
    if (decks_restantes < 1) decks_restantes = 1;
    *true_count = *running_count / decks_restantes;
}

void simulacao_uma_distribuicao(void) {
    rng_init();

    Shoe shoe;
    baralho_criar(&shoe);
    baralho_embaralhar(&shoe);

    // Debug simplificado: apenas header + resultados

    uint64_t *maos_bits = (uint64_t*)calloc(NUM_JOGADORES, sizeof(uint64_t));
    uint64_t dealer_mao = 0;
    double bet = UNIDADE;
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
		hands[0].aposta = bet;

        for (int h = 0; h < hand_count; ++h) {
            Mao *nova = &hands[hand_count];
            Mao *m = &hands[h];
            Mao *split_result = jogar_mao(m, &shoe, dealer_up_rank, nova);
            if (split_result) {
				split_result->aposta = bet;
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
			calcular_pnl(m);
			printf("%s %c %s %s %d %s %c %.1f %.1f\n", init_str, upcard_char, 
				(m->hist_len>0?m->historico:"-"), final_str, m->valor, 
				dealer_final_str, m->resultado, m->aposta, m->pnl);


        }
    }

    /* Dealer summary no longer required */

    baralho_destruir(&shoe);
    free(maos_bits);
} 
