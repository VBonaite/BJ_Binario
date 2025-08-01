#include "structures.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

// Estrutura global de pools (thread-safe)
MemoryPool global_pools[POOL_COUNT];

// Configurações dos pools
static const size_t pool_sizes[POOL_COUNT] = {
    POOL_MAOS_SIZE,
    POOL_BUFFERS_SIZE,
    POOL_THREADS_SIZE,
    POOL_TEMP_SIZE
};

static const size_t pool_alignments[POOL_COUNT] = {
    64,  // POOL_MAOS - alinhamento de cache line
    32,  // POOL_BUFFERS - alinhamento básico
    64,  // POOL_THREADS - alinhamento de cache line
    16   // POOL_TEMP - alinhamento mínimo
};

static const char* pool_names[POOL_COUNT] = {
    "MAOS",
    "BUFFERS", 
    "THREADS",
    "TEMP"
};

// Inicializar sistema de memory pools
bool memory_pools_init(void) {
    DEBUG_IO("Inicializando sistema de memory pools");
    
    for (int i = 0; i < POOL_COUNT; i++) {
        MemoryPool* pool = &global_pools[i];
        
        // Alocar memória alinhada usando mmap para melhor performance
        pool->memory = mmap(NULL, pool_sizes[i], 
                           PROT_READ | PROT_WRITE, 
                           MAP_PRIVATE | MAP_ANONYMOUS, 
                           -1, 0);
        
        if (pool->memory == MAP_FAILED) {
            DEBUG_IO("ERRO: Falha ao alocar pool %s: %s", pool_names[i], strerror(errno));
            // Fallback para malloc se mmap falhar
            pool->memory = aligned_alloc(pool_alignments[i], pool_sizes[i]);
            if (!pool->memory) {
                DEBUG_IO("ERRO: Fallback malloc falhou para pool %s", pool_names[i]);
                return false;
            }
        }
        
        pool->size = pool_sizes[i];
        pool->used = 0;
        pool->align = pool_alignments[i];
        pool->is_active = true;
        
        // Inicializar mutex
        if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
            DEBUG_IO("ERRO: Falha ao inicializar mutex para pool %s", pool_names[i]);
            return false;
        }
        
        // Pré-aquecer a memória para evitar page faults
        memset(pool->memory, 0, pool_sizes[i]);
        
        DEBUG_IO("Pool %s inicializado: %zu bytes, alinhamento %zu", 
                 pool_names[i], pool_sizes[i], pool_alignments[i]);
    }
    
    DEBUG_IO("Sistema de memory pools inicializado com sucesso");
    return true;
}

// Cleanup do sistema de memory pools
void memory_pools_cleanup(void) {
    DEBUG_IO("Limpando sistema de memory pools");
    
    for (int i = 0; i < POOL_COUNT; i++) {
        MemoryPool* pool = &global_pools[i];
        
        if (pool->is_active) {
            // Destruir mutex
            pthread_mutex_destroy(&pool->mutex);
            
            // Liberar memória
            if (munmap(pool->memory, pool->size) == -1) {
                // Se mmap falhar, assumir que foi alocado com aligned_alloc
                free(pool->memory);
            }
            
            DEBUG_IO("Pool %s limpo: %zu bytes utilizados de %zu (%.1f%%)",
                     pool_names[i], pool->used, pool->size,
                     (double)pool->used * 100.0 / pool->size);
            
            pool->is_active = false;
        }
    }
    
    DEBUG_IO("Sistema de memory pools limpo");
}

// Alinhar pointer para o alinhamento especificado
static inline void* align_pointer(void* ptr, size_t align) {
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned = (addr + align - 1) & ~(align - 1);
    return (void*)aligned;
}

