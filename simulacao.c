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

static void atualizar_counts(double *running_count, double *true_count, Carta c, size_t cartas_restantes, FILE *count_file, int shoe_num) {
    *running_count += WONG_HALVES[carta_para_rank_idx(c)];
    int decks_restantes = (cartas_restantes + 26) / 52; // arredondar para cima
    if (decks_restantes < 1) decks_restantes = 1;
    *true_count = *running_count / decks_restantes;
	// Salvar no arquivo apenas para o primeiro shoe
    if (count_file && shoe_num == 0) {
        char carta_char = carta_para_char(c);
        fprintf(count_file, "%c,%.1f,%.2f,%d\n", carta_char, *running_count, *true_count, decks_restantes);
    }
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
    

	// Abrir arquivo de verificação de count (apenas para o primeiro shoe)
    char count_filepath[512];
    snprintf(count_filepath, sizeof(count_filepath), "%s/count_check.csv", OUT_DIR);
    FILE *count_file = fopen(count_filepath, "w");
    if (!count_file) {
        perror("fopen count_check");
        exit(EXIT_FAILURE);
    }
    fprintf(count_file, "carta,running_count,true_count,decks_remaining\n");


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
                atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo, count_file, shoes_jogados);
            }
            
            // Dealer recebe upcard
            Carta c = baralho_comprar(&shoe);
            adicionar_carta(&dealer_mao, c);
            Carta dealer_upcard = c;
            atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo, count_file, shoes_jogados);

            // Segunda rodada de distribuição
            for (int i = 0; i < NUM_JOGADORES; ++i) {
                c = baralho_comprar(&shoe);
                adicionar_carta(&maos_bits[i], c);
                atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo, count_file, shoes_jogados);
            }
            
            // Dealer recebe hole card (NÃO CONTABILIZAR AINDA)
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
            avaliar_mao_dealer(&dealer_info, &shoe, &running_count, &true_count, count_file, shoes_jogados);
            
            char dealer_final_str[32];
            dealer_final_str[0] = '\0'; // Inicializar para evitar lixo
            
            char upcard_char = carta_para_char(dealer_upcard);

            // Verificar se upcard é Ás
            if (dealer_up_rank == 11) {
                // Upcard é Ás - verificar blackjack primeiro
                if (dealer_info.blackjack) {
                    // Dealer tem BJ - contabilizar hole card
                    atualizar_counts(&running_count, &true_count, dealer_hole_card, shoe.total - shoe.topo, count_file, shoes_jogados);
                    
                    // Preencher string do dealer ANTES de usar
                    mao_para_string(dealer_info.bits, dealer_final_str);
                    
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

            // Se chegou aqui: ou upcard não é Ás, ou é Ás mas dealer não tem BJ
            // Primeiro, todos os jogadores jogam
            Mao all_hands[70]; // máximo 7 jogadores x 10 mãos cada
            int total_hands = 0;
            
            for (int pj = 0; pj < NUM_JOGADORES; ++pj) {
                Mao hands[10];
                int hand_count = 1;
                avaliar_mao(maos_bits[pj], &hands[0]);
                hands[0].aposta = bet;
                
                for (int h = 0; h < hand_count; ++h) {
                    Mao *nova = &hands[hand_count];
                    Mao *m = &hands[h];
                    Mao *split_result = jogar_mao(m, &shoe, dealer_up_rank, nova, &running_count, &true_count, count_file, shoes_jogados);
                    if (split_result) {
                        split_result->aposta = bet;
                        hand_count++;
                    }
                }
                
                // Copiar todas as mãos para o array global
                for (int h = 0; h < hand_count; ++h) {
                    all_hands[total_hands++] = hands[h];
                }
            }
            
            // Agora que todos jogaram, dealer conta hole card e joga
            atualizar_counts(&running_count, &true_count, dealer_hole_card, shoe.total - shoe.topo, count_file, shoes_jogados);
            avaliar_mao_dealer(&dealer_info, &shoe, &running_count, &true_count, count_file, shoes_jogados);
            mao_para_string(dealer_info.bits, dealer_final_str);
            
            // Agora salvar todos os resultados
            for (int i = 0; i < total_hands; ++i) {
                Mao *m = &all_hands[i];
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
            
            free(maos_bits);
        }
        
        baralho_destruir(&shoe);
        shoes_jogados++;
        running_count = 0.0; // Reset para novo shoe
        true_count = 0.0;    // Reset true count também

        // Opcional: imprimir progresso
        if (shoes_jogados % 10 == 0) {
            printf("Shoes jogados: %d/%d\n", shoes_jogados, NUM_SHOES);
            fflush(stdout);
        }
    }
    
    fclose(log_file);
	fclose(count_file);
    printf("Simulação completa. Resultados salvos em: %s/log.csv\n", OUT_DIR);
    printf("Verificação de count salva em: %s/count_check.csv\n", OUT_DIR);
}
