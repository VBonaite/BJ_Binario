#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include "jogo.h"
#include "baralho.h"
#include "constantes.h"

#define MAX_BINS 300  // -15 a 15 com bins de 0.1 = 300 bins
#define BIN_WIDTH 0.1
#define MIN_TC -15.0
#define MAX_TC 15.0
#define TEMP_FILE_PREFIX "temp_dealer_bj_"

// Estrutura para dados do bin
typedef struct {
    double tc_min;
    double tc_max;
    int total_ace_upcards;
    int dealer_blackjacks;
    double percentage;
} BinData;

// Estrutura para thread
typedef struct {
    int thread_id;
    int total_threads;
    int simulations_per_thread;
    char temp_filename[256];
    const char* output_suffix;
} ThreadData;

// Declarações de funções
void generate_python_script(const char* csv_filename, const char* output_suffix);

// Função para calcular bin do true count
int get_bin_index(double true_count) {
    if (true_count < MIN_TC) return 0;
    if (true_count >= MAX_TC) return MAX_BINS - 1;
    return (int)((true_count - MIN_TC) / BIN_WIDTH);
}

// Função para atualizar contagem baseada na carta
static void atualizar_counts(double *running_count, double *true_count, Carta c, size_t cartas_restantes) {
    *running_count += WONG_HALVES[carta_para_rank_idx(c)];
    double decks_restantes = (double)cartas_restantes / 52.0;
    if (decks_restantes < 1.0) decks_restantes = 1.0;
    *true_count = *running_count / decks_restantes;
}

// Função para adicionar carta à mão
static void adicionar_carta(uint64_t *mao, Carta c) {
    *mao += c;
}

// Função de simulação por thread
void* simulate_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    FILE* temp_file = fopen(data->temp_filename, "w");
    if (!temp_file) {
        fprintf(stderr, "Erro ao criar arquivo temporário %s\n", data->temp_filename);
        return NULL;
    }
    
    // Cabeçalho do arquivo temporário
    fprintf(temp_file, "true_count,ace_upcard,dealer_blackjack\n");
    
    for (int sim = 0; sim < data->simulations_per_thread; sim++) {
        // Simular 1000 shoes por simulação
        for (int shoe_num = 0; shoe_num < 1000; shoe_num++) {
            Shoe shoe;
            baralho_criar(&shoe);
            baralho_embaralhar(&shoe);
            
            double running_count = 0.0;
            double true_count = 0.0;
            
            // Simular hands até shoe acabar
            while (shoe.topo < shoe.total - 52) { // Deixar pelo menos 1 deck
                // Simular 7 jogadores (6 + 1 contabilizado)
                uint64_t *maos_bits = (uint64_t*)calloc(7, sizeof(uint64_t));
                uint64_t dealer_mao = 0;
                
                // Primeira rodada de distribuição
                for (int i = 0; i < 7; ++i) {
                    Carta c = baralho_comprar(&shoe);
                    adicionar_carta(&maos_bits[i], c);
                    atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
                }
                
                // Dealer recebe upcard
                Carta dealer_upcard = baralho_comprar(&shoe);
                adicionar_carta(&dealer_mao, dealer_upcard);
                atualizar_counts(&running_count, &true_count, dealer_upcard, shoe.total - shoe.topo);
                
                // Verificar se upcard é Ás
                #if defined(__GNUC__)
                int idx = __builtin_ctzll(dealer_upcard) / 3;
                #else
                int idx = 0;
                uint64_t temp = dealer_upcard;
                while ((temp & 0x7ULL) == 0) {
                    temp >>= 3;
                    ++idx;
                }
                #endif
                
                if (idx == 12) { // Ás
                    
                    // Segunda rodada de distribuição
                    for (int i = 0; i < 7; ++i) {
                        Carta c = baralho_comprar(&shoe);
                        adicionar_carta(&maos_bits[i], c);
                        atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
                    }
                    
                    // Dealer recebe hole card
                    Carta dealer_hole = baralho_comprar(&shoe);
                    adicionar_carta(&dealer_mao, dealer_hole);
                    
                    // Verificar se dealer tem blackjack
                    Mao dealer_info;
                    avaliar_mao(dealer_mao, &dealer_info);
                    
                    int dealer_has_bj = dealer_info.blackjack ? 1 : 0;
                    
                    // Salvar dados no arquivo temporário
                    fprintf(temp_file, "%.2f,1,%d\n", true_count, dealer_has_bj);
                    
                    // Contabilizar hole card apenas se dealer tem BJ
                    if (dealer_has_bj) {
                        atualizar_counts(&running_count, &true_count, dealer_hole, shoe.total - shoe.topo);
                    }
                } else {
                    // Upcard não é Ás, completar a mão normalmente
                    for (int i = 0; i < 7; ++i) {
                        Carta c = baralho_comprar(&shoe);
                        adicionar_carta(&maos_bits[i], c);
                        atualizar_counts(&running_count, &true_count, c, shoe.total - shoe.topo);
                    }
                    
                    Carta dealer_hole = baralho_comprar(&shoe);
                    adicionar_carta(&dealer_mao, dealer_hole);
                    atualizar_counts(&running_count, &true_count, dealer_hole, shoe.total - shoe.topo);
                }
                
                free(maos_bits);
            }
            
            baralho_destruir(&shoe);
        }
        
        // Progresso
        if (sim % 100 == 0) {
            printf("Thread %d: %d/%d simulações concluídas\n", 
                   data->thread_id, sim, data->simulations_per_thread);
        }
    }
    
    fclose(temp_file);
    return NULL;
}

