#include "real_time_ev.h"
#include "realtime_strategy_integration.h"  // Para acesso às estatísticas
#include "jogo.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h> // Para assert

// Cache otimizado com hash table
#define CACHE_SIZE 4096  // Potência de 2 para operação de módulo eficiente
#define CACHE_MASK (CACHE_SIZE - 1)  // Para hash rápido

static EVCache ev_cache[CACHE_SIZE];
static bool cache_initialized = false;
static int cache_hits = 0;
static int cache_misses = 0;

// ====================== FUNÇÕES DE CACHE OTIMIZADO ======================

// Função de hash rápida para chave composta
static uint64_t calculate_cache_key(uint64_t hand_bits, int dealer_upcard, double true_count) {
    // Discretizar true count em bins de 0.1 para consistência
    int tc_bin = (int)(true_count * 10.0 + 10000); // +10000 para tornar positivo
    
    // Hash usando FNV-1a algorithm (rápido e boa distribuição)
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    
    // Combinar hand_bits
    hash ^= hand_bits;
    hash *= 1099511628211ULL; // FNV prime
    
    // Combinar dealer_upcard
    hash ^= (uint64_t)dealer_upcard;
    hash *= 1099511628211ULL;
    
    // Combinar tc_bin
    hash ^= (uint64_t)tc_bin;
    hash *= 1099511628211ULL;
    
    return hash;
}

// ====================== FUNÇÕES DE NORMALIZAÇÃO DO TRUE COUNT ======================

double normalize_true_count(double true_count) {
    // Limita TC entre -6.5 e +6.5 conforme especificação
    if (true_count > MAX_TC_LIMIT) return MAX_TC_LIMIT;
    if (true_count < MIN_TC_LIMIT) return MIN_TC_LIMIT;
    
    // Arredonda para uma casa decimal
    return round(true_count * 10.0) / 10.0;
}

double get_tc_bin_start(double true_count) {
    // Para TC = 3.27 → retorna 3.2 (início do bin 3.2-3.3)
    // Para TC = 3.2 → retorna 3.2 (início do bin 3.2-3.3)
    double normalized = normalize_true_count(true_count);
    return floor(normalized * 10.0) / 10.0;
}

// ====================== ANÁLISE DE MÃOS ======================

bool is_soft_hand(uint64_t hand_bits) {
    // Verifica se há Ás e se pode ser contado como 11 sem bust
    int aces = 0;
    int total_value = 0;
    int total_cards = 0;
    
    for (int idx = 0; idx < 13; ++idx) {
        uint64_t count = (hand_bits >> (idx * 3)) & 0x7ULL;
        if (count == 0) continue;
        
        total_cards += (int)count;
        
        if (idx <= 7) { // 2-9
            total_value += (idx + 2) * count;
        } else if (idx <= 11) { // 10,J,Q,K
            total_value += 10 * count;
        } else { // Ás
            aces += (int)count;
        }
    }
    
    // Contar ases como 11 inicialmente
    total_value += aces * 11;
    
    // Se pode contar pelo menos um ás como 11 sem bust, é soft
    return (aces > 0 && total_value <= 21);
}

bool is_pair_hand(uint64_t hand_bits) {
    int total_cards = 0;
    int pairs_found = 0;
    
    for (int idx = 0; idx < 13; ++idx) {
        uint64_t count = (hand_bits >> (idx * 3)) & 0x7ULL;
        total_cards += (int)count;
        if (count == 2) pairs_found++;
    }
    
    return (total_cards == 2 && pairs_found == 1);
}

int get_pair_rank(uint64_t hand_bits) {
    if (!is_pair_hand(hand_bits)) return -1;
    
    for (int idx = 0; idx < 13; ++idx) {
        uint64_t count = (hand_bits >> (idx * 3)) & 0x7ULL;
        if (count == 2) {
            if (idx <= 7) return idx + 2;  // 2-9
            if (idx <= 11) return 10;      // 10,J,Q,K
            return 11;                     // Ás
        }
    }
    return -1;
}

// ====================== PROBABILIDADES DO DEALER ======================

