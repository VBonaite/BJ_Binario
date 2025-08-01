#include "jogo.h"
#include <stdint.h>
#include <stdbool.h>
#include "tabela_estrategia.h"
#include "baralho.h"
#include "constantes.h"
#include "saidas.h"
#include "shoe_counter.h"        // Para EV em tempo real
#include "realtime_strategy_integration.h"  // Para EV em tempo real
#include <stdio.h>
#include <string.h>

int calcular_valor_mao(uint64_t mao) {
    int valor = 0;
    int ases = 0;

    for (int idx = 0; idx < 13; ++idx) {
        uint64_t count = (mao >> (idx * 3)) & 0x7ULL;
        if (count == 0) continue;

        if (idx <= 7) { // 2 a 9
            valor += (idx + 2) * count;
        } else if (idx <= 11) { // 10, J, Q, K
            valor += 10 * count;
        } else { // Ás
            ases += (int)count;
        }
    }

    valor += ases * 11;
    while (valor > 21 && ases > 0) {
        valor -= 10;
        --ases;
    }

    return valor;
}

TipoMao tipo_mao(uint64_t mao) {
    int total_cartas = 0;
    int ases = 0;
    bool par = false;

    int valor_total = calcular_valor_mao(mao); // usar valor calculado

    // contar cartas e verificar par
    for (int idx = 0; idx < 13; ++idx) {
        uint64_t count = (mao >> (idx * 3)) & 0x7ULL;
        if (count == 0) continue;
        total_cartas += (int)count;
        if (count == 2 && total_cartas == 2) {
            par = true; // exatamente um par
        }
        if (idx == 12) {
            ases = (int)count;
        }
    }

    if (total_cartas == 2 && valor_total == 21) {
        return MAO_BLACKJACK;
    }

    if (par && total_cartas == 2) {
        return MAO_PAR;
    }

    // Determinar se é soft
    int valor = 0;
    int ases_restantes = ases;
    for (int idx = 0; idx < 12; ++idx) {
        uint64_t count = (mao >> (idx * 3)) & 0x7ULL;
        if (count == 0) continue;
        if (idx <= 7) {
            valor += (idx + 2) * count;
        } else {
            valor += 10 * count; // 10 J Q K
        }
    }
    valor += ases_restantes * 11;
    while (valor > 21 && ases_restantes > 0) {
        valor -= 10;
        --ases_restantes;
    }

    if (ases_restantes > 0) {
        return MAO_SOFT;
    }
    return MAO_HARD;
}


// Função auxiliar - pode ser útil para debugging
//static int rank_from_carta_bits(uint64_t card_bits) {
//#if defined(__GNUC__)
//    int idx = __builtin_ctzll(card_bits) / 3;
//#else
//    int idx = 0;
//    while ((card_bits & 0x7ULL) == 0) {
//        card_bits >>= 3;
//        ++idx;
//    }
//#endif
//    if (idx <= 7) return idx + 2;      // 2-9
//    if (idx == 8) return 10;           // 10
//    if (idx >= 9 && idx <= 11) return 10; // J Q K
//    return 11;                         // Ace
//}

void avaliar_mao(uint64_t bits, Mao *out) {
    out->bits = bits;
    out->initial_bits = bits;
    out->valor = calcular_valor_mao(bits);
    out->tipo = tipo_mao(bits);
    out->finalizada = false;
    out->from_split = false;  // Inicializar antes de usar
    out->isdouble = false;
    out->contabilizada = false;
    out->blackjack = (out->tipo == MAO_BLACKJACK && !out->from_split);
    out->aposta = 0.0;
    out->pnl = 0.0;
    out->hist_len = 0;
    out->historico[0] = '\0';
    out->resultado = '?';
    // Inicializar campos de split
    out->split_pair_index = -1;
    out->split_upcard_index = -1;
    out->split_cards_used = 0;
    out->split_rank_idx = -1;
    out->original_split_bits = 0;
}

// Nova função que integra EV em tempo real DE FORMA OTIMIZADA
AcaoEstrategia determinar_acao_completa(const Mao *mao, uint64_t mao_bits, int dealer_up_rank, 
                                       double true_count, const ShoeCounter* shoe_counter, 
                                       bool is_initial_hand) {
    // SISTEMA DE EV EM TEMPO REAL OTIMIZADO
    if (mao->blackjack) return ACAO_STAND;
    
    // VERIFICAÇÃO GLOBAL: Se o sistema de EV em tempo real está desabilitado, usar apenas estratégia básica
    extern bool realtime_ev_enabled;  // Declaração extern para acessar a variável global
    if (!realtime_ev_enabled) {
        return estrategia_basica_super_rapida(mao_bits, dealer_up_rank);
    }
    
    // SEMPRE usar EV em tempo real quando habilitado
    return determinar_acao_realtime(mao, mao_bits, dealer_up_rank, true_count, shoe_counter, is_initial_hand);
}

