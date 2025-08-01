#include "structures.h"
#include "tabela_estrategia.h"
#include <stdlib.h>
#include <string.h>

// Estrutura global de perfect hash
PerfectHashStrategy perfect_hash_strategy = {0};

// Tabela de estratégias básicas para perfect hash
static const uint8_t basic_strategy_table[][10] = {
    // Hard hands: 5-21 vs 2-10,A
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 5: HIT
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 6: HIT
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 7: HIT
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 8: HIT
    {0, 1, 1, 1, 1, 0, 0, 0, 0, 0}, // 9: HIT vs 2, DOUBLE vs 3-6, HIT vs 7-A
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0}, // 10: DOUBLE vs 2-9, HIT vs 10-A
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, // 11: DOUBLE vs 2-10, HIT vs A
    {0, 0, 2, 2, 2, 0, 0, 0, 0, 0}, // 12: HIT vs 2-3, STAND vs 4-6, HIT vs 7-A
    {2, 2, 2, 2, 2, 0, 0, 0, 0, 0}, // 13: STAND vs 2-6, HIT vs 7-A
    {2, 2, 2, 2, 2, 0, 0, 0, 0, 0}, // 14: STAND vs 2-6, HIT vs 7-A
    {2, 2, 2, 2, 2, 0, 0, 0, 0, 0}, // 15: STAND vs 2-6, HIT vs 7-A
    {2, 2, 2, 2, 2, 0, 0, 0, 0, 0}, // 16: STAND vs 2-6, HIT vs 7-A
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 17: STAND
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 18: STAND
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 19: STAND
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 20: STAND
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 21: STAND
};

// Tabela de soft hands: A2-A9 vs 2-10,A
static const uint8_t soft_strategy_table[][10] = {
    {0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // A2: HIT vs 2-3, DOUBLE vs 4-5, HIT vs 6-A
    {0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // A3: HIT vs 2-3, DOUBLE vs 4-5, HIT vs 6-A
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0}, // A4: HIT vs 2, DOUBLE vs 3-5, HIT vs 6-A
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0}, // A5: HIT vs 2, DOUBLE vs 3-5, HIT vs 6-A
    {0, 1, 1, 1, 1, 0, 0, 0, 0, 0}, // A6: HIT vs 2, DOUBLE vs 3-6, HIT vs 7-A
    {2, 1, 1, 1, 1, 2, 2, 0, 0, 0}, // A7: STAND vs 2, DOUBLE vs 3-6, STAND vs 7-8, HIT vs 9-A
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // A8: STAND
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // A9: STAND
};

// Tabela de pares: 22-AA vs 2-10,A
static const uint8_t pair_strategy_table[][10] = {
    {3, 3, 3, 3, 3, 3, 0, 0, 0, 0}, // 22: SPLIT vs 2-7, HIT vs 8-A
    {3, 3, 3, 3, 3, 3, 0, 0, 0, 0}, // 33: SPLIT vs 2-7, HIT vs 8-A
    {0, 0, 0, 3, 3, 0, 0, 0, 0, 0}, // 44: HIT vs 2-3, SPLIT vs 4-5, HIT vs 6-A
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0}, // 55: DOUBLE vs 2-9, HIT vs 10-A
    {3, 3, 3, 3, 3, 0, 0, 0, 0, 0}, // 66: SPLIT vs 2-6, HIT vs 7-A
    {3, 3, 3, 3, 3, 3, 0, 0, 0, 0}, // 77: SPLIT vs 2-7, HIT vs 8-A
    {3, 3, 3, 3, 3, 3, 3, 3, 3, 0}, // 88: SPLIT vs 2-10, HIT vs A
    {3, 3, 3, 3, 3, 2, 3, 3, 2, 2}, // 99: SPLIT vs 2-6, STAND vs 7, SPLIT vs 8-9, STAND vs 10-A
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // 1010: STAND
    {3, 3, 3, 3, 3, 3, 3, 3, 3, 3}, // AA: SPLIT
};