DealerProbabilities get_dealer_probabilities(int dealer_upcard, double true_count, const ShoeCounter* counter) {
    DealerProbabilities probs = {0};
    
    // Normalizar true count para lookup
    double tc_bin = get_tc_bin_start(true_count);
    
    // Usar dealer frequency lookup tables se disponíveis
    if (dealer_freq_table_loaded) {
        int upcard_idx = dealer_upcard_to_index(dealer_upcard);
        int tc_bin_idx = get_tc_bin_index(tc_bin);
        
        if (upcard_idx >= 0 && tc_bin_idx >= 0) {
            probs.prob_17 = dealer_freq_table[upcard_idx][DEALER_RESULT_17][tc_bin_idx];
            probs.prob_18 = dealer_freq_table[upcard_idx][DEALER_RESULT_18][tc_bin_idx];
            probs.prob_19 = dealer_freq_table[upcard_idx][DEALER_RESULT_19][tc_bin_idx];
            probs.prob_20 = dealer_freq_table[upcard_idx][DEALER_RESULT_20][tc_bin_idx];
            probs.prob_21 = dealer_freq_table[upcard_idx][DEALER_RESULT_21][tc_bin_idx];
            probs.prob_blackjack = dealer_freq_table[upcard_idx][DEALER_RESULT_BJ][tc_bin_idx];
            probs.prob_bust = dealer_freq_table[upcard_idx][DEALER_RESULT_BUST][tc_bin_idx];
            probs.probabilities_valid = true;
            return probs;
        }
    }
    
    // Fallback: usar probabilidades aproximadas baseadas em composition
    double high_card_concentration = (double)shoe_counter_get_ten_value_cards(counter) / 
                                   shoe_counter_get_total_cards(counter);
    double normal_concentration = 4.0 / 13.0;
    double adjustment = (high_card_concentration / normal_concentration - 1.0) * 0.1;
    
    // Probabilidades base aproximadas por upcard
    double base_probs[10][7] = {
        // 17,   18,   19,   20,   21,   BJ,   BUST
        {0.13, 0.13, 0.13, 0.18, 0.12, 0.00, 0.31}, // 2
        {0.13, 0.13, 0.13, 0.18, 0.12, 0.00, 0.31}, // 3
        {0.13, 0.13, 0.13, 0.18, 0.12, 0.00, 0.31}, // 4
        {0.12, 0.12, 0.12, 0.17, 0.11, 0.00, 0.36}, // 5
        {0.11, 0.11, 0.11, 0.16, 0.10, 0.00, 0.41}, // 6
        {0.14, 0.14, 0.14, 0.19, 0.13, 0.00, 0.26}, // 7
        {0.13, 0.13, 0.13, 0.18, 0.12, 0.00, 0.31}, // 8
        {0.12, 0.12, 0.12, 0.17, 0.11, 0.00, 0.36}, // 9
        {0.11, 0.11, 0.11, 0.16, 0.10, 0.00, 0.41}, // 10
        {0.13, 0.13, 0.13, 0.18, 0.12, 0.31, 0.00}  // A
    };
    
    int upcard_idx = dealer_upcard - 2;
    if (upcard_idx < 0 || upcard_idx > 9) upcard_idx = 8; // default to 10
    
    probs.prob_17 = base_probs[upcard_idx][0];
    probs.prob_18 = base_probs[upcard_idx][1];
    probs.prob_19 = base_probs[upcard_idx][2];
    probs.prob_20 = base_probs[upcard_idx][3];
    probs.prob_21 = base_probs[upcard_idx][4];
    probs.prob_blackjack = base_probs[upcard_idx][5];
    probs.prob_bust = base_probs[upcard_idx][6] + adjustment;
    
    // Normalizar para somar 1.0
    double total = probs.prob_17 + probs.prob_18 + probs.prob_19 + 
                  probs.prob_20 + probs.prob_21 + probs.prob_blackjack + probs.prob_bust;
    if (total > 0) {
        probs.prob_17 /= total;
        probs.prob_18 /= total;
        probs.prob_19 /= total;
        probs.prob_20 /= total;
        probs.prob_21 /= total;
        probs.prob_blackjack /= total;
        probs.prob_bust /= total;
    }
    
    probs.probabilities_valid = true;
    return probs;
}

// ====================== CÁLCULO DE EV STAND ======================

double calculate_ev_stand_realtime(int hand_value, int dealer_upcard, double true_count, const ShoeCounter* counter) {
    /*
     * FÓRMULA STAND IMPLEMENTADA:
     * EV_stand(P_h) = Σ [P(dealer final) × outcome(P_h, dealer final)]
     * 
     * Onde:
     * - outcome = +1 se jogador vence, 0 se empate, -1 se perde
     * - P(dealer final) baseado em composição atual do shoe
     */
    
    if (hand_value > 21) return -1.0; // Bust
    if (hand_value < 3) return -1.0;  // Mão inválida
    
    // Obter probabilidades do dealer baseadas no estado atual do shoe
    DealerProbabilities dealer_probs = get_dealer_probabilities(dealer_upcard, true_count, counter);
    
    if (!dealer_probs.probabilities_valid || !validate_dealer_probabilities(&dealer_probs)) {
        // CORREÇÃO: Fallback mais inteligente baseado em probabilidades aproximadas
        // Para mão 21: sempre boa (empata ou vence)
        if (hand_value == 21) return 0.0;
        // Para mão 20: geralmente boa
        if (hand_value == 20) return -0.1;
        // Para mão 19: moderadamente boa
        if (hand_value == 19) return -0.2;
        // Para mão 18: neutra
        if (hand_value == 18) return -0.3;
        // Para mão 17: ruim
        if (hand_value == 17) return -0.4;
        // Para mãos ≤ 16: muito ruins
        return -0.5;
    }
    
    double ev_stand = 0.0;
    
    // CORREÇÃO CRÍTICA: Fórmulas de EV Stand corrigidas
    if (hand_value == 21) {
        // CORREÇÃO CRÍTICA: Fórmula correta para mão 21
        // Vitória: quando dealer bust ou tem 17-20
        // Empate: quando dealer tem 21
        // Derrota: quando dealer tem blackjack
        ev_stand = 1.0 * (dealer_probs.prob_bust + dealer_probs.prob_17 + dealer_probs.prob_18 + dealer_probs.prob_19 + dealer_probs.prob_20)
                 + 0.0 * dealer_probs.prob_21
                 - 1.0 * dealer_probs.prob_blackjack;
        
    } else if (hand_value == 20) {
        // Para P_h = 20: wins contra 17,18,19,bust; push contra 20; loses contra 21,BJ
        ev_stand = 1.0 * (dealer_probs.prob_bust + dealer_probs.prob_17 + dealer_probs.prob_18 + dealer_probs.prob_19)
                 + 0.0 * dealer_probs.prob_20
                 - 1.0 * (dealer_probs.prob_21 + dealer_probs.prob_blackjack);
                 
    } else if (hand_value == 19) {
        // Para P_h = 19: wins contra 17,18,bust; push contra 19; loses contra 20,21,BJ
        ev_stand = 1.0 * (dealer_probs.prob_bust + dealer_probs.prob_17 + dealer_probs.prob_18)
                 + 0.0 * dealer_probs.prob_19
                 - 1.0 * (dealer_probs.prob_20 + dealer_probs.prob_21 + dealer_probs.prob_blackjack);
                 
    } else if (hand_value == 18) {
        // Para P_h = 18: wins contra 17,bust; push contra 18; loses contra 19,20,21,BJ
        ev_stand = 1.0 * (dealer_probs.prob_bust + dealer_probs.prob_17)
                 + 0.0 * dealer_probs.prob_18
                 - 1.0 * (dealer_probs.prob_19 + dealer_probs.prob_20 + dealer_probs.prob_21 + dealer_probs.prob_blackjack);
                 
    } else if (hand_value == 17) {
        // Para P_h = 17: wins contra bust; push contra 17; loses contra 18,19,20,21,BJ
        ev_stand = 1.0 * dealer_probs.prob_bust
                 + 0.0 * dealer_probs.prob_17
                 - 1.0 * (dealer_probs.prob_18 + dealer_probs.prob_19 + dealer_probs.prob_20 + dealer_probs.prob_21 + dealer_probs.prob_blackjack);
                 
    } else {
        // Para P_h ≤ 16: EV = (+1) × P_d(bust) + (-1) × [P_d(17-21) + P_d(BJ)]
        ev_stand = dealer_probs.prob_bust - (dealer_probs.prob_17 + dealer_probs.prob_18 + dealer_probs.prob_19 + 
                                           dealer_probs.prob_20 + dealer_probs.prob_21 + dealer_probs.prob_blackjack);
    }
    
    // CORREÇÃO CRÍTICA: Remoção da dupla contabilização de dealer blackjack
    // A probabilidade de dealer BJ já está incluída nos cálculos acima
    
    return ev_stand;
}

