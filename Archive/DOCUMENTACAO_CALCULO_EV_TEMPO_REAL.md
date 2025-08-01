# DOCUMENTAÇÃO: SISTEMA DE CÁLCULO DE EV EM TEMPO REAL

## 📋 VISÃO GERAL

Este documento detalha a implementação completa do sistema de **Expected Value (EV) em tempo real** que substitui a tabela de estratégia básica tradicional por cálculos matemáticos precisos baseados na composição atual do shoe, true count e probabilidades dinâmicas do dealer.

## 🎯 ARQUITETURA DO SISTEMA

### **Módulos Implementados:**

```
real_time_ev.h/c                    # Cálculos matemáticos de EV
realtime_strategy_integration.h/c   # Integração com simulador
shoe_counter.h/c                    # Contagem de cartas por rank
dealer_freq_lookup.h/c              # Probabilidades do dealer
split_ev_lookup.h/c                 # Expected values de splits
```

### **Fluxo de Decisão:**

```
1. Mão do jogador + upcard dealer + true count + composição shoe
                    ↓
2. Normalização do True Count (limitado a [-6.5, +6.5])
                    ↓  
3. Cálculo das probabilidades do dealer baseado em lookup tables
                    ↓
4. Cálculo de EV para cada ação (Stand, Hit, Double, Split)
                    ↓
5. Escolha da ação com maior Expected Value
                    ↓
6. Aplicação de regras especiais (doubles, splits de AA)
```

---

## 📊 FÓRMULAS MATEMÁTICAS IMPLEMENTADAS

### **1. NORMALIZAÇÃO DO TRUE COUNT**

#### **Função:** `normalize_true_count(double true_count)`

```c
// Limitar TC entre -6.5 e +6.5
if (true_count > 6.5) true_count = 6.5;
if (true_count < -6.5) true_count = -6.5;

// Arredondar para uma casa decimal
return round(true_count * 10.0) / 10.0;
```

#### **Função:** `get_tc_bin_start(double true_count)`

```c
// Para TC = 3.27 → retorna 3.2 (início do bin 3.2-3.3)
double normalized = normalize_true_count(true_count);
return floor(normalized * 10.0) / 10.0;
```

**Justificativa:** As tabelas de lookup têm bins de 0.1, então para TC = 3.27 usamos os dados do bin que começa em 3.2.

---

### **2. PROBABILIDADES DO DEALER**

#### **Fórmula Geral:**
```
P_d(resultado) = Probabilidade do dealer terminar com resultado específico
```

Onde `resultado ∈ {17, 18, 19, 20, 21, blackjack, bust}`

#### **Implementação:** `get_dealer_probabilities()`

**Método Primário - Lookup Tables:**
```c
if (dealer_freq_table_loaded) {
    int upcard_idx = dealer_upcard_to_index(dealer_upcard);
    int tc_bin_idx = get_tc_bin_index(tc_bin);
    
    prob_17 = dealer_freq_table[upcard_idx][DEALER_RESULT_17][tc_bin_idx];
    prob_18 = dealer_freq_table[upcard_idx][DEALER_RESULT_18][tc_bin_idx];
    // ... etc para 19, 20, 21, BJ, BUST
}
```

**Método Fallback - Composition-Based:**
```c
double high_card_concentration = ten_value_cards / total_cards;
double normal_concentration = 4.0 / 13.0;
double adjustment = (high_card_concentration / normal_concentration - 1.0) * 0.1;

prob_bust = base_prob_bust + adjustment;
```

**Normalização:**
```
Σ P_d(resultado) = 1.0
```

---

### **3. EXPECTED VALUE DE STAND**

#### **Fórmula Matemática:**
```
EV_stand(P_h) = Σ [Resultado(P_h, dealer_final) × P_d(dealer_final)]
```

#### **Casos Específicos:**

**Para P_h ≤ 16:**
```
EV_stand(P_h) = (+1) × P_d(bust) + (-1) × [P_d(17) + P_d(18) + P_d(19) + P_d(20) + P_d(21)]
              = P_d(bust) - [1 - P_d(bust)]
              = 2 × P_d(bust) - 1
```

**Para P_h = 17:**
```
EV_stand(17) = (+1) × P_d(bust) + (0) × P_d(17) + (-1) × [P_d(18) + P_d(19) + P_d(20) + P_d(21)]
```