// Inicializar sistema de perfect hash
bool perfect_hash_init(void) {
    DEBUG_IO("Inicializando sistema de perfect hash");
    
    memset(&perfect_hash_strategy, 0, sizeof(PerfectHashStrategy));
    
    // Inicializar tabela de hash com valores inválidos
    for (int i = 0; i < 4096; i++) {
        perfect_hash_strategy.hash_table[i] = 0xFFFFFFFF;
        perfect_hash_strategy.strategy_table[i] = 0; // HIT como padrão
    }
    
    int collisions = 0;
    int total_entries = 0;
    
    // Populá-la com hard hands (5-21)
    for (int value = 5; value <= 21; value++) {
        for (int dealer = 2; dealer <= 11; dealer++) {
            // Simular mão hard com valor específico
            uint64_t hand_bits = 0;
            
            // Criar mão que resulta no valor desejado
            if (value <= 10) {
                // Usar cartas simples
                for (int i = 0; i < value - 2; i++) {
                    hand_bits += (1ULL << (0 * 3)); // Adicionar Ás como 1
                }
                hand_bits += (1ULL << (1 * 3)); // Adicionar um 2
            } else {
                // Usar 10 + cartas menores
                hand_bits += (1ULL << (8 * 3)); // Adicionar 10
                int remaining = value - 10;
                for (int i = 0; i < remaining - 2; i++) {
                    hand_bits += (1ULL << (0 * 3)); // Adicionar Ás como 1
                }
                if (remaining >= 2) {
                    hand_bits += (1ULL << (1 * 3)); // Adicionar um 2
                }
            }
            
            uint32_t hash = hash_hand_key(hand_bits, dealer);
            
            // Verificar colisão
            if (perfect_hash_strategy.hash_table[hash] != 0xFFFFFFFF) {
                collisions++;
                DEBUG_STATS("Colisão hash detectada: valor=%d, dealer=%d, hash=%u", 
                           value, dealer, hash);
                continue;
            }
            
            perfect_hash_strategy.hash_table[hash] = (uint32_t)((value << 8) | dealer);
            
            // Mapear para índice da tabela de estratégia
            int dealer_idx = (dealer == 11) ? 9 : dealer - 2;
            perfect_hash_strategy.strategy_table[hash] = basic_strategy_table[value - 5][dealer_idx];
            
            total_entries++;
        }
    }
    
    // Populá-la com soft hands (A2-A9)
    for (int soft_card = 2; soft_card <= 9; soft_card++) {
        for (int dealer = 2; dealer <= 11; dealer++) {
            // Simular mão soft: Ás + carta
            uint64_t hand_bits = (1ULL << (0 * 3)); // Ás
            hand_bits += (1ULL << ((soft_card - 1) * 3)); // Carta (2-9)
            
            uint32_t hash = hash_hand_key(hand_bits, dealer);
            
            // Verificar colisão
            if (perfect_hash_strategy.hash_table[hash] != 0xFFFFFFFF) {
                collisions++;
                DEBUG_STATS("Colisão hash detectada: soft A%d, dealer=%d, hash=%u", 
                           soft_card, dealer, hash);
                continue;
            }
            
            perfect_hash_strategy.hash_table[hash] = (uint32_t)((100 + soft_card) << 8) | dealer;
            
            // Mapear para índice da tabela de estratégia
            int dealer_idx = (dealer == 11) ? 9 : dealer - 2;
            perfect_hash_strategy.strategy_table[hash] = soft_strategy_table[soft_card - 2][dealer_idx];
            
            total_entries++;
        }
    }
    
    // Populá-la com pares (22-AA)
    for (int pair_rank = 0; pair_rank <= 12; pair_rank++) {
        for (int dealer = 2; dealer <= 11; dealer++) {
            // Simular par
            uint64_t hand_bits = (2ULL << (pair_rank * 3)); // 2 cartas do mesmo rank
            
            uint32_t hash = hash_hand_key(hand_bits, dealer);
            
            // Verificar colisão
            if (perfect_hash_strategy.hash_table[hash] != 0xFFFFFFFF) {
                collisions++;
                DEBUG_STATS("Colisão hash detectada: par %d%d, dealer=%d, hash=%u", 
                           pair_rank + 1, pair_rank + 1, dealer, hash);
                continue;
            }
            
            perfect_hash_strategy.hash_table[hash] = (uint32_t)((200 + pair_rank) << 8) | dealer;
            
            // Mapear para índice da tabela de estratégia
            int dealer_idx = (dealer == 11) ? 9 : dealer - 2;
            
            // Pares especiais
            if (pair_rank == 0) { // AA
                perfect_hash_strategy.strategy_table[hash] = pair_strategy_table[10][dealer_idx];
            } else if (pair_rank >= 8) { // 99, 1010, JJ, QQ, KK
                if (pair_rank == 8) { // 99
                    perfect_hash_strategy.strategy_table[hash] = pair_strategy_table[8][dealer_idx];
                } else { // 1010, JJ, QQ, KK
                    perfect_hash_strategy.strategy_table[hash] = pair_strategy_table[9][dealer_idx];
                }
            } else { // 22-88
                perfect_hash_strategy.strategy_table[hash] = pair_strategy_table[pair_rank - 1][dealer_idx];
            }
            
            total_entries++;
        }
    }
    
    perfect_hash_strategy.is_initialized = true;
    
    DEBUG_IO("Perfect hash inicializado: %d entradas, %d colisões (%.1f%%)", 
             total_entries, collisions, 
             total_entries > 0 ? (double)collisions * 100.0 / total_entries : 0.0);
    
    return true;
}