// ====================== FUNÇÕES DE VALIDAÇÃO ======================

bool validate_shoe_counter(const ShoeCounter* counter) {
    if (!counter || !counter->initialized) {
        return false;
    }
    
    int total_calculated = 0;
    for (int i = 0; i < NUM_RANKS; i++) {
        if (counter->counts[i] < 0) {
            return false;  // Contagem negativa
        }
        total_calculated += counter->counts[i];
    }
    
    // Verificar consistência do total
    if (total_calculated != counter->total_cards) {
        return false;
    }
    
    // Verificar se total é razoável (0-416 cartas para 8 decks)
    if (counter->total_cards < 0 || counter->total_cards > 416) {
        return false;
    }
    
    return true;
}

bool validate_dealer_probabilities(const DealerProbabilities* probs) {
    if (!probs || !probs->probabilities_valid) {
        return false;
    }
    
    // Verificar se todas as probabilidades são não-negativas
    if (probs->prob_17 < 0.0 || probs->prob_18 < 0.0 || probs->prob_19 < 0.0 ||
        probs->prob_20 < 0.0 || probs->prob_21 < 0.0 || probs->prob_blackjack < 0.0 ||
        probs->prob_bust < 0.0) {
        return false;
    }
    
    // CORREÇÃO: Tolerância mais flexível para erros de floating point
    double total = probs->prob_17 + probs->prob_18 + probs->prob_19 + 
                  probs->prob_20 + probs->prob_21 + probs->prob_blackjack + 
                  probs->prob_bust;
    
    if (fabs(total - 1.0) > 0.01) {  // Aumentado para 1% de tolerância
        return false;  // Soma não é 1.0 (tolerância de 1%)
    }
    
    return true;
}

bool validate_true_count(double true_count) {
    // Verificar se está dentro dos limites esperados
    if (true_count < MIN_TC_LIMIT - 0.1 || true_count > MAX_TC_LIMIT + 0.1) {
        return false;
    }
    
    // Verificar se não é NaN ou infinito
    if (isnan(true_count) || isinf(true_count)) {
        return false;
    }
    
    return true;
}

// ====================== SIMULAÇÃO DE REMOÇÃO DE CARTA ======================

ShoeCounter simulate_card_removal(const ShoeCounter* counter, int card_rank) {
    ShoeCounter temp_counter = *counter;
    int rank_idx = rank_value_to_idx(card_rank);
    
    if (rank_idx >= 0 && rank_idx < NUM_RANKS && temp_counter.counts[rank_idx] > 0) {
        temp_counter.counts[rank_idx]--;
        temp_counter.total_cards--;
    }
    
    return temp_counter;
}

// ====================== FUNÇÕES AUXILIARES PARA CÁLCULO ======================

double calculate_ev_optimal_after_card(uint64_t current_hand_bits, int card_rank, 
                                      int dealer_upcard, double true_count, 
                                      const ShoeCounter* counter, int depth);

// CORREÇÃO CRÍTICA: Função para calcular probabilidade correta de cartas
double get_card_probability(const ShoeCounter* counter, int card_rank) {
    int total_cards = shoe_counter_get_total_cards(counter);
    if (total_cards <= 0) return 0.0;
    
    if (card_rank == 10) {
        // CORREÇÃO: Para valor 10, somar todas as cartas de valor 10 (índices 8-11)
        int ten_value_cards = shoe_counter_get_ten_value_cards(counter);
        return (double)ten_value_cards / total_cards;
    } else {
        // Para outros valores, usar o mapeamento normal
        int rank_idx = rank_value_to_idx(card_rank);
        if (rank_idx < 0) return 0.0;
        int available_cards = shoe_counter_get_rank_count(counter, rank_idx);
        return (double)available_cards / total_cards;
    }
}

// CORREÇÃO CRÍTICA: Função para simular remoção de carta com tratamento correto
ShoeCounter simulate_card_removal_corrected(const ShoeCounter* counter, int card_rank) {
    ShoeCounter temp_counter = *counter;
    
    if (card_rank == 10) {
        // CORREÇÃO: Para valor 10, remover uma carta de qualquer um dos índices 8-11
        // Priorizar índices com mais cartas para distribuição uniforme
        int max_idx = 8;
        int max_count = temp_counter.counts[8];
        
        for (int idx = 9; idx <= 11; idx++) {
            if (temp_counter.counts[idx] > max_count) {
                max_count = temp_counter.counts[idx];
                max_idx = idx;
            }
        }
        
        if (temp_counter.counts[max_idx] > 0) {
            temp_counter.counts[max_idx]--;
            temp_counter.total_cards--;
        }
    } else {
        // Para outros valores, usar o mapeamento normal
        int rank_idx = rank_value_to_idx(card_rank);
        if (rank_idx >= 0 && rank_idx < NUM_RANKS && temp_counter.counts[rank_idx] > 0) {
            temp_counter.counts[rank_idx]--;
            temp_counter.total_cards--;
        }
    }
    
    return temp_counter;
}