// Alocar memória de um pool específico
void* pool_alloc(PoolID pool_id, size_t size) {
    if (pool_id >= POOL_COUNT) {
        DEBUG_IO("ERRO: Pool ID inválido: %d", pool_id);
        return NULL;
    }
    
    if (size == 0) {
        DEBUG_IO("ERRO: Tentativa de alocar 0 bytes do pool %s", pool_names[pool_id]);
        return NULL;
    }
    
    MemoryPool* pool = &global_pools[pool_id];
    
    if (!pool->is_active) {
        DEBUG_IO("ERRO: Pool %s não está ativo", pool_names[pool_id]);
        return NULL;
    }
    
    pthread_mutex_lock(&pool->mutex);
    
    // Alinhar o tamanho para o alinhamento do pool
    size_t aligned_size = (size + pool->align - 1) & ~(pool->align - 1);
    
    // Verificar se há espaço suficiente
    if (pool->used + aligned_size > pool->size) {
        DEBUG_IO("ERRO: Pool %s esgotado: necessário %zu, disponível %zu",
                 pool_names[pool_id], aligned_size, pool->size - pool->used);
        pthread_mutex_unlock(&pool->mutex);
        return NULL;
    }
    
    // Calcular pointer alinhado
    void* base_ptr = (char*)pool->memory + pool->used;
    void* aligned_ptr = align_pointer(base_ptr, pool->align);
    
    // Ajustar used para incluir padding de alinhamento
    size_t actual_used = (char*)aligned_ptr - (char*)pool->memory + size;
    pool->used = actual_used;
    
    DEBUG_STATS("Pool %s: alocado %zu bytes (total usado: %zu/%.1f%%)",
                pool_names[pool_id], size, pool->used,
                (double)pool->used * 100.0 / pool->size);
    
    pthread_mutex_unlock(&pool->mutex);
    
    return aligned_ptr;
}

// Liberar memória de um pool (nota: pools são lineares, não fazem free individual)
void pool_free(PoolID pool_id, void* ptr) {
    if (pool_id >= POOL_COUNT || !ptr) {
        return;
    }
    
    // Memory pools são lineares - free individual não é suportado
    // Use pool_reset() para limpar todo o pool
    DEBUG_STATS("Pool %s: free ignorado (use pool_reset para limpar)", 
                pool_names[pool_id]);
}

// Resetar um pool (liberar toda a memória alocada)
void pool_reset(PoolID pool_id) {
    if (pool_id >= POOL_COUNT) {
        DEBUG_IO("ERRO: Pool ID inválido para reset: %d", pool_id);
        return;
    }
    
    MemoryPool* pool = &global_pools[pool_id];
    
    if (!pool->is_active) {
        DEBUG_IO("ERRO: Pool %s não está ativo para reset", pool_names[pool_id]);
        return;
    }
    
    pthread_mutex_lock(&pool->mutex);
    
    size_t old_used = pool->used;
    pool->used = 0;
    
    // Limpar a memória para segurança
    memset(pool->memory, 0, old_used);
    
    DEBUG_STATS("Pool %s resetado: %zu bytes liberados", 
                pool_names[pool_id], old_used);
    
    pthread_mutex_unlock(&pool->mutex);
}

// Obter uso atual de um pool
size_t pool_get_usage(PoolID pool_id) {
    if (pool_id >= POOL_COUNT) {
        return 0;
    }
    
    MemoryPool* pool = &global_pools[pool_id];
    
    if (!pool->is_active) {
        return 0;
    }
    
    pthread_mutex_lock(&pool->mutex);
    size_t usage = pool->used;
    pthread_mutex_unlock(&pool->mutex);
    
    return usage;
}

// Obter estatísticas detalhadas dos pools
void pool_print_stats(void) {
    DEBUG_IO("=== ESTATÍSTICAS DOS MEMORY POOLS ===");
    
    for (int i = 0; i < POOL_COUNT; i++) {
        MemoryPool* pool = &global_pools[i];
        
        if (pool->is_active) {
            pthread_mutex_lock(&pool->mutex);
            
            size_t used = pool->used;
            size_t size = pool->size;
            double percentage = (double)used * 100.0 / size;
            
            DEBUG_IO("Pool %s: %zu/%zu bytes (%.1f%%) - alinhamento %zu",
                     pool_names[i], used, size, percentage, pool->align);
            
            pthread_mutex_unlock(&pool->mutex);
        } else {
            DEBUG_IO("Pool %s: INATIVO", pool_names[i]);
        }
    }
    
    DEBUG_IO("=======================================");
} 