// Função para processar dados e gerar CSV
void process_data_to_csv(int num_threads, const char* output_suffix) {
    BinData bins[MAX_BINS];
    
    // Inicializar bins
    for (int i = 0; i < MAX_BINS; i++) {
        bins[i].tc_min = MIN_TC + i * BIN_WIDTH;
        bins[i].tc_max = bins[i].tc_min + BIN_WIDTH;
        bins[i].total_ace_upcards = 0;
        bins[i].dealer_blackjacks = 0;
        bins[i].percentage = 0.0;
    }
    
    // Processar arquivos temporários
    for (int t = 0; t < num_threads; t++) {
        char temp_filename[256];
        snprintf(temp_filename, sizeof(temp_filename), "%s%d.csv", TEMP_FILE_PREFIX, t);
        
        FILE* temp_file = fopen(temp_filename, "r");
        if (!temp_file) continue;
        
        char line[256];
        fgets(line, sizeof(line), temp_file); // Pular cabeçalho
        
        double tc;
        int ace_upcard, dealer_bj;
        while (fgets(line, sizeof(line), temp_file)) {
            if (sscanf(line, "%lf,%d,%d", &tc, &ace_upcard, &dealer_bj) == 3) {
                if (ace_upcard == 1) { // Só processar quando upcard é Ás
                    int bin_idx = get_bin_index(tc);
                    if (bin_idx >= 0 && bin_idx < MAX_BINS) {
                        bins[bin_idx].total_ace_upcards++;
                        bins[bin_idx].dealer_blackjacks += dealer_bj;
                    }
                }
            }
        }
        
        fclose(temp_file);
        unlink(temp_filename); // Remover arquivo temporário
    }
    
    // Calcular percentuais
    for (int i = 0; i < MAX_BINS; i++) {
        if (bins[i].total_ace_upcards > 0) {
            bins[i].percentage = (double)bins[i].dealer_blackjacks / bins[i].total_ace_upcards * 100.0;
        }
    }
    
    // Gerar CSV
    char csv_filename[256];
    snprintf(csv_filename, sizeof(csv_filename), "dealer_blackjack_%s.csv", output_suffix);
    
    FILE* csv_file = fopen(csv_filename, "w");
    if (!csv_file) {
        fprintf(stderr, "Erro ao criar arquivo CSV\n");
        return;
    }
    
    fprintf(csv_file, "true_count_min,true_count_max,true_count_center,total_ace_upcards,dealer_blackjacks,percentage\n");
    
    for (int i = 0; i < MAX_BINS; i++) {
        if (bins[i].total_ace_upcards > 0) {
            double tc_center = bins[i].tc_min + BIN_WIDTH / 2.0;
            fprintf(csv_file, "%.2f,%.2f,%.2f,%d,%d,%.4f\n",
                   bins[i].tc_min, bins[i].tc_max, tc_center,
                   bins[i].total_ace_upcards, bins[i].dealer_blackjacks, bins[i].percentage);
        }
    }
    
    fclose(csv_file);
    printf("CSV gerado: %s\n", csv_filename);
    
    // Gerar script Python para histograma
    generate_python_script(csv_filename, output_suffix);
}