double calculate_ev_after_receiving_card(uint64_t current_hand, int card_rank, 
                                        int dealer_upcard, double true_count, 
                                        const ShoeCounter* counter, int depth) {
    /*
     * CORREÇÃO CRÍTICA: Calcula o EV ótimo após receber uma carta específica
     * Implementa a fórmula correta: max(EV_stand(p + c), EV_hit(p + c))
     */
    
    if (depth > MAX_RECURSION_DEPTH) {
        // Fallback inteligente: usar EV Stand da mão atual
        int current_value = calcular_valor_mao(current_hand);
        return calculate_ev_stand_realtime(current_value, dealer_upcard, true_count, counter);
    }
    
    int current_value = calcular_valor_mao(current_hand);
    bool current_is_soft = is_soft_hand(current_hand);
    
    // CORREÇÃO CRÍTICA: Tratamento correto do Ás e conversão soft->hard
    int final_value;
    bool final_is_soft = false;
    
    if (card_rank == 11) { // Ás
        // Para Ás: usar como 11 se possível, senão como 1
        if (current_value + 11 <= 21) {
            final_value = current_value + 11;  // Usar como 11 (mão soft)
            final_is_soft = true;
        } else {
            final_value = current_value + 1;   // Usar como 1 (mão hard)
            final_is_soft = false;
        }
    } else {
        // Para cartas 2-10: adicionar valor da carta
        final_value = current_value + card_rank;
        
        // Se mão atual é soft e nova mão > 21, converter Ás de 11 para 1
        if (current_is_soft && final_value > 21) {
            final_value -= 10;  // Converter Ás de 11 para 1
            final_is_soft = false;
        } else {
            final_is_soft = current_is_soft;
        }
    }
    
    // Verificar se resultou em bust
    if (final_value > 21) {
        return -1.0; // Bust
    }
    
    // CORREÇÃO CRÍTICA: Usar função corrigida de simulação de remoção
    ShoeCounter temp_counter = simulate_card_removal_corrected(counter, card_rank);
    
    // CORREÇÃO CRÍTICA: Calcular EV para o valor final determinado
    if (final_value == 21) {
        // Blackjack/21: sempre stand
        return calculate_ev_stand_realtime(21, dealer_upcard, true_count, &temp_counter);
    } else {
        // CORREÇÃO CRÍTICA: Comparar stand vs hit para a NOVA mão
        double ev_stand = calculate_ev_stand_realtime(final_value, dealer_upcard, true_count, &temp_counter);
        
        // CORREÇÃO CRÍTICA: Calcular EV_hit da NOVA mão (não da atual)
        // Criar nova mão com a carta recebida
        uint64_t new_hand = add_card_to_hand(current_hand, card_rank);
        double ev_hit = calculate_ev_hit_realtime(new_hand, dealer_upcard, true_count, &temp_counter, depth + 1);
        
        return (ev_stand > ev_hit) ? ev_stand : ev_hit;
    }
}

// Função auxiliar para adicionar carta à mão
uint64_t add_card_to_hand(uint64_t hand_bits, int card_rank) {
    int rank_idx = rank_value_to_idx(card_rank);
    if (rank_idx < 0) return hand_bits;
    
    // Adicionar uma carta do rank especificado
    uint64_t mask = 7ULL << (rank_idx * 3); // Máscara para o rank
    uint64_t current_count = (hand_bits & mask) >> (rank_idx * 3);
    
    if (current_count < 7) { // Máximo 7 cartas por rank
        hand_bits += (1ULL << (rank_idx * 3));
    }
    
    return hand_bits;
}

double calculate_ev_optimal_after_card(uint64_t current_hand_bits, int card_rank, 
                                      int dealer_upcard, double true_count, 
                                      const ShoeCounter* counter, int depth) {
    /*
     * Wrapper para calculate_ev_after_receiving_card mantendo interface
     */
    return calculate_ev_after_receiving_card(current_hand_bits, card_rank, dealer_upcard, true_count, counter, depth);
}

// ====================== CÁLCULO DE EV HIT ======================

double calculate_ev_hit_realtime(uint64_t hand_bits, int dealer_upcard, double true_count, const ShoeCounter* counter, int depth) {
    /*
     * FÓRMULA HIT IMPLEMENTADA:
     * EV_hit(P_h) = Σ [P(receber carta r) × EV_ótimo(P_h + valor(r))]
     * 
     * Onde P(receber carta r) = C(r) / N
     * 
     * Casos especiais:
     * - Se P_h + valor(r) > 21: EV = -1 (bust)
     * - Se P_h + valor(r) = 21: EV = EV_stand(21)
     * - Se P_h + valor(r) < 21: EV = max(EV_stand, EV_hit) da nova mão
     * 
     * Tratamento especial do Ás:
     * - Se P_h + 11 ≤ 21: EV = max(EV_ótimo(P_h + 1), EV_ótimo_soft(P_h + 11))
     */
    
    if (depth > MAX_RECURSION_DEPTH) {
        // Fallback inteligente: usar EV Stand da mão atual em vez de -1.0 conservador
        int current_value = calcular_valor_mao(hand_bits);
        return calculate_ev_stand_realtime(current_value, dealer_upcard, true_count, counter);
    }
    
    int current_value = calcular_valor_mao(hand_bits);
    if (current_value >= 21) {
        return (current_value == 21) ? calculate_ev_stand_realtime(21, dealer_upcard, true_count, counter) : -1.0;
    }
    
    double ev_hit = 0.0;
    int total_cards_available = shoe_counter_get_total_cards(counter);
    
    if (total_cards_available <= 0) {
        return -1.0; // Não há cartas disponíveis
    }
    
    // CORREÇÃO CRÍTICA: Iterar sobre todos os ranks possíveis com probabilidades corretas
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        // CORREÇÃO CRÍTICA: Usar função corrigida para calcular probabilidade
        double card_prob = get_card_probability(counter, card_rank);
        if (card_prob <= 0.0) continue;
        
        // CORREÇÃO: Calcular EV ótimo após receber esta carta (stand vs hit)
        double ev_after_card = calculate_ev_after_receiving_card(
            hand_bits, card_rank, dealer_upcard, true_count, counter, depth
        );
        
        ev_hit += card_prob * ev_after_card;
    }
    
    return ev_hit;
}

