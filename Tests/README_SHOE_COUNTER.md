# SISTEMA DE CONTAGEM DE CARTAS DO SHOE

## 📋 VISÃO GERAL

O **Shoe Counter** é um sistema avançado de contagem de cartas por rank que rastreia a composição exata do shoe em tempo real durante as simulações de blackjack. Isso permite análises de probabilidades precisas e implementação de estratégias composition-dependent.

## 🎯 FUNCIONALIDADES PRINCIPAIS

### **📊 Contagem em Tempo Real**
- Rastreia cada rank (2-A) individualmente
- Atualiza automaticamente a cada carta distribuída
- Valida consistência interna dos dados

### **🎲 Análises de Probabilidade**
- Probabilidade de Blackjack atual
- Probabilidade de bust ao pedir carta
- Concentração de cartas específicas (10s, Ases)
- Análises composition-dependent

### **⚡ Performance Otimizada**
- Operações O(1) para todas as consultas
- Estrutura de dados compacta
- Integração eficiente com o sistema existente

## 📁 ARQUIVOS DO SISTEMA

```
shoe_counter.h              # Header com definições e funções
shoe_counter.c              # Implementação principal
test_shoe_counter.c         # Testes completos
exemplo_integracao_shoe_counter.c  # Exemplo de integração
README_SHOE_COUNTER.md      # Esta documentação
```

## 🚀 COMPILAÇÃO E TESTES

```bash
# Compilar teste básico
make test_shoe

# Executar testes
./test_shoe_counter

# Compilar exemplo de integração
make exemplo_integracao_shoe_counter

# Executar exemplo
./exemplo_integracao_shoe_counter

# Limpar arquivos compilados
make clean
```

## 🔧 USO BÁSICO

### **1. Inicialização**
```c
#include "shoe_counter.h"

ShoeCounter counter;
shoe_counter_init(&counter, 8);  // 8 decks
```

### **2. Remoção de Cartas**
```c
// A cada carta distribuída
Carta carta = baralho_comprar(&shoe);
shoe_counter_remove_card(&counter, carta);
```

### **3. Consultas de Probabilidade**
```c
// Probabilidade de Blackjack
double bj_prob = shoe_counter_get_blackjack_probability(&counter);

// Probabilidade de bust ao pedir carta
double bust_prob = shoe_counter_get_bust_probability_on_hit(&counter, 16);

// Contagem de cartas específicas
int aces = shoe_counter_get_aces(&counter);
int tens = shoe_counter_get_ten_value_cards(&counter);
```

### **4. Reset para Novo Shoe**
```c
shoe_counter_reset(&counter);
```

## 📊 ESTRUTURA DE DADOS

### **ShoeCounter**
```c
typedef struct {
    int counts[NUM_RANKS];     // Contagem por rank (0-12 = 2-A)
    int total_cards;           // Total de cartas restantes
    int original_decks;        // Número original de decks
    bool initialized;          // Flag de inicialização
} ShoeCounter;
```

### **Mapeamento de Ranks**
| Índice | Rank | Valor | Cartas por Deck |
|--------|------|-------|-----------------|
| 0      | 2    | 2     | 4               |
| 1      | 3    | 3     | 4               |
| ...    | ...  | ...   | 4               |
| 8      | 10   | 10    | 4               |
| 9      | J    | 10    | 4               |
| 10     | Q    | 10    | 4               |
| 11     | K    | 10    | 4               |
| 12     | A    | 11/1  | 4               |

## 🎯 APLICAÇÕES PRÁTICAS

### **1. Composition-Dependent Strategy**
```c
// Analisar diferença entre 16=10+6 vs 16=9+7
ShoeCounter temp_counter = counter;

// Simular composição específica
temp_counter.counts[8]--; // Remove 10
temp_counter.counts[4]--; // Remove 6
temp_counter.total_cards -= 2;

double bust_prob = shoe_counter_get_bust_probability_on_hit(&temp_counter, 16);
```

