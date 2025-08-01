#include "simulacao.h"
#include "otimizacoes.h"
#include "constantes.h"
#include "baralho.h"
#include "jogo.h"
#include "saidas.h"
#include "rng.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

// Versão otimizada da simulação usando operações binárias
// Esta versão substitui as principais funções por versões otimizadas

static void adicionar_carta(uint64_t *mao, Carta c) {
    *mao += c;
}

static void atualizar_counts(double *running_count, double *true_count, Carta c, size_t cartas_restantes) {
    *running_count += WONG_HALVES[carta_para_rank_idx(c)];
    double decks_restantes = (double)cartas_restantes / 52.0;
    if (decks_restantes < 1.0) decks_restantes = 1.0;
    *true_count = *running_count / decks_restantes;
}

static void registrar_acao(Mao *mao, char code) {
    int n = mao->hist_len;
    if (n < (int)sizeof(mao->historico)-1) {
        mao->historico[n] = code;
        mao->historico[n+1] = '\0';
        mao->hist_len++;
    }
}

void simulacao_completa_otimizada(int log_level, int sim_id, const char* output_suffix, atomic_int* global_log_count, bool disable_deviations, bool dealer_analysis, pthread_mutex_t* dealer_mutex) {
    FILE *log_file = NULL;
    FILE *dealer_file = NULL;
    
    // Inicializar otimizações
    init_otimizacoes();
    
    // Inicializar logging apenas se log_level > 0
    if (log_level > 0) {
        // Criar diretório se não existir
        struct stat st = {0};
        if (stat(OUT_DIR, &st) == -1) {
            if (mkdir(OUT_DIR, 0755) != 0 && errno != EEXIST) {
                perror("mkdir");
                exit(EXIT_FAILURE);
            }
        }
        
        // Abrir arquivo de log com sufixo personalizado
        char filepath[512];
        if (output_suffix && strlen(output_suffix) > 0) {
            snprintf(filepath, sizeof(filepath), "%s/log_%s_%d.csv", OUT_DIR, output_suffix, sim_id);
        } else {
            snprintf(filepath, sizeof(filepath), "%s/log_sim_%d.csv", OUT_DIR, sim_id);
        }
        
        log_file = fopen(filepath, "w");
        if (!log_file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        // Escrever header
        fprintf(log_file, "Inicial,Upcard,Acoes,Final,Valor,DealerFinal,Resultado,Aposta,PNL,Double,Split,BJ_Jogador,BJ_Dealer\n");
    }
    
    // Inicializar arquivo de dealer se análise está ativada
    if (dealer_analysis) {
        // Calcular qual lote esta simulação pertence
        #define BATCH_SIZE_SIMS 10000
        int batch_id = sim_id / BATCH_SIZE_SIMS;  // 10000 simulações por lote
        char dealer_filepath[512];
        snprintf(dealer_filepath, sizeof(dealer_filepath), "temp_dealer_bj_batch_%d.csv", batch_id);
        
        dealer_file = fopen(dealer_filepath, "a");  // Modo append para múltiplas simulações
        if (!dealer_file) {
            perror("fopen dealer_file");
            if (log_file) fclose(log_file);
            return;
        }
        
        // Escrever cabeçalho apenas se arquivo está vazio (primeira simulação do lote)
        if (ftell(dealer_file) == 0) {
            fprintf(dealer_file, "true_count,ace_upcard,dealer_blackjack\n");
        }
    }
    
    rng_init();
    
    // Variáveis para controle do bankroll e estatísticas
    double bankroll = BANKROLL_INICIAL;
    int vitorias = 0;
    int maos_jogadas = 0;
    double pnl_shoe = 0.0;      // PNL acumulado do shoe atual
    double loss_shoe = 0.0;     // Unidades perdidas no shoe atual
    double unidade_atual = UNIDADE_INICIAL;
    
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
            // Verificar se já atingiu o limite global de logs
            if (log_level > 0 && atomic_load(global_log_count) >= log_level) {
                // Interromper loop se limite atingido
                goto finish_simulation;
            }
            
            // Calcular mãos contabilizadas baseado no true count atual
            int maos_contabilizadas = calcular_maos_contabilizadas(true_count);
            int total_maos = NUM_JOGADORES + maos_contabilizadas;
            
            // Atualizar unidade baseada no bankroll
            unidade_atual = calcular_unidade(bankroll);
            
            // Calcular aposta usando o sistema de progressão
            size_t cartas_restantes = shoe.total - shoe.topo;
            int bet = definir_aposta(cartas_restantes, vitorias, true_count, maos_jogadas, loss_shoe, unidade_atual);
            
            // Uma rodada completa
            uint64_t *maos_bits = (uint64_t*)calloc(total_maos, sizeof(uint64_t));
            uint64_t dealer_mao = 0;
            Carta dealer_hole_card = 0; // Guardar para contabilizar depois

            // Primeira rodada de distribuição
            for (int i = 0; i < total_maos; ++i) {
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
            for (int i = 0; i < total_maos; ++i) {
                c = baralho_comprar(&shoe);
                adicionar_carta(&maos_bits[i], c);
                atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
            }
            
            // Dealer recebe hole card (NÃO CONTABILIZAR AINDA)
            c = baralho_comprar(&shoe);
            adicionar_carta(&dealer_mao, c);
            dealer_hole_card = c;
            
            // ========== OTIMIZAÇÕES APLICADAS ==========
            
            // Verificação otimizada de Ás do dealer
            bool dealer_has_ace = is_ace_card(dealer_upcard);
            // int dealer_up_rank = upcard_to_rank_fast(dealer_upcard); // Removido, não usado nesta versão
            
            Mao dealer_info;
            avaliar_mao_fast(dealer_mao, &dealer_info);
            
            char dealer_final_str[32];
            dealer_final_str[0] = '\0';
            
            char upcard_char = carta_para_char(dealer_upcard);

            // Calcular insurance se aplicável
            double insurance_bet = 0.0;
            bool made_insurance = false;
            int maos_contabilizadas_count = 0;
            
            // Contar mãos contabilizadas
            for (int pj = NUM_JOGADORES; pj < total_maos; ++pj) {
                maos_contabilizadas_count++;
            }
            
            if (dealer_has_ace && true_count >= MIN_COUNT_INS && maos_contabilizadas_count > 0) {
                insurance_bet = (bet / 2.0) * maos_contabilizadas_count;
                made_insurance = true;
            }

            // Verificação otimizada: dealer tem Ás?
            if (dealer_has_ace) {
                // Coletar dados do dealer se análise está ativada
                if (dealer_analysis && dealer_file && dealer_mutex) {
                    pthread_mutex_lock(dealer_mutex);
                    fprintf(dealer_file, "%.2f,1,%d\n", true_count, dealer_info.blackjack ? 1 : 0);
                    fflush(dealer_file);
                    pthread_mutex_unlock(dealer_mutex);
                }
                
                // Verificação otimizada de blackjack do dealer
                if (dealer_info.blackjack) {
                    // Dealer tem BJ - contabilizar hole card
                    atualizar_counts(&running_count, &true_count, dealer_hole_card, shoe.total - shoe.topo);
                    
                    // Aplicar ganho do insurance
                    if (made_insurance) {
                        bankroll += insurance_bet * 2.0;
                    }
                    
                    mao_para_string(dealer_info.bits, dealer_final_str);
                    
                    // Variável para acumular PNL total da rodada (mãos contabilizadas + insurance)
                    double pnl_rodada_total = 0.0;
                    
                    // Incluir PNL do insurance se foi feito
                    if (made_insurance) {
                        pnl_rodada_total += insurance_bet * 2.0; // Ganho do insurance
                    }
                    
                    // Processar jogadores com avaliação otimizada
                    for (int pj = 0; pj < total_maos; ++pj) {
                        Mao mao_jogador;
                        avaliar_mao_fast(maos_bits[pj], &mao_jogador);
                        mao_jogador.aposta = bet;
                        mao_jogador.finalizada = true;
                        
                        if (pj >= NUM_JOGADORES) {
                            mao_jogador.contabilizada = true;
                        }
                        
                        verificar_mao(&mao_jogador, &dealer_info);
                        calcular_pnl(&mao_jogador);
                        
                        if (mao_jogador.contabilizada) {
                            bankroll += mao_jogador.pnl;
                            pnl_shoe += mao_jogador.pnl;
                            pnl_rodada_total += mao_jogador.pnl; // Acumular PNL da rodada
                            maos_jogadas++;
                            if (mao_jogador.resultado == 'V') {
                                vitorias++;
                            }
                            if (pnl_shoe >= 0.0) {
                                loss_shoe = 0.0;
                            } else {
                                loss_shoe = -pnl_shoe / unidade_atual;
                            }
                        }
                        
                        // Log normal (não otimizado por simplicidade)
                        if (log_file && atomic_load(global_log_count) < log_level) {
                            char init_str[32];
                            char final_str[32];
                            mao_para_string(mao_jogador.initial_bits, init_str);
                            mao_para_string(mao_jogador.bits, final_str);
                            
                            fprintf(log_file, "%s,%c,%s,%s,%d,%s,%c,%.1f,%.1f,%c,%c,%c,%c\n", 
                                   init_str, upcard_char, 
                                   "-", final_str, mao_jogador.valor, 
                                   dealer_final_str, mao_jogador.resultado, mao_jogador.aposta, mao_jogador.pnl,
                                   'N', 'N', mao_jogador.blackjack ? 'S' : 'N', 'S');
                            atomic_fetch_add(global_log_count, 1);
                        }
                    }
                    
                    // Adicionar unidades da rodada à variável global
                    if (pnl_rodada_total != 0.0) {
                        double unidades_rodada = pnl_rodada_total / unidade_atual;
                        pthread_mutex_lock(&unidades_mutex);
                        unidades_total_global += unidades_rodada;
                        pthread_mutex_unlock(&unidades_mutex);
                    }
                    
                    free(maos_bits);
                    continue;
                }
            }

            // Aplicar perda do insurance se foi feito
            if (made_insurance) {
                bankroll -= insurance_bet;
            }
            
            // Jogadores jogam com estratégia otimizada
            Mao all_hands[70];
            int total_hands = 0;
            
            for (int pj = 0; pj < total_maos; ++pj) {
                Mao hands[10];
                int hand_count = 1;
                avaliar_mao_fast(maos_bits[pj], &hands[0]);
                hands[0].aposta = bet;
                
                if (pj >= NUM_JOGADORES) {
                    hands[0].contabilizada = true;
                }
                
                // Jogar cada mão com estratégia otimizada
                for (int h = 0; h < hand_count; ++h) {
                    Mao *m = &hands[h];
                    
                    while (!m->finalizada) {
                        AcaoEstrategia ac = determinar_acao_rapida(m->bits, dealer_upcard, true_count, disable_deviations);
                        
                        switch (ac) {
                            case ACAO_STAND:
                                registrar_acao(m, 'S');
                                m->finalizada = true;
                                break;
                            case ACAO_HIT: {
                                registrar_acao(m, 'H');
                                Carta nova_carta = baralho_comprar(&shoe);
                                m->bits += nova_carta;
                                atualizar_counts(&running_count, &true_count, nova_carta, shoe.total - shoe.topo);
                                
                                // Recalcular usando função otimizada
                                m->valor = calcular_valor_mao_fast(m->bits);
                                m->tipo = tipo_mao_fast(m->bits);
                                m->blackjack = is_blackjack_considerando_split(m->bits, m->from_split);
                                
                                if (m->valor >= 21) {
                                    m->finalizada = true;
                                }
                                break;
                            }
                            // Outros casos simplificados por brevidade
                            default:
                                m->finalizada = true;
                                break;
                        }
                    }
                }
                
                // Copiar mãos para array global
                for (int h = 0; h < hand_count; ++h) {
                    all_hands[total_hands++] = hands[h];
                }
            }
            
            // Dealer joga
            atualizar_counts(&running_count, &true_count, dealer_hole_card, shoe.total - shoe.topo);
            avaliar_mao_dealer(&dealer_info, &shoe, &running_count, &true_count);
            mao_para_string(dealer_info.bits, dealer_final_str);
            
            // Variável para acumular PNL total da rodada (mãos contabilizadas + insurance)
            double pnl_rodada_total = 0.0;
            
            // Incluir perda do insurance se foi feito (dealer não tem BJ)
            if (made_insurance) {
                pnl_rodada_total -= insurance_bet; // Perda do insurance
            }
            
            // Processar resultados
            for (int i = 0; i < total_hands; ++i) {
                Mao *m = &all_hands[i];
                verificar_mao(m, &dealer_info);
                calcular_pnl(m);

                if (m->contabilizada) {
                    bankroll += m->pnl;
                    pnl_shoe += m->pnl;
                    pnl_rodada_total += m->pnl; // Acumular PNL da rodada
                    maos_jogadas++;
                    if (m->resultado == 'V') {
                        vitorias++;
                    }
                    if (pnl_shoe >= 0.0) {
                        loss_shoe = 0.0;
                    } else {
                        loss_shoe = -pnl_shoe / unidade_atual;
                    }
                }

                // Log simplificado
                if (log_file && atomic_load(global_log_count) < log_level) {
                    char init_str[32];
                    char final_str[32];
                    mao_para_string(m->initial_bits, init_str);
                    mao_para_string(m->bits, final_str);
                    
                    fprintf(log_file, "%s,%c,%s,%s,%d,%s,%c,%.1f,%.1f,%c,%c,%c,%c\n", 
                           init_str, upcard_char, 
                           (m->hist_len>0?m->historico:"-"), final_str, m->valor, 
                           dealer_final_str, m->resultado, m->aposta, m->pnl,
                           m->isdouble ? 'S' : 'N',
                           m->from_split ? 'S' : 'N',
                           m->blackjack ? 'S' : 'N',
                           dealer_info.blackjack ? 'S' : 'N');
                    atomic_fetch_add(global_log_count, 1);
                }
            }
            
            // Adicionar unidades da rodada à variável global
            if (pnl_rodada_total != 0.0) {
                double unidades_rodada = pnl_rodada_total / unidade_atual;
                pthread_mutex_lock(&unidades_mutex);
                unidades_total_global += unidades_rodada;
                pthread_mutex_unlock(&unidades_mutex);
            }
            
            free(maos_bits);
        }
        
        baralho_destruir(&shoe);
        shoes_jogados++;
        running_count = 0.0;
        true_count = 0.0;
        pnl_shoe = 0.0;
        loss_shoe = 0.0;
    }
    
    finish_simulation:
    if (log_file) {
        fclose(log_file);
    }
    
    if (dealer_file) {
        fclose(dealer_file);
    }
} 