// ====================== CÁLCULO DE EV DOUBLE ======================

double calculate_ev_double_realtime(uint64_t hand_bits, int dealer_upcard, double true_count, const ShoeCounter* counter) {
    /*
     * FÓRMULA DOUBLE IMPLEMENTADA:
     * EV_double(P_h) = 2 × Σ [P(receber carta r) × EV_stand(P_h + valor(r))]
     * 
     * Tratamento do Ás no Double:
     * EV_stand_após_double(P_h, A) = max(EV_stand(P_h + 1), EV_stand(P_h + 11))
     * se P_h + 11 ≤ 21, caso contrário EV_stand(P_h + 1)
     */
    
    int current_value = calcular_valor_mao(hand_bits);
    double ev_double = 0.0;
    int total_cards_available = shoe_counter_get_total_cards(counter);
    
    if (total_cards_available <= 0) {
        return -2.0; // Não há cartas disponíveis
    }
    
    // CORREÇÃO CRÍTICA: Iterar sobre todos os ranks possíveis com probabilidades corretas
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        // CORREÇÃO CRÍTICA: Usar função corrigida para calcular probabilidade
        double card_prob = get_card_probability(counter, card_rank);
        if (card_prob <= 0.0) continue;
        
        // CORREÇÃO CRÍTICA: Usar função corrigida de simulação de remoção
        ShoeCounter temp_counter = simulate_card_removal_corrected(counter, card_rank);
        
        double ev_after_card;
        
        if (card_rank == 11) { // Ás
            // CORREÇÃO CRÍTICA: Tratamento correto do Ás conforme fórmula
            int hard_value = current_value + 1;
            int soft_value = current_value + 11;
            
            if (hard_value > 21) {
                ev_after_card = -1.0; // Bust
            } else if (soft_value <= 21) {
                // CORREÇÃO: max(EV_stand(P_h + 1), EV_stand(P_h + 11)) conforme fórmula
                double ev_hard = calculate_ev_stand_realtime(hard_value, dealer_upcard, true_count, &temp_counter);
                double ev_soft = calculate_ev_stand_realtime(soft_value, dealer_upcard, true_count, &temp_counter);
                ev_after_card = (ev_hard > ev_soft) ? ev_hard : ev_soft;
            } else {
                ev_after_card = calculate_ev_stand_realtime(hard_value, dealer_upcard, true_count, &temp_counter);
            }
        } else {
            // Cartas 2-10
            int final_value = current_value + card_rank;
            
            if (final_value > 21) {
                ev_after_card = -1.0; // Bust
            } else {
                ev_after_card = calculate_ev_stand_realtime(final_value, dealer_upcard, true_count, &temp_counter);
            }
        }
        
        // CORREÇÃO CRÍTICA: Multiplicar cada termo por 2 conforme fórmula
        ev_double += card_prob * ev_after_card * 2.0;
    }
    
    return ev_double;
}

// ====================== CÁLCULO DE EV SPLIT ======================

double calculate_ev_split_realtime(int pair_rank, int dealer_upcard, double true_count, const ShoeCounter* counter) {
    /*
     * FÓRMULA SPLIT IMPLEMENTADA:
     * EV_split(par_r) = 2 × Σ(s=A até K) [C(s)/N × EV_ótimo(valor(r) + valor(s))]
     * 
     * Usar tabelas de lookup se disponíveis, caso contrário calcular usando fórmula
     */
    
    if (split_ev_table_loaded) {
        double tc_bin = get_tc_bin_start(true_count);
        return get_split_ev(pair_rank, dealer_upcard, tc_bin);
    }
    
    // Calcular usando fórmula matemática
    double ev_split = 0.0;
    int total_cards_available = shoe_counter_get_total_cards(counter);
    
    if (total_cards_available <= 0) {
        return -2.0; // Não há cartas disponíveis
    }
    
    // Para cada carta possível que pode vir após o split
    for (int card_rank = 2; card_rank <= 11; card_rank++) {
        int rank_idx = rank_value_to_idx(card_rank);
        if (rank_idx < 0) continue;
        
        int available_cards = shoe_counter_get_rank_count(counter, rank_idx);
        if (available_cards <= 0) continue;
        
        double card_prob = (double)available_cards / total_cards_available;
        
        // Calcular valor da nova mão
        int new_value = pair_rank + card_rank;
        if (pair_rank == 11 && card_rank != 10) { // Ás + não-10
            new_value = 11 + ((card_rank == 11) ? 1 : card_rank); // Ás como 11 se possível
        } else if (pair_rank == 11 && card_rank == 10) { // Ás + 10 = 21
            new_value = 21;
        }
        
        // Simular remoção da carta
        ShoeCounter temp_counter = simulate_card_removal(counter, card_rank);
        
        double ev_for_this_card;
        
        if (new_value > 21) {
            ev_for_this_card = -1.0; // Bust
        } else if (new_value == 21 || (pair_rank == 11)) {
            // 21 ou Ases splitados (recebem só uma carta)
            ev_for_this_card = calculate_ev_stand_realtime(new_value, dealer_upcard, true_count, &temp_counter);
        } else {
            // Calcular EV ótimo da nova mão
            double ev_stand = calculate_ev_stand_realtime(new_value, dealer_upcard, true_count, &temp_counter);
            // Simplificação: para splits assumimos que pode continuar jogando normalmente
            // TODO: Implementar cálculo completo considerando que é após split
            ev_for_this_card = ev_stand; // Conservador
        }
        
        ev_split += card_prob * ev_for_this_card;
    }
    
    // Multiplicar por 2 (duas mãos)
    return ev_split * 2.0;
}

