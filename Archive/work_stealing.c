#include "structures.h"
#include "simulacao.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Sistema global de work stealing
WorkStealingSystem work_stealing_system = {0};

// Inicializar work stealing queue
static bool init_work_queue(WorkStealingQueue* queue, int capacity) {
    if (!queue || capacity <= 0) {
        return false;
    }
    
    queue->tasks = (WorkTask*)calloc(capacity, sizeof(WorkTask));
    if (!queue->tasks) {
        return false;
    }
    
    queue->head = 0;
    queue->tail = 0;
    queue->capacity = capacity;
    queue->shutdown = false;
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->tasks);
        return false;
    }
    
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->tasks);
        return false;
    }
    
    return true;
}

// Cleanup work stealing queue
static void cleanup_work_queue(WorkStealingQueue* queue) {
    if (!queue) {
        return;
    }
    
    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = true;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    
    if (queue->tasks) {
        free(queue->tasks);
        queue->tasks = NULL;
    }
}

// Inicializar sistema de work stealing
bool work_stealing_init(int num_workers) {
    DEBUG_IO("Inicializando sistema de work stealing com %d workers", num_workers);
    
    if (num_workers <= 0 || num_workers > 64) {
        DEBUG_IO("ERRO: Número inválido de workers: %d", num_workers);
        return false;
    }
    
    if (work_stealing_system.is_initialized) {
        DEBUG_IO("Sistema de work stealing já foi inicializado");
        return true;
    }
    
    memset(&work_stealing_system, 0, sizeof(WorkStealingSystem));
    
    // Alocar estruturas
    work_stealing_system.queues = (WorkStealingQueue*)calloc(num_workers, sizeof(WorkStealingQueue));
    work_stealing_system.workers = (WorkerContext*)calloc(num_workers, sizeof(WorkerContext));
    work_stealing_system.threads = (pthread_t*)calloc(num_workers, sizeof(pthread_t));
    
    if (!work_stealing_system.queues || !work_stealing_system.workers || !work_stealing_system.threads) {
        DEBUG_IO("ERRO: Falha ao alocar memória para work stealing");
        work_stealing_cleanup();
        return false;
    }
    
    work_stealing_system.num_workers = num_workers;
    
    // Inicializar filas de trabalho (uma por worker)
    int queue_capacity = 1000; // Capacidade da fila por worker
    for (int i = 0; i < num_workers; i++) {
        if (!init_work_queue(&work_stealing_system.queues[i], queue_capacity)) {
            DEBUG_IO("ERRO: Falha ao inicializar fila do worker %d", i);
            work_stealing_cleanup();
            return false;
        }
    }
    
    // Inicializar contextos dos workers
    for (int i = 0; i < num_workers; i++) {
        WorkerContext* worker = &work_stealing_system.workers[i];
        worker->worker_id = i;
        worker->num_workers = num_workers;
        worker->local_queue = &work_stealing_system.queues[i];
        
        // Array de ponteiros para todas as filas (para work stealing)
        worker->all_queues = (WorkStealingQueue**)malloc(num_workers * sizeof(WorkStealingQueue*));
        if (!worker->all_queues) {
            DEBUG_IO("ERRO: Falha ao alocar array de filas para worker %d", i);
            work_stealing_cleanup();
            return false;
        }
        
        for (int j = 0; j < num_workers; j++) {
            worker->all_queues[j] = &work_stealing_system.queues[j];
        }
        
        // Inicializar estatísticas atômicas
        atomic_init(&worker->tasks_completed, 0);
        atomic_init(&worker->tasks_stolen, 0);
        atomic_init(&worker->tasks_given, 0);
    }
    
    work_stealing_system.is_initialized = true;
    
    DEBUG_IO("Sistema de work stealing inicializado com sucesso");
    return true;
}

