#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>

// Constantes unificadas para batch sizes
#define DEALER_BATCH_SIZE 20000     // Padronizado: 20k simulações por lote
#define FREQ_BATCH_SIZE 20000       // Padronizado: 20k simulações por lote  
#define SPLIT_BATCH_SIZE 20000      // Padronizado: 20k simulações por lote

// Constantes para análise
#define MAX_BINS 130               // -6.5 a 6.5 com bins de 0.1 = 130 bins
#define BIN_WIDTH 0.1
#define MIN_TC -6.5
#define MAX_TC 6.5

// Prefixos para arquivos temporários
#define DEALER_TEMP_FILE_PREFIX "temp_dealer_bj_batch_"
#define FREQ_TEMP_FILE_PREFIX "temp_freq_batch_"
#define SPLIT_TEMP_FILE_PREFIX "temp_split_batch_"
#define BINARY_SUFFIX ".bin"

// Buffer sizes seguros
#define FREQ_BUFFER_SIZE 5000
#define FREQ_BUFFER_THRESHOLD (FREQ_BUFFER_SIZE - 100)  // Flush antes do overflow
#define DEALER_BUFFER_SIZE 2000
#define DEALER_BUFFER_THRESHOLD (DEALER_BUFFER_SIZE - 50)

// Debug system
extern bool debug_enabled;
#define DEBUG_PRINT(fmt, ...) do { \
    if (debug_enabled) { \
        printf("[DEBUG] " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout); \
    } \
} while(0)

#define DEBUG_STATS(fmt, ...) do { \
    if (debug_enabled) { \
        printf("[STATS] " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout); \
    } \
} while(0)

#define DEBUG_IO(fmt, ...) do { \
    if (debug_enabled) { \
        printf("[IO] " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout); \
    } \
} while(0)

#define DEBUG_MUTEX(fmt, ...) do { \
    if (debug_enabled) { \
        printf("[MUTEX] " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout); \
    } \
} while(0)

// Estrutura para dados de dealer (12 bytes)
typedef struct {
    float true_count;      // 4 bytes
    int32_t ace_upcard;    // 4 bytes  
    int32_t dealer_bj;     // 4 bytes
    uint32_t checksum;     // 4 bytes - para integridade
} __attribute__((packed)) DealerBinaryRecord;  // 16 bytes total

// Estrutura para dados de frequência (8 bytes)
typedef struct {
    float true_count;      // 4 bytes
    uint32_t checksum;     // 4 bytes - para integridade
} __attribute__((packed)) FreqBinaryRecord;    // 8 bytes total

// Estrutura para dados de split (48 bytes)
typedef struct {
    float true_count;      // 4 bytes
    // Combinações reais de resultados (não assumir independência)
    int32_t lose_lose;     // 4 bytes - ambas mãos perderam
    int32_t win_win;       // 4 bytes - ambas mãos ganharam
    int32_t push_push;     // 4 bytes - ambas mãos empataram
    int32_t lose_win;      // 4 bytes - mão1 perdeu, mão2 ganhou
    int32_t lose_push;     // 4 bytes - mão1 perdeu, mão2 empatou
    int32_t win_lose;      // 4 bytes - mão1 ganhou, mão2 perdeu
    int32_t win_push;      // 4 bytes - mão1 ganhou, mão2 empatou
    int32_t push_lose;     // 4 bytes - mão1 empatou, mão2 perdeu
    int32_t push_win;      // 4 bytes - mão1 empatou, mão2 ganhou
    int32_t cards_used;    // 4 bytes - cartas usadas
    uint32_t checksum;     // 4 bytes - para integridade
} __attribute__((packed)) SplitBinaryRecord;   // 48 bytes total

// Funções de utilidade para checksum (safe para strict aliasing)
static inline uint32_t calculate_dealer_checksum(const DealerBinaryRecord* record) {
    uint32_t checksum = 0;
    uint32_t float_as_uint;
    memcpy(&float_as_uint, &record->true_count, sizeof(float));
    checksum ^= float_as_uint;
    checksum ^= (uint32_t)record->ace_upcard;
    checksum ^= (uint32_t)record->dealer_bj;
    return checksum;
}

static inline uint32_t calculate_freq_checksum(const FreqBinaryRecord* record) {
    uint32_t float_as_uint;
    memcpy(&float_as_uint, &record->true_count, sizeof(float));
    return float_as_uint;
}