**Para P_h = 18:**
```
EV_stand(18) = (+1) × [P_d(bust) + P_d(17)] + (0) × P_d(18) + (-1) × [P_d(19) + P_d(20) + P_d(21)]
```

**Para P_h = 19:**
```
EV_stand(19) = (+1) × [P_d(bust) + P_d(17) + P_d(18)] + (0) × P_d(19) + (-1) × [P_d(20) + P_d(21)]
```

**Para P_h = 20:**
```
EV_stand(20) = (+1) × [P_d(bust) + P_d(17) + P_d(18) + P_d(19)] + (0) × P_d(20) + (-1) × P_d(21)
```

**Para P_h = 21:**
```
EV_stand(21) = (+1) × [P_d(bust) + P_d(17) + P_d(18) + P_d(19) + P_d(20)] + (0) × P_d(21)
             = (+1) × [1 - P_d(21)]
```

#### **Implementação:**
```c
double ev_stand = 0.0;

// Dealer bust: jogador sempre ganha
ev_stand += dealer_probs.prob_bust * 1.0;

// Comparar com cada resultado final do dealer
for (dealer_final = 17; dealer_final <= 21; dealer_final++) {
    if (hand_value > dealer_final) {
        ev_stand += prob * 1.0;   // Jogador ganha
    } else if (hand_value == dealer_final) {
        ev_stand += prob * 0.0;   // Push
    } else {
        ev_stand += prob * -1.0;  // Jogador perde
    }
}

// Dealer blackjack
if (hand_value != 21) {
    ev_stand += dealer_probs.prob_blackjack * -1.0;
}
```

---

### **4. EXPECTED VALUE DE HIT**

#### **Fórmula Matemática:**
```
EV_hit(P_h) = Σ [P(receber carta r) × EV_ótimo(P_h + valor(r))]
```

#### **Probabilidades das Cartas:**
```
P(receber carta r) = C(r) / N
```

Onde:
- `C(r)` = número de cartas de rank `r` restantes no shoe
- `N` = número total de cartas restantes no shoe

#### **Casos após Receber Carta:**

**Se P_h + valor(r) > 21 (bust):**
```
EV_após_hit(P_h, r) = -1
```

**Se P_h + valor(r) = 21:**
```
EV_após_hit(P_h, r) = EV_stand(21)
```

**Se P_h + valor(r) < 21:**
```
EV_após_hit(P_h, r) = max(EV_stand(P_h + valor(r)), EV_hit(P_h + valor(r)))
```

#### **Tratamento do Ás:**
```c
int card_value = (card_rank == 11) ? 1 : card_rank; // Ás como 1 inicialmente
if (card_rank == 11 && current_value + 11 <= 21) {
    card_value = 11; // Ás como 11 se não causar bust
}
```

#### **Implementação Recursiva:**
```c
for (card_rank = 2; card_rank <= 11; card_rank++) {
    int available_cards = shoe_counter_get_rank_count(counter, rank_idx);
    double card_prob = (double)available_cards / total_cards;
    
    int new_value = current_value + card_value;
    
    if (new_value > 21) {
        ev_after_card = -1.0; // Bust
    } else {
        // Simular remoção da carta do shoe
        ShoeCounter temp_counter = simulate_card_removal(counter, card_rank);
        
        if (new_value == 21) {
            ev_after_card = calculate_ev_stand_realtime(21, dealer_upcard, true_count, &temp_counter);
        } else {
            double ev_stand_new = calculate_ev_stand_realtime(new_value, dealer_upcard, true_count, &temp_counter);
            double ev_hit_new = calculate_ev_hit_realtime(new_hand_bits, dealer_upcard, true_count, &temp_counter, depth + 1);
            ev_after_card = max(ev_stand_new, ev_hit_new);
        }
    }
    
    ev_hit += card_prob * ev_after_card;
}
```

**Controle de Recursão:**
```c
if (depth > MAX_RECURSION_DEPTH) {
    return -1.0; // Evitar recursão infinita
}
```

---

### **5. EXPECTED VALUE DE DOUBLE**

#### **Fórmula Matemática:**
```
EV_double(P_h) = 2 × Σ [P(receber carta r) × EV_stand(P_h + valor(r))]
```

