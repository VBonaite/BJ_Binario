# EXPLICAÇÃO COMPLETA DAS ESTATÍSTICAS DO SIMULADOR DE BLACKJACK

## 📊 VISÃO GERAL DOS ARGUMENTOS ESTATÍSTICOS

O simulador de blackjack possui **4 sistemas principais** de coleta de estatísticas:

### 🎯 **ARGUMENTOS DISPONÍVEIS**
- **`-hist26`**: Análise de frequência para upcards 2-6 do dealer
- **`-hist70`**: Análise de frequência para upcards 7-10 do dealer  
- **`-histA`**: Análise de frequência para upcard A do dealer
- **`-split`**: Análise completa de resultados de splits

---

## 📈 **1. ANÁLISE DE FREQUÊNCIA (-hist26, -hist70, -histA)**

### 🎯 **Objetivo**
Analisar a frequência dos resultados finais do dealer (17, 18, 19, 20, 21, BJ, BUST) em função do true count, separado por upcard.

### 📊 **Sistema de Bins**
```c
// Configuração dos bins de true count
const double MIN_TC = -6.5;
const double MAX_TC = +6.5;  
const double BIN_WIDTH = 0.1;
const int MAX_BINS = 130;  // (-6.5 a +6.5) / 0.1

// Função de mapeamento
int bin_idx = (true_count - MIN_TC) / BIN_WIDTH;
```

### 📋 **Dados Coletados**

#### **Para cada combinação (upcard, resultado final):**

1. **true_count_for_stats**: TC capturado **ANTES** do hole card
2. **upcard**: Carta visível do dealer (2-A)
3. **final_result**: Resultado final do dealer

#### **Mapeamento de Resultados:**
```c
if (dealer_info.blackjack) {
    final_result = (dealer_up_rank == 10 || dealer_up_rank == 11) ? 5 : 4; // BJ ou 21
} else if (dealer_info.valor >= 17 && dealer_info.valor <= 21) {
    final_result = dealer_info.valor - 17; // 17->0, 18->1, 19->2, 20->3, 21->4
} else if (dealer_info.valor > 21) {
    final_result = 6; // BUST
}
```

### 🧮 **Fórmulas de Cálculo**

#### **1. Frequência por Bin:**
```
Frequência(bin_i, upcard, resultado) = 
    (Contagem_específica(bin_i, upcard, resultado) / Contagem_total(bin_i, upcard)) × 100.0
```

#### **2. Estrutura de Dados:**
```c
typedef struct {
    double tc_min, tc_max;       // Limites do bin
    int total_upcard_count;      // Total de vezes que upcard apareceu
    int final_count;             // Vezes que resultado específico ocorreu
    double frequency;            // Frequência calculada (%)
} FreqBinData;
```

### 📁 **Arquivos Gerados**

#### **Nomenclatura:**
- `freq_2_17_[sufixo].csv` - Upcard 2, resultado 17
- `freq_A_BJ_[sufixo].csv` - Upcard A, resultado Blackjack
- `freq_10_BUST_[sufixo].csv` - Upcard 10, resultado Bust

#### **Formato CSV:**
```csv
true_count_min,true_count_max,true_count_center,total_upcard_count,final_count,frequency
-6.50,-6.40,-6.45,1904,490,25.7353
-6.40,-6.30,-6.35,969,293,30.2374
...
```

### 🔍 **Argumentos Específicos**

#### **`-hist26` (Upcards 2-6)**
- **Objetivo**: Analisar upcards "fracos" do dealer
- **Range**: upcards 2, 3, 4, 5, 6
- **Arquivos**: `freq_2_*.csv` até `freq_6_*.csv`

#### **`-hist70` (Upcards 7-10)**
- **Objetivo**: Analisar upcards "fortes" do dealer
- **Range**: upcards 7, 8, 9, 10 (inclui J, Q, K mapeados para 10)
- **Arquivos**: `freq_7_*.csv` até `freq_10_*.csv`

#### **`-histA` (Upcard A)**
- **Objetivo**: Analisar upcard Ás (caso especial)
- **Range**: apenas upcard A
- **Arquivos**: `freq_A_*.csv`

---

## 🎴 **2. ANÁLISE DE SPLITS (-split)**

### 🎯 **Objetivo**
Analisar resultados de splits de pares específicos vs todas as upcards do dealer, capturando as **correlações reais** entre as duas mãos.

### 📊 **Pares Analisados**
```c
const char* pairs[] = {"AA", "1010", "99", "88", "77", "66", "55", "44", "33", "22"};
```
**Nota**: JJ, QQ, KK são mapeados para "1010" (tratados como 10-10).

### 📊 **Upcards Analisadas**
```c
const char* upcards[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "A"};
```