AcaoEstrategia determinar_acao(const Mao *mao, uint64_t mao_bits, int dealer_up_rank) {
    // SISTEMA DE EV EM TEMPO REAL ATIVADO
    // Substituindo estratégia básica por cálculos matemáticos precisos
    
    if (mao->blackjack) return ACAO_STAND;

    // Para usar EV em tempo real, precisamos de shoe counter e true count
    // Como estes não estão disponíveis nesta função, vamos usar fallback por enquanto
    // TODO: Refatorar para incluir estes parâmetros na função
    
    // Por enquanto, usar estratégia básica SUPER-OTIMIZADA como fallback
    return estrategia_basica_super_rapida(mao_bits, dealer_up_rank);
}

const char* acao_to_str(AcaoEstrategia a) {
    switch (a) {
        case ACAO_HIT: return "H";
        case ACAO_STAND: return "S";
        case ACAO_DOUBLE: return "D";
        case ACAO_SPLIT: return "P";
        case ACAO_DOUBLE_OR_HIT: return "D/H";
        case ACAO_DOUBLE_OR_STAND: return "D/S";
        case ACAO_SPLIT_OR_HIT: return "P/H";
        case ACAO_SPLIT_OR_STAND: return "P/S";
        default: return "?";
    }
}


static Carta comprar_carta_e_adicionar(Mao *mao, Shoe *shoe, double *running_count, double *true_count) {
    Carta c = baralho_comprar(shoe);
    mao->bits += c;
	if (running_count) {
        *running_count += WONG_HALVES[carta_para_rank_idx(c)];
        if (true_count) {
            size_t cartas_restantes = shoe->total - shoe->topo;
            double decks_restantes = (double)cartas_restantes / 52.0;
            if (decks_restantes < 1.0) decks_restantes = 1.0;
            *true_count = *running_count / decks_restantes;
        }
    }

    // atualizar valor / tipo / blackjack
    mao->valor = calcular_valor_mao(mao->bits);
    mao->tipo  = tipo_mao(mao->bits);
    mao->blackjack = (mao->tipo == MAO_BLACKJACK && !mao->from_split);
    return c;
}

static void registrar_acao(Mao *mao, char code) {
    int n = mao->hist_len;
    if (n < (int)sizeof(mao->historico)-1) {
        mao->historico[n] = code;
        mao->historico[n+1] = '\0';
        mao->hist_len++;
    }
}

// Helper to duplicate a mao when splitting
static void inicializar_mao(Mao *mao, uint64_t bits, bool from_split) {
    mao->bits = bits;
    mao->initial_bits = bits;
    mao->from_split = from_split;
    mao->hist_len = 0;
    avaliar_mao(bits, mao);
    
    // Corrigir from_split após avaliar_mao (que define como false)
    mao->from_split = from_split;
    // Reavaliar blackjack considerando from_split
    mao->blackjack = (mao->tipo == MAO_BLACKJACK && !from_split);
    
    mao->finalizada = false;
    mao->isdouble = false;
    mao->contabilizada = false;
    mao->aposta = 0.0;
    mao->pnl = 0.0;
    mao->resultado = '?';
    
    // Inicializar campos de split
    mao->split_pair_index = -1;
    mao->split_upcard_index = -1;
    mao->split_cards_used = 0;
    mao->split_rank_idx = -1;
    mao->original_split_bits = 0;
}