static inline uint32_t calculate_split_checksum(const SplitBinaryRecord* record) {
    uint32_t checksum = 0;
    uint32_t float_as_uint;
    memcpy(&float_as_uint, &record->true_count, sizeof(float));
    checksum ^= float_as_uint;
    checksum ^= (uint32_t)record->lose_lose;
    checksum ^= (uint32_t)record->win_win;
    checksum ^= (uint32_t)record->push_push;
    checksum ^= (uint32_t)record->lose_win;
    checksum ^= (uint32_t)record->lose_push;
    checksum ^= (uint32_t)record->win_lose;
    checksum ^= (uint32_t)record->win_push;
    checksum ^= (uint32_t)record->push_lose;
    checksum ^= (uint32_t)record->push_win;
    checksum ^= (uint32_t)record->cards_used;
    return checksum;
}

// Função robusta para cálculo de bins
static inline int get_bin_index_robust(double true_count) {
    DEBUG_STATS("Calculando bin para TC=%.6f", true_count);
    
    if (true_count < MIN_TC - 1e-10) {
        DEBUG_STATS("TC %.6f abaixo do mínimo %.2f", true_count, MIN_TC);
        return -1;
    }
    if (true_count >= MAX_TC + 1e-10) {
        DEBUG_STATS("TC %.6f acima do máximo %.2f, usando último bin", true_count, MAX_TC);
        return MAX_BINS - 1;
    }
    
    int bin_idx = (int)((true_count - MIN_TC) / BIN_WIDTH);
    
    // Verificação adicional de segurança
    if (bin_idx < 0 || bin_idx >= MAX_BINS) {
        DEBUG_STATS("Bin calculado %d inválido para TC=%.6f, corrigindo", bin_idx, true_count);
        return (bin_idx < 0) ? 0 : MAX_BINS - 1;
    }
    
    DEBUG_STATS("TC=%.6f -> bin=%d (%.2f a %.2f)", 
                true_count, bin_idx, 
                MIN_TC + bin_idx * BIN_WIDTH, 
                MIN_TC + (bin_idx + 1) * BIN_WIDTH);
    
    return bin_idx;
}

// Funções de validação
static inline bool validate_dealer_record(const DealerBinaryRecord* record) {
    if (!record) return false;
    
    // Verificar checksum
    uint32_t expected_checksum = calculate_dealer_checksum(record);
    if (record->checksum != expected_checksum) {
        DEBUG_STATS("Checksum inválido para dealer record: esperado=%u, encontrado=%u", 
                   expected_checksum, record->checksum);
        return false;
    }
    
    // Verificar ranges
    if (record->true_count < -15.0 || record->true_count > 15.0) {
        DEBUG_STATS("True count inválido: %.3f", record->true_count);
        return false;
    }
    
    if (record->ace_upcard != 0 && record->ace_upcard != 1) {
        DEBUG_STATS("Ace upcard inválido: %d", record->ace_upcard);
        return false;
    }
    
    if (record->dealer_bj != 0 && record->dealer_bj != 1) {
        DEBUG_STATS("Dealer BJ inválido: %d", record->dealer_bj);
        return false;
    }
    
    return true;
}

static inline bool validate_freq_record(const FreqBinaryRecord* record) {
    if (!record) return false;
    
    // Verificar checksum
    uint32_t expected_checksum = calculate_freq_checksum(record);
    if (record->checksum != expected_checksum) {
        DEBUG_STATS("Checksum inválido para freq record: esperado=%u, encontrado=%u", 
                   expected_checksum, record->checksum);
        return false;
    }
    
    // Verificar ranges
    if (record->true_count < -15.0 || record->true_count > 15.0) {
        DEBUG_STATS("True count inválido: %.3f", record->true_count);
        return false;
    }
    
    return true;
}

static inline bool validate_split_record(const SplitBinaryRecord* record) {
    if (!record) return false;
    
    // Verificar checksum
    uint32_t expected_checksum = calculate_split_checksum(record);
    if (record->checksum != expected_checksum) {
        DEBUG_STATS("Checksum inválido para split record: esperado=%u, encontrado=%u", 
                   expected_checksum, record->checksum);
        return false;
    }
    
    // Verificar ranges básicos
    if (record->true_count < -15.0 || record->true_count > 15.0) {
        DEBUG_STATS("True count inválido: %.3f", record->true_count);
        return false;
    }
    
    if (record->cards_used < 4 || record->cards_used > 20) {
        DEBUG_STATS("Cards used inválido: %d", record->cards_used);
        return false;
    }
    
    // Verificar se todos os campos de combinação são 0 ou 1 (sem usar ponteiros)
    int32_t combinations[9] = {
        record->lose_lose, record->win_win, record->push_push,
        record->lose_win, record->lose_push, record->win_lose,
        record->win_push, record->push_lose, record->push_win
    };
    
    int total_combinations = 0;
    for (int i = 0; i < 9; i++) {
        if (combinations[i] < 0 || combinations[i] > 1) {
            DEBUG_STATS("Combinação %d inválida: %d", i, combinations[i]);
            return false;
        }
        total_combinations += combinations[i];
    }
    
    // Exatamente uma combinação deve ser 1
    if (total_combinations != 1) {
        DEBUG_STATS("Total de combinações inválido: %d (deve ser 1)", total_combinations);
        return false;
    }
    
    return true;
}