### 📋 **Dados Coletados**

#### **Para cada split:**
1. **true_count**: TC no momento do split
2. **Combinações de resultados** (9 possibilidades):
   - `lose_lose`: Ambas mãos perderam
   - `win_win`: Ambas mãos ganharam  
   - `push_push`: Ambas mãos empataram
   - `lose_win`: Mão1 perdeu, Mão2 ganhou
   - `lose_push`: Mão1 perdeu, Mão2 empatou
   - `win_lose`: Mão1 ganhou, Mão2 perdeu
   - `win_push`: Mão1 ganhou, Mão2 empatou
   - `push_lose`: Mão1 empatou, Mão2 perdeu
   - `push_win`: Mão1 empatou, Mão2 ganhou
3. **cards_used**: Total de cartas utilizadas no split

#### **Identificação de Resultados:**
```c
// Classificação individual
int mao1_win = (m1->resultado == 'V') ? 1 : 0;
int mao1_push = (m1->resultado == 'E') ? 1 : 0;
int mao1_lose = (m1->resultado == 'D') ? 1 : 0;

// Combinações reais (CORRETO - não assume independência)
int lose_lose = (mao1_lose && mao2_lose) ? 1 : 0;
int win_win = (mao1_win && mao2_win) ? 1 : 0;
// ... etc para as 9 combinações
```

### 🧮 **Fórmulas de Cálculo**

#### **1. Frequências das Combinações:**
```
freq_lose_lose = contagem_lose_lose / total_splits
freq_win_win = contagem_win_win / total_splits
// ... para todas as 9 combinações

// IMPORTANTE: ∑(todas as 9 frequências) = 1.0 (100%)
```

#### **2. Expected Value:**
```
EV = 2.0 × freq_win_win - 2.0 × freq_lose_lose
```
**Explicação**: +2 unidades quando ambas ganham, -2 quando ambas perdem.

#### **3. Estatísticas de Cartas:**
```
média_cartas = total_cards_used / total_splits
variância = (total_cards_squared / total_splits) - (média_cartas)²
desvio_padrão = √variância
```

### 📁 **Arquivos Gerados**

#### **Nomenclatura:**
- `split_outcome_AA_vs_2_[sufixo].csv`
- `split_outcome_88_vs_A_[sufixo].csv`
- etc.

#### **Formato CSV:**
```csv
true_count_min,true_count_max,true_count_center,total_splits,freq_lose_lose,freq_win_win,...,expected_value,avg_cards_used
-6.50,-6.40,-6.45,1250,0.0784,0.0712,...,0.1456,4.25
```

### 🔍 **Estrutura de Dados:**
```c
typedef struct {
    float true_count;
    int32_t lose_lose, win_win, push_push;
    int32_t lose_win, lose_push, win_lose;
    int32_t win_push, push_lose, push_win;
    int32_t cards_used;
    uint32_t checksum;
} SplitBinaryRecord;
```

---

## 💰 **3. MÉDIA DE UNIDADES POR SHOE**

### 🎯 **Objetivo**
Medir a performance média do sistema em unidades apostadas por shoe completado.

### 📊 **Processo de Coleta**

#### **1. Durante Cada Rodada:**
```c
// Calcular PNL da rodada em valor monetário
double pnl_rodada_total = soma_de_todos_PNLs_da_rodada;

// Converter para unidades
double unidades_rodada = pnl_rodada_total / unidade_atual;

// Acumular globalmente (thread-safe)
pthread_mutex_lock(&unidades_mutex);
unidades_total_global += unidades_rodada;
pthread_mutex_unlock(&unidades_mutex);
```

#### **2. Cálculo do PNL Individual:**
```c
void calcular_pnl(Mao *mao) {
    if (mao->resultado == 'V') {           // Vitória
        if (mao->blackjack) {
            mao->pnl = 1.5 * mao->aposta;  // BJ paga 3:2
        } else if (mao->isdouble) {
            mao->pnl = 2.0 * mao->aposta;  // Double: +2x
        } else {
            mao->pnl = mao->aposta;        // Vitória normal: +1x
        }
    } else if (mao->resultado == 'E') {    // Empate
        mao->pnl = 0.0;                    // Push: +0
    } else if (mao->resultado == 'D') {    // Derrota
        if (mao->isdouble) {
            mao->pnl = -2.0 * mao->aposta; // Double: -2x
        } else {
            mao->pnl = -mao->aposta;       // Derrota normal: -1x
        }
    }
}
```

### 🧮 **Fórmula Final**

```c
// No final da simulação
double unidades_totais = unidades_total_global;
long long total_shoes = num_sims * NUM_SHOES;
double media_unidades_por_shoe = unidades_totais / total_shoes;

printf("Média de unidades por shoe: %.4f\n", media_unidades_por_shoe);
```