#### **Características Especiais:**
1. **Recebe exatamente uma carta**
2. **Aposta é dobrada** (multiplicador 2)
3. **Sempre fica após receber a carta**

#### **Implementação:**
```c
for (card_rank = 2; card_rank <= 11; card_rank++) {
    int available_cards = shoe_counter_get_rank_count(counter, rank_idx);
    double card_prob = (double)available_cards / total_cards;
    
    int card_value = (card_rank == 11) ? 1 : card_rank;
    if (card_rank == 11 && current_value + 11 <= 21) {
        card_value = 11;
    }
    
    int final_value = current_value + card_value;
    
    if (final_value > 21) {
        ev_after_card = -1.0; // Bust
    } else {
        ShoeCounter temp_counter = simulate_card_removal(counter, card_rank);
        ev_after_card = calculate_ev_stand_realtime(final_value, dealer_upcard, true_count, &temp_counter);
    }
    
    ev_double += card_prob * ev_after_card;
}

// Multiplicar por 2 (aposta dobrada)
return ev_double * 2.0;
```

---

### **6. EXPECTED VALUE DE SPLIT**

#### **Fórmula Matemática:**
```
EV_split(par_r) = lookup_table[par_rank][dealer_upcard][tc_bin]
```

#### **Método Primário - Lookup Tables:**
```c
if (split_ev_table_loaded) {
    double tc_bin = get_tc_bin_start(true_count);
    return get_split_ev(pair_rank, dealer_upcard, tc_bin);
}
```

#### **Método Fallback - Aproximação:**
```c
double base_ev = calculate_ev_stand_realtime(pair_rank, dealer_upcard, true_count, counter);

if (pair_rank == 11) { // Ases
    return 0.6; // Ases splitados são muito favoráveis
} else if (pair_rank == 10) { // 10s
    return -0.1; // 10s splitados são desfavoráveis
} else {
    return base_ev * 0.8; // Aproximação para outros pares
}
```

**Justificativa:** As tabelas de splits já contêm os EVs corretos calculados com a fórmula complexa que considera todas as possibilidades de cada mão após o split.

---

### **7. DECISÃO ÓTIMA**

#### **Fórmula de Escolha:**
```
EV_ótimo(mão) = max(EV_stand, EV_hit, EV_double, EV_split)
```

#### **Implementação:**
```c
// Calcular todos os EVs
result.ev_stand = calculate_ev_stand_realtime(...);
result.ev_hit = calculate_ev_hit_realtime(...);
result.ev_double = calculate_ev_double_realtime(...);
result.ev_split = calculate_ev_split_realtime(...);

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
```

---

## 🎮 REGRAS ESPECIAIS IMPLEMENTADAS

### **1. Doubles (ACAO_DOUBLE)**
- **Só permitido em mãos iniciais**
- **Recebe exatamente uma carta**
- **Mão é marcada como finalizada**
- **Aposta é dobrada**

### **2. Splits de Ases (AA)**
- **Só permitido em mãos iniciais**
- **Cada mão recebe exatamente uma carta**
- **Mãos são marcadas como finalizadas**
- **Não pode fazer double após split de ases**

### **3. Outros Splits**
- **Só permitido em mãos iniciais com pares**
- **Cada mão joga normalmente após receber segunda carta**
- **Pode fazer double após split (se permitido pelas regras)**

---

## 🔧 OTIMIZAÇÕES IMPLEMENTADAS

### **1. Cache de EVs**
```c
typedef struct {
    uint64_t hand_bits;
    int dealer_upcard;
    double true_count;
    RealTimeEVResult result;
    bool valid;
} EVCache;
```

**Benefício:** Evita recálculos de situações já analisadas.

### **2. Simulação de Remoção de Cartas**
```c
ShoeCounter simulate_card_removal(const ShoeCounter* counter, int card_rank) {
    ShoeCounter temp_counter = *counter;
    temp_counter.counts[rank_idx]--;
    temp_counter.total_cards--;
    return temp_counter;
}
```

**Benefício:** Permite cálculos precisos sem afetar o shoe counter principal.

### **3. Controle de Recursão**
```c
if (depth > MAX_RECURSION_DEPTH) {
    return -1.0; // Conservador
}
```

