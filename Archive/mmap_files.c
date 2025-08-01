#define _GNU_SOURCE
#include "structures.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

// Manager global de arquivos mapeados
MappedFileManager mmap_manager = {0};

// Inicializar sistema de memory-mapped files
bool mmap_init(void) {
    DEBUG_IO("Inicializando sistema de memory-mapped files");
    
    if (mmap_manager.is_initialized) {
        DEBUG_IO("Sistema de mmap já foi inicializado");
        return true;
    }
    
#if !MMAP_ENABLED
    DEBUG_IO("Memory-mapped files não suportados neste sistema");
    return false;
#endif
    
    memset(&mmap_manager, 0, sizeof(MappedFileManager));
    
    // Inicializar mutex
    if (pthread_mutex_init(&mmap_manager.mutex, NULL) != 0) {
        DEBUG_IO("ERRO: Falha ao inicializar mutex do mmap manager");
        return false;
    }
    
    mmap_manager.active_count = 0;
    mmap_manager.is_initialized = true;
    
    DEBUG_IO("Sistema de memory-mapped files inicializado com sucesso");
    return true;
}

// Cleanup do sistema de memory-mapped files
void mmap_cleanup(void) {
    DEBUG_IO("Limpando sistema de memory-mapped files");
    
    if (!mmap_manager.is_initialized) {
        return;
    }
    
    pthread_mutex_lock(&mmap_manager.mutex);
    
    // Fechar todos os arquivos mapeados
    for (int i = 0; i < MAX_MAPPED_FILES; i++) {
        MappedFile* file = &mmap_manager.files[i];
        if (file->is_mapped) {
            mmap_close_file(file);
        }
    }
    
    mmap_manager.active_count = 0;
    mmap_manager.is_initialized = false;
    
    pthread_mutex_unlock(&mmap_manager.mutex);
    pthread_mutex_destroy(&mmap_manager.mutex);
    
    DEBUG_IO("Sistema de memory-mapped files limpo");
}

// Encontrar slot livre no manager
static int find_free_slot(void) {
    for (int i = 0; i < MAX_MAPPED_FILES; i++) {
        if (!mmap_manager.files[i].is_mapped) {
            return i;
        }
    }
    return -1;
}