Mao* jogar_mao(Mao *mao, Shoe *shoe, int dealer_up_rank, Mao *nova_mao_out, 
               double *running_count, double *true_count, ShoeCounter *shoe_counter, bool ev_realtime_enabled) {
    if (mao->blackjack) {
        mao->finalizada = true;
        return NULL;
    }

    if (dealer_up_rank == 11) dealer_up_rank = 11; // Ace remains 11

    if (mao->tipo == MAO_BLACKJACK) {
        mao->finalizada = true;
        return NULL;
    }

    // Sistema de EV em tempo real será usado quando shoe_counter estiver disponível

    while (!mao->finalizada) {
        // Usar sistema de EV em tempo real se ativado e disponível
        AcaoEstrategia ac;
        if (ev_realtime_enabled && shoe_counter && shoe_counter->initialized) {
            ac = determinar_acao_completa(mao, mao->bits, dealer_up_rank, 
                                        *true_count, shoe_counter, 
                                        mao->hist_len == 0); // is_initial_hand
        } else {
            // Usar estratégia básica (padrão ou fallback)
            ac = determinar_acao(mao, mao->bits, dealer_up_rank);
        }

        switch (ac) {
            case ACAO_STAND:
                registrar_acao(mao, 'S');
                mao->finalizada = true;
                break;
            case ACAO_HIT:
                registrar_acao(mao, 'H');
                comprar_carta_e_adicionar(mao, shoe, running_count, true_count);
                
                // Atualizar shoe counter após carta distribuída
                if (shoe_counter && shoe->topo > 0) {
                    Carta ultima_carta = shoe->cartas[shoe->topo - 1];
                    int rank_idx = carta_para_rank_idx(ultima_carta);
                    if (rank_idx >= 0 && rank_idx < NUM_RANKS && shoe_counter->counts[rank_idx] > 0) {
                        shoe_counter->counts[rank_idx]--;
                        shoe_counter->total_cards--;
                    }
                }
                
                if (mao->valor >= 21) {
                    mao->finalizada = true;
                }
                break;
            case ACAO_DOUBLE:
            case ACAO_DOUBLE_OR_HIT:
            case ACAO_DOUBLE_OR_STAND: {
                if (!mao->from_split) {
                    registrar_acao(mao, 'D');
                	comprar_carta_e_adicionar(mao, shoe, running_count, true_count);
                    
                    // Atualizar shoe counter após carta distribuída
                    if (shoe_counter && shoe->topo > 0) {
                        Carta ultima_carta = shoe->cartas[shoe->topo - 1];
                        int rank_idx = carta_para_rank_idx(ultima_carta);
                        if (rank_idx >= 0 && rank_idx < NUM_RANKS && shoe_counter->counts[rank_idx] > 0) {
                            shoe_counter->counts[rank_idx]--;
                            shoe_counter->total_cards--;
                        }
                    }
                    
                    mao->finalizada = true;;
                    mao->isdouble = true;
                } else {
                    if (ac == ACAO_DOUBLE_OR_HIT) {
                        registrar_acao(mao, 'H');
		                comprar_carta_e_adicionar(mao, shoe, running_count, true_count);
		                
                        // Atualizar shoe counter após carta distribuída
                        if (shoe_counter && shoe->topo > 0) {
                            Carta ultima_carta = shoe->cartas[shoe->topo - 1];
                            int rank_idx = carta_para_rank_idx(ultima_carta);
                            if (rank_idx >= 0 && rank_idx < NUM_RANKS && shoe_counter->counts[rank_idx] > 0) {
                                shoe_counter->counts[rank_idx]--;
                                shoe_counter->total_cards--;
                            }
                        }
                        
                        if (mao->valor >= 21) mao->finalizada = true;
                    } else { // fallback stand
                        registrar_acao(mao, 'S');
                        mao->finalizada = true;
                    }
                }
                break;
            }
            case ACAO_SPLIT:
            case ACAO_SPLIT_OR_HIT:
            case ACAO_SPLIT_OR_STAND: {
                // Determine if we can split; assume we always can for this simplistic model
                registrar_acao(mao, 'P');

                int split_rank_idx_real = -1;   // índice do rank cuja carta será movida para a nova mão
                int split_rank_idx_class = -1;  // índice usado para fins de estatística (pode ser diferente)

                // --- Passo 1: verificar se há par natural (duas cartas do MESMO rank) ---
                int ranks_present[2] = {-1, -1}; // armazenar os ranks das duas cartas caso não seja par natural
                int cards_found = 0;
                for (int idx = 0; idx < 13; ++idx) {
                    uint64_t cnt = (mao->bits >> (idx * 3)) & 0x7ULL;
                    if (cnt == 0) continue;

                    if (cnt >= 2 && split_rank_idx_real == -1) {
                        // Par natural
                        split_rank_idx_real = idx;
                    }

                    // Guardar até dois ranks encontrados (para caso de 10,J etc.)
                    if (cnt >= 1 && cards_found < 2) {
                        ranks_present[cards_found++] = idx;
                    }
                }

                // --- Passo 2: Se não houve par natural, verificar combinação 10-value (T,J,Q,K) ---
                if (split_rank_idx_real == -1 && cards_found == 2) {
                    bool first_is_ten = (ranks_present[0] >= 8 && ranks_present[0] <= 11);
                    bool second_is_ten = (ranks_present[1] >= 8 && ranks_present[1] <= 11);
                    if (first_is_ten && second_is_ten) {
                        // Podemos considerar como split de 10-10
                        split_rank_idx_real = ranks_present[0];   // mover esta carta
                        split_rank_idx_class = 8;                // classificar como 10-10
                    }
                }

                // --- Passo 3: Se ainda não for possível splitar, aplicar fallback ---
                if (split_rank_idx_real == -1) {
                    // não é possível splitar
                    if (ac == ACAO_SPLIT_OR_HIT) {
                        registrar_acao(mao, 'H');
                        comprar_carta_e_adicionar(mao, shoe, running_count, true_count);
                    } else {
                        registrar_acao(mao, 'S');
                        mao->finalizada = true;
                    }
                    break;
                }

                // Se não definimos índice de classificação ainda, usar o real
                if (split_rank_idx_class == -1) {
                    split_rank_idx_class = split_rank_idx_real;
                }

                // Agrupar quaisquer pares de valor 10 (8-11) como 10-10 (índice 8) para estatísticas
                if (split_rank_idx_real >= 8 && split_rank_idx_real <= 11) {
                    split_rank_idx_class = 8;
                }

                // --- Passo 4: executar o split físico ---
                uint64_t rank_bit = (uint64_t)1ULL << (split_rank_idx_real * 3);
                mao->bits -= rank_bit;                       // remover uma carta da mão original
                mao->valor = calcular_valor_mao(mao->bits);  // atualizar valor/tipo
                mao->tipo  = tipo_mao(mao->bits);

                // Inicializar nova mão com a carta removida
                inicializar_mao(nova_mao_out, rank_bit, true);

                // Salvar mão original antes do split (apenas as 2 cartas iniciais)
                uint64_t original_split_bits = mao->initial_bits;
                
                // Dar uma carta adicional para cada mão
                comprar_carta_e_adicionar(mao, shoe, running_count, true_count);
                comprar_carta_e_adicionar(nova_mao_out, shoe, running_count, true_count);

                mao->from_split = true;
                nova_mao_out->from_split = true;
                
                // CORREÇÃO: Atualizar initial_bits APÓS adicionar cartas
                mao->initial_bits = mao->bits;
                nova_mao_out->initial_bits = nova_mao_out->bits;

                // Salvar índice de classificação (para estatísticas)
                mao->split_rank_idx = split_rank_idx_class;
                nova_mao_out->split_rank_idx = split_rank_idx_class;
                
                // Salvar mão original
                mao->original_split_bits = original_split_bits;
                nova_mao_out->original_split_bits = original_split_bits;

                // Debug removido do momento do split - será impresso no processamento final

                // Reavaliar blackjack após marcar from_split
                mao->blackjack = (mao->tipo == MAO_BLACKJACK && !mao->from_split);

                // Tratamento especial para A,A
                if (split_rank_idx_real == 12) { // AA
                    mao->finalizada = true;
                    nova_mao_out->finalizada = true;
                }
                return nova_mao_out;
            }
            default:
                mao->finalizada = true;
                break;
        }
    }
    return NULL;
}