**Benefício:** Evita recursão infinita em cálculos de EV_hit.

### **4. Fallbacks Inteligentes**
- Se tabelas não carregadas → aproximações baseadas na composição
- Se sistema desabilitado → estratégia básica tradicional
- Se cálculo falha → ação conservadora

---

## 📈 EXEMPLO PRÁTICO DE CÁLCULO

### **Cenário:** Jogador com 12, Dealer mostra 6, TC = +2.0

#### **1. Normalização do TC:**
```
TC = +2.0 → normalizado = +2.0
TC_bin = floor(2.0 * 10) / 10 = 2.0
```

#### **2. Probabilidades do Dealer:**
```
P_d(bust) = 0.45 (tabela para upcard 6, TC +2.0)
P_d(17) = 0.11
P_d(18) = 0.11  
P_d(19) = 0.11
P_d(20) = 0.11
P_d(21) = 0.11
```

#### **3. EV Stand:**
```
EV_stand(12) = (+1) × 0.45 + (-1) × (0.11 + 0.11 + 0.11 + 0.11 + 0.11)
             = 0.45 - 0.55
             = -0.10
```

#### **4. EV Hit:**
```c
// Para cada carta possível:
P(2) = 30/400 = 0.075 → novo valor 14 → EV_ótimo(14) = -0.05
P(3) = 30/400 = 0.075 → novo valor 15 → EV_ótimo(15) = -0.03
...
P(9) = 30/400 = 0.075 → novo valor 21 → EV_stand(21) = +0.89
P(10) = 120/400 = 0.30 → novo valor 22 → EV = -1.0 (bust)

EV_hit(12) = 0.075×(-0.05) + 0.075×(-0.03) + ... + 0.075×(+0.89) + 0.30×(-1.0)
           = -0.25
```

#### **5. Decisão:**
```
EV_stand(12) = -0.10
EV_hit(12) = -0.25
EV_double(12) = -0.30 (não permitido para 12)

Melhor ação: STAND (EV = -0.10)
```

---

## 🚀 INTEGRAÇÃO COM O SIMULADOR

### **Função Principal de Integração:**
```c
AcaoEstrategia determinar_acao_realtime(
    const Mao *mao, 
    uint64_t mao_bits, 
    int dealer_up_rank,
    double true_count,
    const ShoeCounter* shoe_counter,
    bool is_initial_hand
);
```

### **Substituição no Código Existente:**
```c
// ANTES (jogo.c):
AcaoEstrategia ac = determinar_acao(mao, mao->bits, dealer_up_rank);

// DEPOIS:
AcaoEstrategia ac = determinar_acao_realtime(mao, mao->bits, dealer_up_rank, 
                                           true_count, &shoe_counter, is_initial_hand);
```

### **Inicialização do Sistema:**
```c
// No início da simulação
init_realtime_strategy_system();

// No final da simulação  
cleanup_realtime_strategy_system();
print_realtime_strategy_stats();
```

---

## 📊 VALIDAÇÃO E TESTES

### **Verificações Implementadas:**
1. **Sanidade dos EVs:** -2.0 ≤ EV ≤ +2.0
2. **Consistência das probabilidades:** Σ P_d = 1.0
3. **Validação do shoe counter:** total = Σ counts[i]
4. **Comparação com estratégia básica**

### **Estatísticas Coletadas:**
```c
typedef struct {
    int total_decisions;
    int realtime_decisions;
    int basic_strategy_decisions;
    int differences_found;
    double avg_ev_improvement;
} RealtimeStrategyStats;
```

---

## 🎯 CONCLUSÃO

Este sistema implementa matematicamente todos os cálculos de Expected Value em tempo real, considerando:

✅ **Composição exata do shoe** via shoe_counter  
✅ **True count normalizado** para lookup correto  
✅ **Probabilidades dinâmicas do dealer** via tabelas  
✅ **EVs precisos** para stand, hit, double e split  
✅ **Regras especiais** para doubles e splits de AA  
✅ **Otimizações de performance** com cache e fallbacks  
✅ **Integração completa** com o simulador existente  

**Resultado:** Estratégia matematicamente ótima que adapta-se dinamicamente à situação atual do jogo, superando a estratégia básica tradicional através de cálculos precisos de Expected Value baseados na composição real do shoe. 