// Abrir arquivo com memory mapping
MappedFile* mmap_open_file(const char* filepath, bool read_only, size_t expected_size) {
    if (!filepath || !mmap_manager.is_initialized) {
        DEBUG_IO("ERRO: Parâmetros inválidos para mmap_open_file");
        return NULL;
    }
    
#if !MMAP_ENABLED
    DEBUG_IO("Memory-mapped files não suportados, usando I/O tradicional");
    return NULL;
#endif
    
    // Verificar se vale a pena usar mmap
    if (expected_size > 0 && expected_size < MMAP_MIN_SIZE) {
        DEBUG_IO("Arquivo muito pequeno (%zu bytes) para mmap, usando I/O tradicional", expected_size);
        return NULL;
    }
    
    pthread_mutex_lock(&mmap_manager.mutex);
    
    int slot = find_free_slot();
    if (slot == -1) {
        DEBUG_IO("ERRO: Não há slots livres para memory mapping");
        pthread_mutex_unlock(&mmap_manager.mutex);
        return NULL;
    }
    
    MappedFile* mapped_file = &mmap_manager.files[slot];
    memset(mapped_file, 0, sizeof(MappedFile));
    
    // Abrir arquivo
    int flags = read_only ? O_RDONLY : (O_RDWR | O_CREAT);
    int mode = read_only ? 0 : (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    
    mapped_file->fd = open(filepath, flags, mode);
    if (mapped_file->fd == -1) {
        DEBUG_IO("ERRO: Falha ao abrir arquivo %s: %s", filepath, strerror(errno));
        pthread_mutex_unlock(&mmap_manager.mutex);
        return NULL;
    }
    
    // Obter tamanho do arquivo
    struct stat st;
    if (fstat(mapped_file->fd, &st) == -1) {
        DEBUG_IO("ERRO: Falha ao obter stat do arquivo %s: %s", filepath, strerror(errno));
        close(mapped_file->fd);
        pthread_mutex_unlock(&mmap_manager.mutex);
        return NULL;
    }
    
    size_t file_size = st.st_size;
    
    // Para arquivos novos, usar o tamanho esperado
    if (file_size == 0 && expected_size > 0 && !read_only) {
        if (ftruncate(mapped_file->fd, expected_size) == -1) {
            DEBUG_IO("ERRO: Falha ao definir tamanho do arquivo %s: %s", filepath, strerror(errno));
            close(mapped_file->fd);
            pthread_mutex_unlock(&mmap_manager.mutex);
            return NULL;
        }
        file_size = expected_size;
    }
    
    // Verificar se ainda vale a pena usar mmap após conhecer o tamanho real
    if (file_size < MMAP_MIN_SIZE) {
        DEBUG_IO("Arquivo %s muito pequeno (%zu bytes) para mmap", filepath, file_size);
        close(mapped_file->fd);
        pthread_mutex_unlock(&mmap_manager.mutex);
        return NULL;
    }
    
    // Mapear arquivo na memória
    int prot = read_only ? PROT_READ : (PROT_READ | PROT_WRITE);
    int map_flags = MAP_SHARED;
    
    mapped_file->data = mmap(NULL, file_size, prot, map_flags, mapped_file->fd, 0);
    if (mapped_file->data == MAP_FAILED) {
        DEBUG_IO("ERRO: Falha ao mapear arquivo %s na memória: %s", filepath, strerror(errno));
        close(mapped_file->fd);
        pthread_mutex_unlock(&mmap_manager.mutex);
        return NULL;
    }
    
    // Configurar estrutura
    mapped_file->size = file_size;
    mapped_file->is_mapped = true;
    mapped_file->read_only = read_only;
    strncpy(mapped_file->filepath, filepath, sizeof(mapped_file->filepath) - 1);
    mapped_file->filepath[sizeof(mapped_file->filepath) - 1] = '\0';
    
    mmap_manager.active_count++;
    
    DEBUG_IO("Arquivo %s mapeado com sucesso: %zu bytes, modo %s", 
             filepath, file_size, read_only ? "readonly" : "readwrite");
    
    pthread_mutex_unlock(&mmap_manager.mutex);
    return mapped_file;
}

// Fechar arquivo mapeado
void mmap_close_file(MappedFile* mapped_file) {
    if (!mapped_file || !mapped_file->is_mapped) {
        return;
    }
    
    pthread_mutex_lock(&mmap_manager.mutex);
    
    // Sincronizar dados antes de fechar
    if (!mapped_file->read_only) {
        mmap_sync_file(mapped_file);
    }
    
    // Desmapear da memória
    if (munmap(mapped_file->data, mapped_file->size) == -1) {
        DEBUG_IO("ERRO: Falha ao desmapear arquivo %s: %s", 
                mapped_file->filepath, strerror(errno));
    }
    
    // Fechar arquivo
    if (close(mapped_file->fd) == -1) {
        DEBUG_IO("ERRO: Falha ao fechar arquivo %s: %s", 
                mapped_file->filepath, strerror(errno));
    }
    
    DEBUG_IO("Arquivo %s fechado e desmapeado", mapped_file->filepath);
    
    // Limpar estrutura
    memset(mapped_file, 0, sizeof(MappedFile));
    mmap_manager.active_count--;
    
    pthread_mutex_unlock(&mmap_manager.mutex);
}

// Sincronizar dados do arquivo mapeado
bool mmap_sync_file(MappedFile* mapped_file) {
    if (!mapped_file || !mapped_file->is_mapped || mapped_file->read_only) {
        return false;
    }
    
    if (msync(mapped_file->data, mapped_file->size, MS_ASYNC) == -1) {
        DEBUG_IO("ERRO: Falha ao sincronizar arquivo %s: %s", 
                mapped_file->filepath, strerror(errno));
        return false;
    }
    
    DEBUG_STATS("Arquivo %s sincronizado com sucesso", mapped_file->filepath);
    return true;
}

// Obter pointer para dados com offset
void* mmap_get_data(MappedFile* mapped_file, size_t offset) {
    if (!mapped_file || !mapped_file->is_mapped) {
        DEBUG_IO("ERRO: Arquivo não está mapeado");
        return NULL;
    }
    
    if (offset >= mapped_file->size) {
        DEBUG_IO("ERRO: Offset %zu além do tamanho do arquivo %zu", 
                offset, mapped_file->size);
        return NULL;
    }
    
    return (char*)mapped_file->data + offset;
}

// Escrever dados no arquivo mapeado
size_t mmap_write_data(MappedFile* mapped_file, size_t offset, const void* data, size_t size) {
    if (!mapped_file || !mapped_file->is_mapped || mapped_file->read_only) {
        DEBUG_IO("ERRO: Arquivo não pode ser escrito");
        return 0;
    }
    
    if (!data || size == 0) {
        DEBUG_IO("ERRO: Dados inválidos para escrita");
        return 0;
    }
    
    if (offset + size > mapped_file->size) {
        DEBUG_IO("ERRO: Escrita além do fim do arquivo: offset=%zu, size=%zu, file_size=%zu", 
                offset, size, mapped_file->size);
        return 0;
    }
    
    void* dest = mmap_get_data(mapped_file, offset);
    if (!dest) {
        return 0;
    }
    
    memcpy(dest, data, size);
    
    DEBUG_STATS("Escritos %zu bytes no arquivo %s (offset %zu)", 
               size, mapped_file->filepath, offset);
    
    return size;
}

// Ler dados do arquivo mapeado
size_t mmap_read_data(MappedFile* mapped_file, size_t offset, void* buffer, size_t size) {
    if (!mapped_file || !mapped_file->is_mapped) {
        DEBUG_IO("ERRO: Arquivo não está mapeado");
        return 0;
    }
    
    if (!buffer || size == 0) {
        DEBUG_IO("ERRO: Buffer inválido para leitura");
        return 0;
    }
    
    if (offset + size > mapped_file->size) {
        DEBUG_IO("ERRO: Leitura além do fim do arquivo: offset=%zu, size=%zu, file_size=%zu", 
                offset, size, mapped_file->size);
        return 0;
    }
    
    void* src = mmap_get_data(mapped_file, offset);
    if (!src) {
        return 0;
    }
    
    memcpy(buffer, src, size);
    
    DEBUG_STATS("Lidos %zu bytes do arquivo %s (offset %zu)", 
               size, mapped_file->filepath, offset);
    
    return size;
}

// Obter estatísticas do sistema de mmap
void mmap_print_stats(void) {
    if (!mmap_manager.is_initialized) {
        DEBUG_IO("Sistema de mmap não inicializado");
        return;
    }
    
    pthread_mutex_lock(&mmap_manager.mutex);
    
    DEBUG_IO("=== ESTATÍSTICAS MEMORY-MAPPED FILES ===");
    DEBUG_IO("Arquivos ativos: %d/%d", mmap_manager.active_count, MAX_MAPPED_FILES);
    
    size_t total_mapped = 0;
    for (int i = 0; i < MAX_MAPPED_FILES; i++) {
        MappedFile* file = &mmap_manager.files[i];
        if (file->is_mapped) {
            DEBUG_IO("Arquivo %d: %s (%zu bytes, %s)", 
                     i, file->filepath, file->size, 
                     file->read_only ? "readonly" : "readwrite");
            total_mapped += file->size;
        }
    }
    
    DEBUG_IO("Total mapeado: %zu bytes (%.2f MB)", 
             total_mapped, (double)total_mapped / (1024 * 1024));
    DEBUG_IO("========================================");
    
    pthread_mutex_unlock(&mmap_manager.mutex);
} 