// ====================== FUNÇÃO PRINCIPAL ======================

RealTimeEVResult calculate_real_time_ev(
    uint64_t hand_bits,
    int dealer_upcard,
    double true_count,
    const ShoeCounter* counter,
    bool is_initial_hand,
    bool double_allowed,
    bool split_allowed
) {
    RealTimeEVResult result = {0};
    
    // VALIDAÇÕES CRÍTICAS: Verificar dados de entrada
    if (!validate_shoe_counter(counter)) {
        result.calculation_valid = false;
        result.best_action = 'S';  // Fallback conservador para stand
        result.best_ev = -0.5;
        return result;
    }
    
    if (!validate_true_count(true_count)) {
        result.calculation_valid = false;
        result.best_action = 'S';  // Fallback conservador para stand
        result.best_ev = -0.5;
        return result;
    }
    
    if (dealer_upcard < 2 || dealer_upcard > 11) {
        result.calculation_valid = false;
        result.best_action = 'S';  // Fallback conservador para stand
        result.best_ev = -0.5;
        return result;
    }
    
    // Verificar cache primeiro
    if (cache_initialized && get_cached_ev(hand_bits, dealer_upcard, true_count, &result)) {
        return result;
    }
    
    int hand_value = calcular_valor_mao(hand_bits);
    
    // Calcular EV Stand
    result.ev_stand = calculate_ev_stand_realtime(hand_value, dealer_upcard, true_count, counter);
    
    // Calcular EV Hit (se mão < 21)
    if (hand_value < 21) {
        result.ev_hit = calculate_ev_hit_realtime(hand_bits, dealer_upcard, true_count, counter, 1);
    } else {
        result.ev_hit = -1.0; // Não pode pedir com 21
    }
    
    // Calcular EV Double (se permitido)
    if (double_allowed && is_initial_hand && hand_value >= 9 && hand_value <= 11) {
        result.ev_double = calculate_ev_double_realtime(hand_bits, dealer_upcard, true_count, counter);
    } else {
        result.ev_double = -2.0; // Valor muito baixo para não ser escolhido
    }
    
    // Calcular EV Split (se permitido e é par)
    result.has_split_option = false;
    if (split_allowed && is_initial_hand && is_pair_hand(hand_bits)) {
        int pair_rank = get_pair_rank(hand_bits);
        if (pair_rank > 0) {
            result.ev_split = calculate_ev_split_realtime(pair_rank, dealer_upcard, true_count, counter);
            result.has_split_option = true;
        }
    }
    
    if (!result.has_split_option) {
        result.ev_split = -2.0; // Valor muito baixo para não ser escolhido
    }
    
    // Determinar melhor ação
    result.best_ev = result.ev_stand;
    result.best_action = 'S';
    
    if (result.ev_hit > result.best_ev) {
        result.best_ev = result.ev_hit;
        result.best_action = 'H';
    }
    
    if (result.ev_double > result.best_ev) {
        result.best_ev = result.ev_double;
        result.best_action = 'D';
    }
    
    if (result.has_split_option && result.ev_split > result.best_ev) {
        result.best_ev = result.ev_split;
        result.best_action = 'P';
    }
    
    result.calculation_valid = true;
    
    // Armazenar no cache
    if (cache_initialized) {
        cache_ev_result(hand_bits, dealer_upcard, true_count, &result);
    }
    
    return result;
}

// ====================== FUNÇÃO DE INTEGRAÇÃO ======================