### 📊 **Exemplo de Cálculo**

```
Simulações: 1000
Shoes por simulação: 1000  
Total de shoes: 1.000.000

Unidades acumuladas: -34.1523
Média por shoe: -34.1523 / 1.000.000 = -0.0000341523 ≈ -0.0000 unidades/shoe
```

### 💡 **Interpretação**
- **Valor positivo**: Sistema lucrativo
- **Valor negativo**: Sistema com edge da casa
- **Próximo de zero**: Sistema próximo do break-even

---

## ⚙️ **4. SISTEMA DE LOTES (BATCH SYSTEM)**

### 📊 **Otimização de Performance**

#### **Configuração:**
```c
#define FREQ_BATCH_SIZE 10000     // Frequências
#define SPLIT_BATCH_SIZE 10000    // Splits  
#define DEALER_BATCH_SIZE 10000   // Dealer analysis
```

#### **Funcionamento:**
1. **Coleta em Buffer**: Dados armazenados em arrays locais
2. **Escrita em Lotes**: Gravação periódica em arquivos binários temporários
3. **Processamento Final**: Consolidação de todos os lotes em CSVs finais
4. **Limpeza**: Remoção automática de arquivos temporários

### 📁 **Arquivos Temporários**
```
temp_total_upcard_2_batch_0.bin
temp_result_upcard_2_final_17_batch_0.bin
temp_split_0_AA_vs_2.bin
...
```

---

## 🔍 **5. CONTROLE DE QUALIDADE**

### ✅ **Validações Implementadas**

#### **1. Checksums:**
```c
uint32_t calculate_freq_checksum(const FreqBinaryRecord* record);
uint32_t calculate_split_checksum(const SplitBinaryRecord* record);
```

#### **2. Validação de Ranges:**
```c
int get_bin_index_robust(double true_count) {
    if (true_count < MIN_TC - 1e-10) return -1;
    if (true_count >= MAX_TC + 1e-10) return MAX_BINS - 1;
    
    int bin_idx = (int)((true_count - MIN_TC) / BIN_WIDTH);
    return (bin_idx < 0) ? 0 : (bin_idx >= MAX_BINS) ? MAX_BINS - 1 : bin_idx;
}
```

#### **3. Integridade de Splits:**
```c
bool validate_split_record(const SplitBinaryRecord* record) {
    // Verificar se soma das combinações = 1
    int total = record->lose_lose + record->win_win + record->push_push + 
                record->lose_win + record->lose_push + record->win_lose +
                record->win_push + record->push_lose + record->push_win;
    return total == 1;
}
```

---

## 📊 **6. EXEMPLO DE EXECUÇÃO COMPLETA**

### 🚀 **Comando:**
```bash
./blackjack_sim -n 10000 -hist26 -hist70 -histA -split -o complete_analysis
```

### 📁 **Arquivos Gerados:**
```
Resultados/
├── freq_2_17_complete_analysis.csv
├── freq_2_18_complete_analysis.csv
├── ...
├── freq_A_BJ_complete_analysis.csv
├── freq_A_BUST_complete_analysis.csv
├── split_outcome_AA_vs_2_complete_analysis.csv
├── split_outcome_AA_vs_3_complete_analysis.csv
├── ...
└── split_outcome_22_vs_A_complete_analysis.csv
```

### 📈 **Saída do Console:**
```
Simulação concluída!
  Tempo total: 45.32 segundos
  Taxa: 220.6 simulações/segundo
  Jogos processados: 10000000
  Taxa de jogos: 220645 jogos/segundo
  Média de unidades por shoe: -0.0341
Processando dados de análise de frequência...
Análise de frequência concluída!
Processando dados de análise de splits...
Análise de splits concluída!
```

---

## 🎯 **RESUMO DAS FÓRMULAS PRINCIPAIS**

### **Frequência:**
```
Freq(TC_bin, upcard, resultado) = Contagem(TC_bin, upcard, resultado) / Total(TC_bin, upcard) × 100%
```

### **Split Expected Value:**
```
EV = 2.0 × freq_win_win - 2.0 × freq_lose_lose
```

### **Média Unidades/Shoe:**
```
Média = Σ(PNL_todas_as_rodadas) / (num_sims × NUM_SHOES)
```

### **True Count (Wong Halves):**
```
Running_Count = Σ(WONG_HALVES[carta_rank])
True_Count = Running_Count / (cartas_restantes / 52.0)
```

---

**🎖️ O sistema de estatísticas do simulador fornece análise completa e cientificamente rigorosa do comportamento do blackjack em função do true count, com validação de dados e otimizações de performance para simulações de grande escala.** 