void avaliar_mao_dealer(Mao *dealer, Shoe *shoe, double *running_count, double *true_count) {
    while (dealer->valor < 17) {
        registrar_acao(dealer, 'H');
        comprar_carta_e_adicionar(dealer, shoe, running_count, true_count);
    }
    dealer->finalizada = true;
}

void verificar_mao(Mao *jog, const Mao *dealer){
    // Assume dealer and jog have valor, blackjack updated
    char res = 'D';
    if (dealer->blackjack){
        if (jog->blackjack) res = 'E';
        else res = 'D';
    } else {
        if (jog->blackjack) res = 'V';
        else {
            if (dealer->valor > 21){
                if (jog->valor <= 21) res = 'V';
                else res = 'D';
            } else {
                if (jog->valor > 21) res = 'D';
                else if (jog->valor == dealer->valor) res = 'E';
                else if (jog->valor > dealer->valor) res = 'V';
                else res = 'D';
            }
        }
    }
    jog->resultado = res;
}


void calcular_pnl(Mao *mao) {
    if (mao->resultado == 'V') {
        if (mao->blackjack) {
            mao->pnl = 1.5 * mao->aposta;
        } else if (mao->isdouble) {
            mao->pnl = 2.0 * mao->aposta;
        } else {
            mao->pnl = mao->aposta;
        }
    } else if (mao->resultado == 'E') {
        mao->pnl = 0.0;
    } else if (mao->resultado == 'D') {
        if (mao->isdouble) {
            mao->pnl = -2.0 * mao->aposta;
        } else {
            mao->pnl = -mao->aposta;
        }
    }
}