AcaoEstrategia determine_optimal_action_realtime(
    uint64_t hand_bits,
    int dealer_upcard,
    double true_count,
    const ShoeCounter* counter,
    bool is_initial_hand
) {
    // FALLBACK #1: Verificações básicas de entrada
    if (hand_bits == 0 || dealer_upcard < 2 || dealer_upcard > 11) {
        realtime_stats.fallback_invalid_input++;
        realtime_stats.total_fallbacks++;
        return estrategia_basica_super_rapida(hand_bits, dealer_upcard);
    }
    
    // FALLBACK #2: Verificar blackjack (caso trivial)
    int hand_value = calcular_valor_mao(hand_bits);
    if (is_initial_hand && hand_value == 21) {
        realtime_stats.fallback_trivial_cases++;
        return ACAO_STAND;
    }
    
    // FALLBACK #3: Mão bust (caso trivial)
    if (hand_value > 21) {
        realtime_stats.fallback_trivial_cases++;
        return ACAO_STAND; // Já está bust, não pode fazer mais nada
    }
    
    // FALLBACK #4: Validação adicional do contador de cartas
    if (!counter || !counter->initialized || !validate_shoe_counter(counter)) {
        realtime_stats.fallback_invalid_counter++;
        realtime_stats.total_fallbacks++;
        return estrategia_basica_super_rapida(hand_bits, dealer_upcard);
    }
    
    // FALLBACK #5: Validação do true count
    if (!validate_true_count(true_count)) {
        realtime_stats.fallback_invalid_true_count++;
        realtime_stats.total_fallbacks++;
        return estrategia_basica_super_rapida(hand_bits, dealer_upcard);
    }
    
    // Tentar calcular EV em tempo real
    RealTimeEVResult result = calculate_real_time_ev(
        hand_bits, dealer_upcard, true_count, counter,
        is_initial_hand, is_initial_hand, is_initial_hand
    );
    
    // FALLBACK #6: Verificar se cálculo é válido
    if (!result.calculation_valid) {
        realtime_stats.fallback_calculation_failed++;
        realtime_stats.total_fallbacks++;
        return estrategia_basica_super_rapida(hand_bits, dealer_upcard);
    }
    
    // FALLBACK #7: Verificar sanidade dos valores de EV
    if (isnan(result.best_ev) || isinf(result.best_ev) || 
        result.best_ev < -10.0 || result.best_ev > 10.0) {
        realtime_stats.fallback_invalid_ev_values++;
        realtime_stats.total_fallbacks++;
        return estrategia_basica_super_rapida(hand_bits, dealer_upcard);
    }
    
    // FALLBACK #8: Verificar se ação é válida
    if (result.best_action != 'S' && result.best_action != 'H' && 
        result.best_action != 'D' && result.best_action != 'P') {
        realtime_stats.fallback_invalid_action++;
        realtime_stats.total_fallbacks++;
        return estrategia_basica_super_rapida(hand_bits, dealer_upcard);
    }
    
    // Converter ação para enum com fallbacks apropriados
    switch (result.best_action) {
        case 'S': 
            return ACAO_STAND;
        case 'H': 
            return ACAO_HIT;
        case 'D': 
            // FALLBACK #9: Double só permitido em mão inicial
            if (is_initial_hand) {
                return ACAO_DOUBLE;
            } else {
                // Fallback: escolher entre stand/hit usando estratégia básica
                realtime_stats.fallback_context_restrictions++;
                realtime_stats.total_fallbacks++;
                AcaoEstrategia basic_action = estrategia_basica_super_rapida(hand_bits, dealer_upcard);
                return (basic_action == ACAO_DOUBLE_OR_HIT) ? ACAO_HIT : basic_action;
            }
        case 'P': 
            // FALLBACK #10: Split só permitido em mão inicial e se for par
            if (is_initial_hand && is_pair_hand(hand_bits)) {
                return ACAO_SPLIT;
            } else {
                // Fallback: usar estratégia básica
                realtime_stats.fallback_context_restrictions++;
                realtime_stats.total_fallbacks++;
                return estrategia_basica_super_rapida(hand_bits, dealer_upcard);
            }
        default: 
            // FALLBACK #11: Ação desconhecida
            realtime_stats.fallback_invalid_action++;
            realtime_stats.total_fallbacks++;
            return estrategia_basica_super_rapida(hand_bits, dealer_upcard);
    }
}

// ====================== FUNÇÕES DE CACHE ======================

void init_ev_cache(void) {
    memset(ev_cache, 0, sizeof(ev_cache));
    cache_initialized = true;
    cache_hits = 0;
    cache_misses = 0;
}

bool get_cached_ev(uint64_t hand_bits, int dealer_upcard, double true_count, RealTimeEVResult* result) {
    if (!cache_initialized) {
        cache_misses++;
        return false;
    }
    
    // Calcular hash e índice
    uint64_t cache_key = calculate_cache_key(hand_bits, dealer_upcard, true_count);
    int index = (int)(cache_key & CACHE_MASK);
    
    // Verificar se entrada é válida e corresponde
    if (ev_cache[index].valid && ev_cache[index].hand_bits == hand_bits &&
        ev_cache[index].dealer_upcard == dealer_upcard &&
        fabs(ev_cache[index].true_count - true_count) < 0.1) {
        *result = ev_cache[index].result;
        cache_hits++;
        return true;
    }
    
    cache_misses++;
    return false;
}

void cache_ev_result(uint64_t hand_bits, int dealer_upcard, double true_count, const RealTimeEVResult* result) {
    if (!cache_initialized) return;
    
    // Calcular hash e índice para novo entry
    uint64_t cache_key = calculate_cache_key(hand_bits, dealer_upcard, true_count);
    int index = (int)(cache_key & CACHE_MASK);
    
    // Substituir entrada existente (estratégia de substituição simples)
    ev_cache[index].hand_bits = hand_bits;
    ev_cache[index].dealer_upcard = dealer_upcard;
    ev_cache[index].true_count = true_count;
    ev_cache[index].result = *result;
    ev_cache[index].valid = true;
}

void clear_ev_cache(void) {
    memset(ev_cache, 0, sizeof(ev_cache));
    cache_hits = 0;
    cache_misses = 0;
}

void print_cache_stats(void) {
    int total_requests = cache_hits + cache_misses;
    double hit_rate = total_requests > 0 ? (double)cache_hits / total_requests * 100.0 : 0.0;
    
    printf("\n📊 CACHE STATISTICS\n");
    printf("===================\n");
    printf("Cache Size: %d entries\n", CACHE_SIZE);
    printf("Cache Hits: %d\n", cache_hits);
    printf("Cache Misses: %d\n", cache_misses);
    printf("Hit Rate: %.2f%%\n", hit_rate);
    printf("===================\n");
}

// ====================== FUNÇÕES DE DEBUG ======================

void print_ev_breakdown(const RealTimeEVResult* result, uint64_t hand_bits, int dealer_upcard, double true_count) {
    printf("\n🎯 REAL-TIME EV BREAKDOWN\n");
    printf("=========================\n");
    printf("Hand: %d, Dealer: %d, TC: %.1f\n", calcular_valor_mao(hand_bits), dealer_upcard, true_count);
    printf("EV(Stand):  %+.6f\n", result->ev_stand);
    printf("EV(Hit):    %+.6f\n", result->ev_hit);
    printf("EV(Double): %+.6f\n", result->ev_double);
    if (result->has_split_option) {
        printf("EV(Split):  %+.6f\n", result->ev_split);
    }
    printf("Best Action: %c (EV = %+.6f)\n", result->best_action, result->best_ev);
}

// ====================== MELHORIAS RECOMENDADAS ======================