// Função para verificar integridade de arquivo
static inline bool verify_file_integrity(const char* filename, size_t expected_record_size) {
    DEBUG_IO("Verificando integridade do arquivo: %s", filename);
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        DEBUG_IO("Não foi possível abrir arquivo: %s", filename);
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fclose(file);
    
    if (file_size < 0) {
        DEBUG_IO("Erro ao obter tamanho do arquivo: %s", filename);
        return false;
    }
    
    if (file_size % expected_record_size != 0) {
        DEBUG_IO("Arquivo %s tem tamanho incompatível: %ld bytes, esperado múltiplo de %zu", 
                filename, file_size, expected_record_size);
        return false;
    }
    
    long num_records = file_size / expected_record_size;
    DEBUG_IO("Arquivo %s verificado: %ld registros de %zu bytes cada", 
            filename, num_records, expected_record_size);
    
    return true;
}

// =============================================================================
// SISTEMA DE MEMORY POOLS OTIMIZADO
// =============================================================================

// Pool de memory para diferentes tipos de alocações
typedef struct {
    void* memory;
    size_t size;
    size_t used;
    size_t align;
    bool is_active;
    pthread_mutex_t mutex;
} MemoryPool;

// Definições dos pools
#define POOL_MAOS_SIZE (1024 * 1024)      // 1MB para mãos
#define POOL_BUFFERS_SIZE (512 * 1024)     // 512KB para buffers diversos
#define POOL_THREADS_SIZE (64 * 1024)      // 64KB para dados de threads
#define POOL_TEMP_SIZE (256 * 1024)        // 256KB para alocações temporárias

// IDs dos pools
typedef enum {
    POOL_MAOS = 0,
    POOL_BUFFERS = 1,
    POOL_THREADS = 2,
    POOL_TEMP = 3,
    POOL_COUNT = 4
} PoolID;

// Estrutura global de pools (thread-safe)
extern MemoryPool global_pools[POOL_COUNT];

// Funções do sistema de memory pools
bool memory_pools_init(void);
void memory_pools_cleanup(void);
void* pool_alloc(PoolID pool_id, size_t size);
void pool_free(PoolID pool_id, void* ptr);
void pool_reset(PoolID pool_id);
size_t pool_get_usage(PoolID pool_id);

// Macros para facilitar o uso
#define POOL_ALLOC_MAOS(size) pool_alloc(POOL_MAOS, size)
#define POOL_ALLOC_BUFFERS(size) pool_alloc(POOL_BUFFERS, size)
#define POOL_ALLOC_THREADS(size) pool_alloc(POOL_THREADS, size)
#define POOL_ALLOC_TEMP(size) pool_alloc(POOL_TEMP, size)

#define POOL_FREE_MAOS(ptr) pool_free(POOL_MAOS, ptr)
#define POOL_FREE_BUFFERS(ptr) pool_free(POOL_BUFFERS, ptr)
#define POOL_FREE_THREADS(ptr) pool_free(POOL_THREADS, ptr)
#define POOL_FREE_TEMP(ptr) pool_free(POOL_TEMP, ptr)

// =============================================================================
// OTIMIZAÇÕES SIMD
// =============================================================================

#ifdef __SSE2__
#include <emmintrin.h>
#define SIMD_ENABLED 1
#else
#define SIMD_ENABLED 0
#endif

// Estrutura para operações SIMD otimizadas
typedef struct {
    bool simd_available;
    int vector_width;
    void (*count_cards_simd)(uint64_t* cards, int count, double* result);
    void (*evaluate_hands_simd)(uint64_t* hands, int count, int* results);
} SIMDContext;

extern SIMDContext simd_ctx;

// Funções SIMD
bool simd_init(void);
void simd_cleanup(void);

#if SIMD_ENABLED
// Funções SIMD específicas
void simd_count_cards(uint64_t* cards, int count, double* result);
void simd_evaluate_hands(uint64_t* hands, int count, int* results);
#endif

// =============================================================================
// PERFECT HASHING PARA ESTRATÉGIA BÁSICA
// =============================================================================

// Estrutura para perfect hash da estratégia básica
typedef struct {
    uint32_t hash_table[4096];  // Tabela de hash com 4096 entradas
    uint8_t strategy_table[4096]; // Estratégias correspondentes
    bool is_initialized;
} PerfectHashStrategy;