// Cleanup do sistema de work stealing
void work_stealing_cleanup(void) {
    DEBUG_IO("Limpando sistema de work stealing");
    
    if (!work_stealing_system.is_initialized) {
        return;
    }
    
    // Sinalizar shutdown para todas as filas
    for (int i = 0; i < work_stealing_system.num_workers; i++) {
        cleanup_work_queue(&work_stealing_system.queues[i]);
    }
    
    // Aguardar todas as threads terminarem
    for (int i = 0; i < work_stealing_system.num_workers; i++) {
        if (work_stealing_system.threads[i] != 0) {
            pthread_join(work_stealing_system.threads[i], NULL);
        }
    }
    
    // Liberar estruturas dos workers
    for (int i = 0; i < work_stealing_system.num_workers; i++) {
        WorkerContext* worker = &work_stealing_system.workers[i];
        if (worker->all_queues) {
            free(worker->all_queues);
        }
    }
    
    // Liberar estruturas principais
    if (work_stealing_system.queues) {
        free(work_stealing_system.queues);
    }
    if (work_stealing_system.workers) {
        free(work_stealing_system.workers);
    }
    if (work_stealing_system.threads) {
        free(work_stealing_system.threads);
    }
    
    memset(&work_stealing_system, 0, sizeof(WorkStealingSystem));
    
    DEBUG_IO("Sistema de work stealing limpo");
}