// Função para gerar script Python do histograma
void generate_python_script(const char* csv_filename, const char* output_suffix) {
    char script_filename[256];
    snprintf(script_filename, sizeof(script_filename), "generate_histogram_%s.py", output_suffix);
    
    FILE* script = fopen(script_filename, "w");
    if (!script) {
        fprintf(stderr, "Erro ao criar script Python\n");
        return;
    }
    
    fprintf(script, "#!/usr/bin/env python3\n");
    fprintf(script, "import pandas as pd\n");
    fprintf(script, "import matplotlib.pyplot as plt\n");
    fprintf(script, "import numpy as np\n\n");
    
    fprintf(script, "# Carregar dados\n");
    fprintf(script, "df = pd.read_csv('%s')\n", csv_filename);
    fprintf(script, "df = df[df['total_ace_upcards'] > 0]  # Filtrar bins com dados\n\n");
    
    fprintf(script, "# Calcular valores para escala customizada\n");
    fprintf(script, "max_val = df['percentage'].max()\n");
    fprintf(script, "min_val = df[df['percentage'] > 0]['percentage'].min()\n\n");
    
    fprintf(script, "# Criar o histograma\n");
    fprintf(script, "plt.figure(figsize=(12, 8))\n");
    fprintf(script, "plt.bar(df['true_count_center'], df['percentage'], width=0.08, alpha=0.7, edgecolor='black')\n\n");
    
    fprintf(script, "# Configurar escala customizada do eixo Y\n");
    fprintf(script, "custom_ticks = [0, min_val, max_val*0.09, max_val*0.21, max_val*0.33, max_val*0.45, max_val*0.57, max_val*0.69, max_val*0.81, max_val*0.93, max_val*1.05]\n");
    fprintf(script, "custom_labels = ['0%%', '10%%', '20%%', '30%%', '40%%', '50%%', '60%%', '70%%', '80%%', '90%%', '100%%']\n");
    fprintf(script, "plt.yticks(custom_ticks, custom_labels)\n\n");
    
    fprintf(script, "# Configurações do gráfico\n");
    fprintf(script, "plt.xlabel('True Count')\n");
    fprintf(script, "plt.ylabel('Percentual de Blackjack do Dealer (%%)')\n");
    fprintf(script, "plt.title('Percentual de Blackjack do Dealer quando Upcard é Ás vs True Count')\n");
    fprintf(script, "plt.grid(True, alpha=0.3)\n");
    fprintf(script, "plt.xlim(-15, 15)\n");
    fprintf(script, "plt.ylim(0, max_val*1.1)\n\n");
    
    fprintf(script, "# Salvar o gráfico\n");
    fprintf(script, "plt.tight_layout()\n");
    fprintf(script, "plt.savefig('dealer_blackjack_%s.png', dpi=300, bbox_inches='tight')\n", output_suffix);
    fprintf(script, "print('Histograma salvo: dealer_blackjack_%s.png')\n", output_suffix);
    fprintf(script, "plt.close()\n");
    
    fclose(script);
    
    // Executar script Python
    char command[512];
    snprintf(command, sizeof(command), "python3 %s", script_filename);
    system(command);
    
    // Remover script temporário
    unlink(script_filename);
}

// Função principal
int main(int argc, char *argv[]) {
    int num_simulations = 1000;
    int num_threads = 8;
    char output_suffix[64] = "analysis";
    
    // Processar argumentos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            num_simulations = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            num_threads = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            strcpy(output_suffix, argv[++i]);
        }
    }
    
    printf("=== ANALISADOR DE BLACKJACK DO DEALER ===\n");
    printf("Simulações: %d\n", num_simulations);
    printf("Threads: %d\n", num_threads);
    printf("Sufixo de saída: %s\n", output_suffix);
    printf("Analisando percentual de BJ do dealer com upcard Ás vs True Count...\n\n");
    
    // Criar threads
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    
    int sims_per_thread = num_simulations / num_threads;
    
    for (int t = 0; t < num_threads; t++) {
        thread_data[t].thread_id = t;
        thread_data[t].total_threads = num_threads;
        thread_data[t].simulations_per_thread = sims_per_thread;
        thread_data[t].output_suffix = output_suffix;
        snprintf(thread_data[t].temp_filename, sizeof(thread_data[t].temp_filename),
                "%s%d.csv", TEMP_FILE_PREFIX, t);
        
        pthread_create(&threads[t], NULL, simulate_thread, &thread_data[t]);
    }
    
    // Aguardar threads
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
    
    printf("\nProcessando dados e gerando relatórios...\n");
    process_data_to_csv(num_threads, output_suffix);
    
    printf("\nAnálise concluída!\n");
    return 0;
} 