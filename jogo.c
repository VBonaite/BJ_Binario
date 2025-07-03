#include "jogo.h"
#include <stdint.h>
#include <stdbool.h>
#include "tabela_estrategia.h"
#include "baralho.h"

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

static int rank_from_carta_bits(uint64_t card_bits) {
#if defined(__GNUC__)
    int idx = __builtin_ctzll(card_bits) / 3;
#else
    int idx = 0;
    while ((card_bits & 0x7ULL) == 0) {
        card_bits >>= 3;
        ++idx;
    }
#endif
    if (idx <= 7) return idx + 2;      // 2-9
    if (idx == 8) return 10;           // 10
    if (idx >= 9 && idx <= 11) return 10; // J Q K
    return 11;                         // Ace
}

void avaliar_mao(uint64_t bits, Mao *out) {
    out->bits = bits;
    out->initial_bits = bits;
    out->valor = calcular_valor_mao(bits);
    out->tipo = tipo_mao(bits);
    out->blackjack = (out->tipo == MAO_BLACKJACK);
    out->finalizada = false;
    out->from_split = false;
    out->hist_len = 0;
    out->historico[0] = '\0';
    out->resultado = '?';
}

AcaoEstrategia determinar_acao(const Mao *mao, uint64_t mao_bits, int dealer_up_rank) {
    if (mao->blackjack) return ACAO_STAND; // não usada, mas placeholder

    switch (mao->tipo) {
        case MAO_PAR: {
            // Encontrar rank do par
            int par_rank_val = 0;
            for (int idx = 0; idx < 13; ++idx) {
                uint64_t cnt = (mao_bits >> (idx * 3)) & 0x7ULL;
                if (cnt == 2) {
                    if (idx >= 8 && idx <= 11) {
                        par_rank_val = 10; // 10 J Q K
                    } else if (idx == 12) {
                        par_rank_val = 11; // A
                    } else {
                        par_rank_val = idx + 2;
                    }
                    break;
                }
            }
            return estrategia_par(par_rank_val, dealer_up_rank);
        }
        case MAO_SOFT:
            return estrategia_soft(mao->valor, dealer_up_rank);
        case MAO_HARD:
        default:
            return estrategia_hard(mao->valor, dealer_up_rank);
    }
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

static Carta comprar_carta_e_adicionar(Mao *mao, Shoe *shoe) {
    Carta c = baralho_comprar(shoe);
    mao->bits += c;
    // atualizar valor / tipo / blackjack
    mao->valor = calcular_valor_mao(mao->bits);
    mao->tipo  = tipo_mao(mao->bits);
    mao->blackjack = (mao->tipo == MAO_BLACKJACK);
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
    mao->finalizada = false;
    mao->resultado = '?';
}

Mao* jogar_mao(Mao *mao, Shoe *shoe, int dealer_up_rank, Mao *nova_mao_out) {
    if (mao->blackjack) {
        mao->finalizada = true;
        return NULL;
    }

    if (dealer_up_rank == 11) dealer_up_rank = 11; // Ace remains 11

    if (mao->tipo == MAO_BLACKJACK) {
        mao->finalizada = true;
        return NULL;
    }

    // Dealer logic handled separately outside.

    while (!mao->finalizada) {
        AcaoEstrategia ac = determinar_acao(mao, mao->bits, dealer_up_rank);

        switch (ac) {
            case ACAO_STAND:
                registrar_acao(mao, 'S');
                mao->finalizada = true;
                break;
            case ACAO_HIT:
                registrar_acao(mao, 'H');
                comprar_carta_e_adicionar(mao, shoe);
                if (mao->valor >= 21) {
                    mao->finalizada = true;
                }
                break;
            case ACAO_DOUBLE:
            case ACAO_DOUBLE_OR_HIT:
            case ACAO_DOUBLE_OR_STAND: {
                if (!mao->from_split) {
                    registrar_acao(mao, 'D');
                    comprar_carta_e_adicionar(mao, shoe);
                    mao->finalizada = true;
                } else {
                    if (ac == ACAO_DOUBLE_OR_HIT) {
                        registrar_acao(mao, 'H');
                        comprar_carta_e_adicionar(mao, shoe);
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
                // find rank idx
                int split_rank_idx = -1;
                for (int idx = 0; idx < 13; ++idx) {
                    uint64_t cnt = (mao->bits >> (idx * 3)) & 0x7ULL;
                    if (cnt >= 2) {
                        split_rank_idx = idx;
                        break;
                    }
                }
                if (split_rank_idx == -1) {
                    // cannot split, treat fallback
                    if (ac == ACAO_SPLIT_OR_HIT) {
                        registrar_acao(mao, 'H');
                        comprar_carta_e_adicionar(mao, shoe);
                    } else {
                        registrar_acao(mao, 'S');
                        mao->finalizada = true;
                    }
                    break;
                }

                uint64_t rank_bit = (uint64_t)1ULL << (split_rank_idx * 3);
                // remove one card from original
                mao->bits -= rank_bit;
                // Update mao after removal (decrement)
                mao->valor = calcular_valor_mao(mao->bits);
                mao->tipo = tipo_mao(mao->bits);

                // Initialize new hand
                inicializar_mao(nova_mao_out, rank_bit, true);

                // Give each hand one card
                comprar_carta_e_adicionar(mao, shoe);
                comprar_carta_e_adicionar(nova_mao_out, shoe);

                mao->from_split = true;
                nova_mao_out->from_split = true;
                nova_mao_out->initial_bits = nova_mao_out->bits;

                if (split_rank_idx == 12) { // AA
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

void avaliar_mao_dealer(Mao *dealer, Shoe *shoe) {
    while (dealer->valor < 17) {
        registrar_acao(dealer, 'H');
        comprar_carta_e_adicionar(dealer, shoe);
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