### **2. Otimização de Apostas**
```c
double bj_prob = shoe_counter_get_blackjack_probability(&counter);
double normal_bj_prob = 0.047; // ~4.7%

if (bj_prob > normal_bj_prob * 1.1) {
    // Aumentar aposta
} else if (bj_prob < normal_bj_prob * 0.9) {
    // Diminuir aposta
}
```

### **3. Análise de Penetração**
```c
int total_initial = counter.original_decks * 52;
int cards_dealt = total_initial - counter.total_cards;
double penetration = (double)cards_dealt / total_initial;
```

## 🔗 INTEGRAÇÃO COM SIMULADOR PRINCIPAL

### **Pontos de Integração:**

1. **Inicialização do Shoe**
   ```c
   // Em simulacao.c, junto com baralho_criar()
   ShoeCounter shoe_counter;
   shoe_counter_init(&shoe_counter, DECKS);
   ```

2. **Distribuição de Cartas**
   ```c
   // A cada carta distribuída, adicionar:
   Carta c = baralho_comprar(&shoe);
   shoe_counter_remove_card(&shoe_counter, c);
   ```

3. **Decisões Estratégicas**
   ```c
   // Usar probabilidades para estratégia avançada
   double bust_prob = shoe_counter_get_bust_probability_on_hit(&shoe_counter, player_total);
   if (bust_prob > threshold) {
       action = ACAO_STAND;
   }
   ```

4. **Sistema de Apostas**
   ```c
   // Ajustar apostas baseado na composição
   double bj_advantage = shoe_counter_get_blackjack_probability(&shoe_counter) / normal_bj_prob;
   bet_multiplier = calculate_bet_multiplier(bj_advantage);
   ```

## 📈 MÉTRICAS E ANÁLISES

### **Probabilidades Calculadas:**
- **Blackjack**: P(A,10) + P(10,A) = 2 × (aces/total) × (tens/(total-1))
- **Bust**: Soma das probabilidades de cartas que excedem 21
- **Concentração**: count[rank] / total_cards

### **Validações Automáticas:**
- Verificação de contagens não-negativas
- Consistência entre soma individual e total
- Validação de inicialização

## ⚠️ CONSIDERAÇÕES IMPORTANTES

### **Performance:**
- Operações O(1) para consultas básicas
- Operações O(13) para análises completas
- Memória: ~60 bytes por counter

### **Precisão:**
- Contagem exata por rank (não aproximada)
- Considera regras específicas do Ás
- Validação automática de consistência

### **Thread Safety:**
- **NÃO é thread-safe** por design (performance)
- Use uma instância por thread em simulações paralelas
- Ou implemente mutexes se necessário

## 🧪 TESTES INCLUÍDOS

1. **Teste de Inicialização**: Verificação de estado inicial
2. **Teste de Remoção**: Atualização correta de contagens
3. **Teste de Análises**: Cálculos de probabilidade
4. **Teste de Simulação**: Cenário completo de jogo
5. **Teste de Reset**: Restauração do estado inicial

## 🔮 POSSÍVEIS EXTENSÕES

### **Futuras Melhorias:**
1. **Histogram Tracking**: Distribuição de true counts
2. **Multi-Deck Analysis**: Comparação entre diferentes penetrações
3. **Strategy Deviation**: Integração com tabelas de desvio
4. **Real-Time Recommendations**: Sugestões automáticas de ação
5. **Advanced Betting**: Sistemas de Kelly Criterion baseados em composição

## 🎯 CONCLUSÃO

O sistema de Shoe Counter fornece uma base sólida para análises avançadas de blackjack, permitindo:

- ✅ **Estratégias composition-dependent precisas**
- ✅ **Otimização de apostas baseada em composição real**
- ✅ **Cálculos de probabilidade em tempo real**
- ✅ **Integração eficiente com simulador existente**
- ✅ **Extensibilidade para análises futuras**

**Resultado:** Simulações mais precisas e estratégias mais sofisticadas com overhead mínimo de performance. 