// Adicionar task à fila de um worker
bool work_stealing_add_task(int worker_id, int start_sim, int end_sim) {
    if (!work_stealing_system.is_initialized || 
        worker_id < 0 || worker_id >= work_stealing_system.num_workers) {
        return false;
    }
    
    WorkStealingQueue* queue = &work_stealing_system.queues[worker_id];
    
    pthread_mutex_lock(&queue->mutex);
    
    // Verificar se há espaço na fila
    int next_tail = (queue->tail + 1) % queue->capacity;
    if (next_tail == queue->head) {
        DEBUG_IO("ERRO: Fila do worker %d está cheia", worker_id);
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    
    // Adicionar task
    WorkTask* task = &queue->tasks[queue->tail];
    task->start_sim = start_sim;
    task->end_sim = end_sim;
    task->is_taken = false;
    task->is_completed = false;
    
    queue->tail = next_tail;
    
    // Sinalizar que há trabalho disponível
    pthread_cond_signal(&queue->not_empty);
    
    DEBUG_STATS("Task adicionada ao worker %d: simulações %d-%d", 
               worker_id, start_sim, end_sim);
    
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

// Obter task da fila local
WorkTask* work_stealing_get_task(int worker_id) {
    if (!work_stealing_system.is_initialized || 
        worker_id < 0 || worker_id >= work_stealing_system.num_workers) {
        return NULL;
    }
    
    WorkStealingQueue* queue = &work_stealing_system.queues[worker_id];
    
    pthread_mutex_lock(&queue->mutex);
    
    // Aguardar até haver trabalho ou shutdown
    while (queue->head == queue->tail && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    
    // Pegar task do início da fila (FIFO)
    WorkTask* task = &queue->tasks[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    
    task->is_taken = true;
    
    DEBUG_STATS("Worker %d pegou task: simulações %d-%d", 
               worker_id, task->start_sim, task->end_sim);
    
    pthread_mutex_unlock(&queue->mutex);
    return task;
}

// Tentar roubar task de outro worker
WorkTask* work_stealing_steal_task(int thief_id, int victim_id) {
    if (!work_stealing_system.is_initialized || 
        thief_id < 0 || thief_id >= work_stealing_system.num_workers ||
        victim_id < 0 || victim_id >= work_stealing_system.num_workers ||
        thief_id == victim_id) {
        return NULL;
    }
    
    WorkStealingQueue* victim_queue = &work_stealing_system.queues[victim_id];
    
    // Tentar obter lock sem bloquear (non-blocking steal)
    if (pthread_mutex_trylock(&victim_queue->mutex) != 0) {
        return NULL; // Não conseguiu obter lock, tentar outro worker
    }
    
    // Verificar se há trabalho para roubar
    if (victim_queue->head == victim_queue->tail || victim_queue->shutdown) {
        pthread_mutex_unlock(&victim_queue->mutex);
        return NULL;
    }
    
    // Roubar task do final da fila (para minimizar contenção com o owner)
    int steal_index = (victim_queue->tail - 1 + victim_queue->capacity) % victim_queue->capacity;
    WorkTask* task = &victim_queue->tasks[steal_index];
    
    // Verificar se a task não foi tomada
    if (task->is_taken) {
        pthread_mutex_unlock(&victim_queue->mutex);
        return NULL;
    }
    
    // Marcar como tomada e ajustar tail
    task->is_taken = true;
    victim_queue->tail = steal_index;
    
    DEBUG_STATS("Worker %d roubou task do worker %d: simulações %d-%d", 
               thief_id, victim_id, task->start_sim, task->end_sim);
    
    // Atualizar estatísticas
    atomic_fetch_add(&work_stealing_system.workers[thief_id].tasks_stolen, 1);
    atomic_fetch_add(&work_stealing_system.workers[victim_id].tasks_given, 1);
    
    pthread_mutex_unlock(&victim_queue->mutex);
    return task;
}

// Marcar task como completada
void work_stealing_complete_task(WorkTask* task) {
    if (!task) {
        return;
    }
    
    task->is_completed = true;
    
    DEBUG_STATS("Task completada: simulações %d-%d", 
               task->start_sim, task->end_sim);
}

// Tentar obter trabalho (local + work stealing)
static WorkTask* get_work(int worker_id) {
    // Primeiro, tentar pegar trabalho da fila local
    WorkTask* task = work_stealing_get_task(worker_id);
    if (task) {
        return task;
    }
    
    // Se não há trabalho local, tentar roubar de outros workers
    int num_workers = work_stealing_system.num_workers;
    
    // Tentar roubar de todos os outros workers em ordem aleatória
    for (int attempts = 0; attempts < num_workers - 1; attempts++) {
        int victim_id = (worker_id + 1 + attempts) % num_workers;
        task = work_stealing_steal_task(worker_id, victim_id);
        if (task) {
            return task;
        }
    }
    
    return NULL; // Não há trabalho disponível
}

// Função principal do worker thread
void* work_stealing_worker_thread(void* arg) {
    WorkerContext* worker = (WorkerContext*)arg;
    if (!worker) {
        return NULL;
    }
    
    DEBUG_IO("Worker %d iniciado", worker->worker_id);
    
    while (true) {
        // Tentar obter trabalho
        WorkTask* task = get_work(worker->worker_id);
        if (!task) {
            // Não há mais trabalho, terminar
            DEBUG_IO("Worker %d: não há mais trabalho, terminando", worker->worker_id);
            break;
        }
        
        // Executar simulações da task
        for (int sim_id = task->start_sim; sim_id < task->end_sim; sim_id++) {
            simulacao_completa(
                worker->log_level, sim_id, worker->output_suffix, 
                worker->global_log_count, worker->dealer_analysis,
                worker->freq_analysis_26, worker->freq_analysis_70, 
                worker->freq_analysis_A, worker->split_analysis,
                worker->dealer_mutex, worker->freq_mutex, worker->split_mutex
            );
        }
        
        // Marcar task como completada
        work_stealing_complete_task(task);
        atomic_fetch_add(&worker->tasks_completed, 1);
        
        DEBUG_STATS("Worker %d completou task: %d simulações", 
                   worker->worker_id, task->end_sim - task->start_sim);
    }
    
    DEBUG_IO("Worker %d finalizado", worker->worker_id);
    return NULL;
}

// Mostrar estatísticas do work stealing
void work_stealing_print_stats(void) {
    if (!work_stealing_system.is_initialized) {
        DEBUG_IO("Sistema de work stealing não inicializado");
        return;
    }
    
    DEBUG_IO("=== ESTATÍSTICAS WORK STEALING ===");
    
    int total_completed = 0;
    int total_stolen = 0;
    int total_given = 0;
    
    for (int i = 0; i < work_stealing_system.num_workers; i++) {
        WorkerContext* worker = &work_stealing_system.workers[i];
        
        int completed = atomic_load(&worker->tasks_completed);
        int stolen = atomic_load(&worker->tasks_stolen);
        int given = atomic_load(&worker->tasks_given);
        
        DEBUG_IO("Worker %d: %d tasks completadas, %d roubadas, %d doadas", 
                 i, completed, stolen, given);
        
        total_completed += completed;
        total_stolen += stolen;
        total_given += given;
    }
    
    DEBUG_IO("Total: %d tasks completadas, %d roubos, balanceamento: %.1f%%", 
             total_completed, total_stolen, 
             total_completed > 0 ? (double)total_stolen * 100.0 / total_completed : 0.0);
    DEBUG_IO("=====================================");
} 