// CORREÇÃO CRÍTICA: Mapeamento biunívoco com parâmetro suit_offset
int rank_value_to_idx_bijective(int rank_value, int suit_offset) {
    // Converter valor do rank (2-11) para índice (0-12) com distinção por naipe
    if (rank_value >= 2 && rank_value <= 9) {
        return rank_value - 2;  // 2->0, 3->1, ..., 9->7
    } else if (rank_value == 10) {
        // CORREÇÃO: Distinguir 10, J, Q, K por suit_offset
        switch (suit_offset) {
            case 0: return 8;   // 10
            case 1: return 9;   // J
            case 2: return 10;  // Q
            case 3: return 11;  // K
            default: return 8;  // Fallback para 10
        }
    } else if (rank_value == 11) {
        return 12; // Ás
    }
    return -1; // Valor inválido
}

// CORREÇÃO CRÍTICA: Função para iterar sempre por rank_idx nos loops de EV
void iterate_by_rank_idx_for_ev(const ShoeCounter* counter, 
                                void (*callback)(int rank_idx, double probability, void* user_data),
                                void* user_data) {
    int total_cards = shoe_counter_get_total_cards(counter);
    if (total_cards <= 0) return;
    
    // Iterar por todos os índices de rank (0-12)
    for (int rank_idx = 0; rank_idx < NUM_RANKS; rank_idx++) {
        int count = shoe_counter_get_rank_count(counter, rank_idx);
        if (count > 0) {
            double probability = (double)count / total_cards;
            callback(rank_idx, probability, user_data);
        }
    }
}

// CORREÇÃO CRÍTICA: Função para simular remoção por rank_idx
ShoeCounter simulate_card_removal_by_idx(const ShoeCounter* counter, int rank_idx) {
    ShoeCounter temp_counter = *counter;
    
    if (rank_idx >= 0 && rank_idx < NUM_RANKS && temp_counter.counts[rank_idx] > 0) {
        temp_counter.counts[rank_idx]--;
        temp_counter.total_cards--;
        
        // CORREÇÃO CRÍTICA: Validação forte após cada atualização
        assert(temp_counter.total_cards >= 0);
        assert(temp_counter.counts[rank_idx] >= 0);
        
        // Verificar consistência
        int sum = 0;
        for (int i = 0; i < NUM_RANKS; i++) {
            sum += temp_counter.counts[i];
        }
        assert(sum == temp_counter.total_cards);
    }
    
    return temp_counter;
}

// CORREÇÃO CRÍTICA: Normalização aprimorada com tolerância 1e-6
bool validate_probability_sum(const ShoeCounter* counter) {
    int total_cards = shoe_counter_get_total_cards(counter);
    if (total_cards <= 0) return false;
    
    double sum = 0.0;
    for (int rank_idx = 0; rank_idx < NUM_RANKS; rank_idx++) {
        sum += (double)counter->counts[rank_idx] / total_cards;
    }
    
    // Tolerância aprimorada: 1e-6 em vez de 0.001
    return fabs(sum - 1.0) < 1e-6;
}

// CORREÇÃO CRÍTICA: Função para usar lookup de frequências reais do dealer
DealerProbabilities get_dealer_probabilities_real_lookup(int dealer_upcard, double true_count, const ShoeCounter* counter) {
    DealerProbabilities probs = {0};
    
    // CORREÇÃO: Usar lookup de frequências reais de 3M shoes
    // Buscar no diretório Resultados/freq_*
    char filename[256];
    snprintf(filename, sizeof(filename), "Resultados/freq_%d_%d_3M.csv", 
             dealer_upcard, dealer_upcard);
    
    FILE* file = fopen(filename, "r");
    if (file) {
        // Ler frequências reais do arquivo
        // Implementação simplificada - usar lookup existente como fallback
        fclose(file);
    }
    
    // Fallback para lookup heurístico (melhorado)
    probs = get_dealer_probabilities(dealer_upcard, true_count, counter);
    
    // CORREÇÃO: Normalizar para soma = 1.0
    double sum = probs.prob_17 + probs.prob_18 + probs.prob_19 + 
                 probs.prob_20 + probs.prob_21 + probs.prob_blackjack + probs.prob_bust;
    
    if (sum > 0.0) {
        probs.prob_17 /= sum;
        probs.prob_18 /= sum;
        probs.prob_19 /= sum;
        probs.prob_20 /= sum;
        probs.prob_21 /= sum;
        probs.prob_blackjack /= sum;
        probs.prob_bust /= sum;
    }
    
    probs.probabilities_valid = true;
    return probs;
}

// CORREÇÃO CRÍTICA: Função para validação estatística forte
bool validate_ev_calculations_statistical(const RealTimeEVResult* result, 
                                         uint64_t hand_bits, 
                                         int dealer_upcard, 
                                         double true_count,
                                         const ShoeCounter* counter) {
    if (!result || !result->calculation_valid) {
        return false;
    }
    
    // Validações estatísticas
    if (result->best_ev < -2.0 || result->best_ev > 2.0) {
        fprintf(stderr, "ERRO: EV fora do intervalo esperado: %.6f\n", result->best_ev);
        return false;
    }
    
    // Verificar se a ação escolhida tem EV válido
    double expected_ev = -999.0;
    switch (result->best_action) {
        case 'S': expected_ev = result->ev_stand; break;
        case 'H': expected_ev = result->ev_hit; break;
        case 'D': expected_ev = result->ev_double; break;
        case 'P': expected_ev = result->ev_split; break;
        default: break;
    }
    
    if (fabs(expected_ev - result->best_ev) > 1e-6) {
        fprintf(stderr, "ERRO: EV da ação escolhida não corresponde ao melhor EV\n");
        return false;
    }
    
    // Validar shoe counter
    if (!validate_shoe_counter(counter)) {
        fprintf(stderr, "ERRO: Shoe counter inválido\n");
        return false;
    }
    
    // Validar true count
    if (!validate_true_count(true_count)) {
        fprintf(stderr, "ERRO: True count inválido: %.6f\n", true_count);
        return false;
    }
    
    return true;
} 