extern PerfectHashStrategy perfect_hash_strategy;

// Funções de perfect hashing
bool perfect_hash_init(void);
void perfect_hash_cleanup(void);
uint8_t perfect_hash_lookup(uint64_t hand_bits, int dealer_upcard);

// Função de hash otimizada
static inline uint32_t hash_hand_key(uint64_t hand_bits, int dealer_upcard) {
    // Hash rápido usando bit manipulation
    uint32_t key = (uint32_t)(hand_bits ^ (hand_bits >> 32));
    key ^= (uint32_t)dealer_upcard << 16;
    key ^= key >> 16;
    key ^= key >> 8;
    return key & 0xFFF; // Máscara para 4096 entradas
}

// =============================================================================
// MEMORY-MAPPED FILES PARA I/O OTIMIZADO
// =============================================================================

#ifdef __linux__
#include <sys/mman.h>
#define MMAP_ENABLED 1
#else
#define MMAP_ENABLED 0
#endif

// Estrutura para gerenciar arquivos memory-mapped
typedef struct {
    void* data;
    size_t size;
    int fd;
    bool is_mapped;
    bool read_only;
    char filepath[512];
} MappedFile;

// Configurações para memory-mapped files
#define MMAP_MIN_SIZE (1024 * 1024)  // 1MB mínimo para usar mmap
#define MAX_MAPPED_FILES 32          // Máximo de arquivos mapeados simultaneamente

// Estrutura para gerenciar múltiplos arquivos mapeados
typedef struct {
    MappedFile files[MAX_MAPPED_FILES];
    int active_count;
    pthread_mutex_t mutex;
    bool is_initialized;
} MappedFileManager;

extern MappedFileManager mmap_manager;

// Funções de memory-mapped files
bool mmap_init(void);
void mmap_cleanup(void);
MappedFile* mmap_open_file(const char* filepath, bool read_only, size_t expected_size);
void mmap_close_file(MappedFile* mapped_file);
bool mmap_sync_file(MappedFile* mapped_file);
void* mmap_get_data(MappedFile* mapped_file, size_t offset);
size_t mmap_write_data(MappedFile* mapped_file, size_t offset, const void* data, size_t size);
size_t mmap_read_data(MappedFile* mapped_file, size_t offset, void* buffer, size_t size);

// Macros para facilitar o uso
#define MMAP_OPEN_READ(filepath, size) mmap_open_file(filepath, true, size)
#define MMAP_OPEN_WRITE(filepath, size) mmap_open_file(filepath, false, size)
#define MMAP_CLOSE(mapped_file) mmap_close_file(mapped_file)
#define MMAP_SYNC(mapped_file) mmap_sync_file(mapped_file)

// =============================================================================
// WORK STEALING PARA BALANCEAMENTO DINÂMICO
// =============================================================================

// Estrutura para task de work stealing
typedef struct {
    int start_sim;
    int end_sim;
    volatile bool is_taken;
    volatile bool is_completed;
} WorkTask;

// Estrutura para work stealing queue
typedef struct {
    WorkTask* tasks;
    volatile int head;
    volatile int tail;
    int capacity;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    bool shutdown;
} WorkStealingQueue;

// Estrutura para worker thread context
typedef struct {
    int worker_id;
    int num_workers;
    WorkStealingQueue* local_queue;
    WorkStealingQueue** all_queues;
    
    // Dados da simulação
    int log_level;
    const char* output_suffix;
    atomic_int* global_log_count;
    bool dealer_analysis;
    bool freq_analysis_26;
    bool freq_analysis_70;
    bool freq_analysis_A;
    bool split_analysis;
    pthread_mutex_t* dealer_mutex;
    pthread_mutex_t* freq_mutex;
    pthread_mutex_t* split_mutex;
    
    // Estatísticas do worker
    atomic_int tasks_completed;
    atomic_int tasks_stolen;
    atomic_int tasks_given;
} WorkerContext;

// Estrutura global para work stealing
typedef struct {
    WorkStealingQueue* queues;
    WorkerContext* workers;
    int num_workers;
    bool is_initialized;
    pthread_t* threads;
} WorkStealingSystem;

extern WorkStealingSystem work_stealing_system;

// Funções de work stealing
bool work_stealing_init(int num_workers);
void work_stealing_cleanup(void);
bool work_stealing_add_task(int worker_id, int start_sim, int end_sim);
WorkTask* work_stealing_get_task(int worker_id);
WorkTask* work_stealing_steal_task(int thief_id, int victim_id);
void work_stealing_complete_task(WorkTask* task);
void work_stealing_print_stats(void);

// Função principal do worker
void* work_stealing_worker_thread(void* arg);

#endif // STRUCTURES_H 