// Cleanup do sistema de perfect hash
void perfect_hash_cleanup(void) {
    DEBUG_IO("Limpando sistema de perfect hash");
    
    memset(&perfect_hash_strategy, 0, sizeof(PerfectHashStrategy));
    
    DEBUG_IO("Sistema de perfect hash limpo");
}

// Lookup usando perfect hash
uint8_t perfect_hash_lookup(uint64_t hand_bits, int dealer_upcard) {
    if (!perfect_hash_strategy.is_initialized) {
        DEBUG_IO("ERRO: Perfect hash não inicializado");
        return 0; // HIT como padrão
    }
    
    uint32_t hash = hash_hand_key(hand_bits, dealer_upcard);
    
    // Verificar se hash está na tabela
    if (perfect_hash_strategy.hash_table[hash] == 0xFFFFFFFF) {
        DEBUG_STATS("Hash não encontrado: %u para dealer %d", hash, dealer_upcard);
        return 0; // HIT como padrão
    }
    
    uint8_t strategy = perfect_hash_strategy.strategy_table[hash];
    
    DEBUG_STATS("Perfect hash lookup: hand=%llu, dealer=%d, hash=%u, strategy=%d", 
                (unsigned long long)hand_bits, dealer_upcard, hash, strategy);
    
    return strategy;
}

// Função para validar a tabela de perfect hash
bool perfect_hash_validate(void) {
    if (!perfect_hash_strategy.is_initialized) {
        DEBUG_IO("ERRO: Perfect hash não inicializado para validação");
        return false;
    }
    
    DEBUG_IO("Validando tabela de perfect hash");
    
    int valid_entries = 0;
    int invalid_entries = 0;
    
    for (int i = 0; i < 4096; i++) {
        if (perfect_hash_strategy.hash_table[i] != 0xFFFFFFFF) {
            valid_entries++;
            
            // Verificar se estratégia está em range válido
            uint8_t strategy = perfect_hash_strategy.strategy_table[i];
            if (strategy > 3) { // 0=HIT, 1=DOUBLE, 2=STAND, 3=SPLIT
                DEBUG_IO("ERRO: Estratégia inválida %d no índice %d", strategy, i);
                invalid_entries++;
            }
        }
    }
    
    double load_factor = (double)valid_entries / 4096.0;
    bool is_valid = (invalid_entries == 0);
    
    DEBUG_IO("Validação perfect hash: %d entradas válidas, %d inválidas, load factor %.3f, válido: %s",
             valid_entries, invalid_entries, load_factor, is_valid ? "SIM" : "NÃO");
    
    return is_valid;
}

// Função para obter estatísticas da tabela de perfect hash
void perfect_hash_stats(void) {
    if (!perfect_hash_strategy.is_initialized) {
        DEBUG_IO("Perfect hash não inicializado");
        return;
    }
    
    DEBUG_IO("=== ESTATÍSTICAS PERFECT HASH ===");
    
    int total_entries = 0;
    int strategy_counts[4] = {0}; // HIT, DOUBLE, STAND, SPLIT
    
    for (int i = 0; i < 4096; i++) {
        if (perfect_hash_strategy.hash_table[i] != 0xFFFFFFFF) {
            total_entries++;
            
            uint8_t strategy = perfect_hash_strategy.strategy_table[i];
            if (strategy < 4) {
                strategy_counts[strategy]++;
            }
        }
    }
    
    DEBUG_IO("Total de entradas: %d", total_entries);
    DEBUG_IO("HIT: %d (%.1f%%)", strategy_counts[0], 
             total_entries > 0 ? (double)strategy_counts[0] * 100.0 / total_entries : 0.0);
    DEBUG_IO("DOUBLE: %d (%.1f%%)", strategy_counts[1], 
             total_entries > 0 ? (double)strategy_counts[1] * 100.0 / total_entries : 0.0);
    DEBUG_IO("STAND: %d (%.1f%%)", strategy_counts[2], 
             total_entries > 0 ? (double)strategy_counts[2] * 100.0 / total_entries : 0.0);
    DEBUG_IO("SPLIT: %d (%.1f%%)", strategy_counts[3], 
             total_entries > 0 ? (double)strategy_counts[3] * 100.0 / total_entries : 0.0);
    DEBUG_IO("Load factor: %.3f", (double)total_entries / 4096.0);
    DEBUG_IO("================================");
} 