int calcular_maos_contabilizadas(double true_count) {
    if (true_count < TC_MAOS_CONTAB[0]) {
        return 0;  // 1 mão contabilizada -> total 5 mãos
    } else if (true_count >= TC_MAOS_CONTAB[0] && true_count <= TC_MAOS_CONTAB[1]) {
        return 1;  // 2 mãos contabilizadas -> total 6 mãos
    } else if (true_count > TC_MAOS_CONTAB[1] && true_count <= TC_MAOS_CONTAB[2]) {
        return 2;  // 2 mãos contabilizadas -> total 6 mãos
    } else {
        return 3;  // 3 mãos contabilizadas -> total 7 mãos
    }
}

// Função auxiliar para ajustar unidades baseado no true count
static double _ajusta_unidades(double base, double tc, bool shoe_ok) {
    if (!shoe_ok) {
        return base;
    }
    
    // Encontrar a aposta correspondente ao true count usando os arrays de constantes
    double aposta_tc = APOSTAS_BASE_AJUSTE[0]; // valor padrão para tc muito baixo
    
    for (int i = 0; i < 10; i++) {
        if (tc < TC_AJUSTA_UNIDADES[i]) {
            aposta_tc = APOSTAS_BASE_AJUSTE[i];
            break;
        }
        // Se chegou ao último elemento, usar a última aposta
        if (i == 9) {
            aposta_tc = APOSTAS_BASE_AJUSTE[9];
        }
    }
    
    // Se a aposta calculada for menor que a aposta do true count, usar a do true count
    // Caso contrário, usar a aposta base calculada
    return (base < aposta_tc) ? aposta_tc : base;
}

// Função para definir aposta baseada no sistema de progressão
int definir_aposta(size_t cartas_restantes, int vitorias, double true_count, int maos_jogadas, double loss_shoe, double unidade_atual) {
    (void)loss_shoe; // Marcar como unused para evitar warning
    double resultado_aposta;
    
    // Se shoe muito longo, apostar só 1 unidade
    if (cartas_restantes > (size_t)CARTAS_RESTANTES_LIMITE) {
        resultado_aposta = unidade_atual;
    } else {
        bool shoe_ok = (cartas_restantes >= (size_t)CARTAS_RESTANTES_SHOE_OK);
        double pct_vit = (maos_jogadas > 0) ? (vitorias / (double)maos_jogadas) * 100.0 : 0.0;

        // Percorrer os blocos de condições (mantendo min_pct, min_len_shoe e min_true_count)
        bool encontrou_bloco = false;
        for (int i = 0; i < 12; ++i) {
            if (pct_vit < MIN_PCT[i] && 
                (int)cartas_restantes <= MIN_LEN_SHOE[i] && 
                true_count >= MIN_TRUE_COUNT[i]) {
                
                // Usar aposta base diretamente do array APOSTAS_BASE
                double base = APOSTAS_BASE[i];
                
                resultado_aposta = _ajusta_unidades(base, true_count, shoe_ok) * unidade_atual;
                encontrou_bloco = true;
                break;
            }
        }

        if (!encontrou_bloco) {
            // Fallback genérico - usar a menor aposta base
            double base = APOSTAS_BASE[11]; // última posição (2.0)
            resultado_aposta = _ajusta_unidades(base, true_count, shoe_ok) * unidade_atual;
        }
    }

    // Converter para inteiro e garantir mínimo de 5
    int aposta_final = (int)(resultado_aposta + 0.5); // Arredondamento
    return (aposta_final >= 5) ? aposta_final : 5;
}

// Função para calcular unidade baseada no bankroll
double calcular_unidade(double bankroll) {
    double unidade_bankroll = bankroll / 1500.0;
    // Arredondar para baixo (int cast)
    unidade_bankroll = (double)((int)unidade_bankroll);
    return (unidade_bankroll > UNIDADE_INICIAL) ? unidade_bankroll : UNIDADE_INICIAL;
}


 
