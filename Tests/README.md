# SIMULADOR DE BLACKJACK

## 📋 ORGANIZAÇÃO DO PROJETO

### **Pasta Principal (BJ_Binario/)**
Contém apenas os **arquivos principais** do simulador:

#### **Sistema Principal:**
- `main.c` - Programa principal
- `blackjack_sim` - Executável compilado
- `simulacao.c/h` - Engine de simulação
- `baralho.c/h` - Sistema de cartas
- `jogo.c/h` - Lógica do blackjack
- `constantes.c/h` - Configurações
- `tabela_estrategia.c/h` - Estratégia básica
- `rng.c/h` - Gerador de números aleatórios
- `saidas.c/h` - Sistema de outputs
- `structures.h` - Estruturas de dados

#### **Sistemas de Análise:**
- `shoe_counter.c/h` - Contagem de cartas por rank
- `split_ev_lookup.c/h` - Tabela de lookup de EV de splits
- `dealer_freq_lookup.c/h` - Tabela de frequências do dealer

### **Pasta de Testes (tests/)**
Contém todos os **testes, exemplos e documentação**:

#### **Testes:**
- `test_shoe_counter.c` - Teste do sistema de contagem
- `test_lookup_tables.c` - Teste das tabelas de lookup
- `test_baralho` - Teste do sistema de baralho

#### **Exemplos:**
- `exemplo_integracao_shoe_counter.c` - Como usar contagem de cartas
- `exemplo_uso_lookup.c` - Como usar lookup tables

#### **Documentação:**
- `README_SHOE_COUNTER.md` - Documentação do shoe counter
- `README_LOOKUP_TABLES.md` - Documentação das lookup tables

## 🚀 COMPILAÇÃO

### **Programa Principal:**
```bash
# Na pasta principal
make                    # Compila simulador principal
make clean             # Limpa arquivos compilados
./blackjack_sim        # Executa simulador
```

### **Testes e Exemplos:**
```bash
# Na pasta tests/
cd tests
make help              # Lista opções disponíveis
make test_shoe         # Compila teste do shoe counter
make run_tests         # Executa todos os testes
make run_examples      # Executa todos os exemplos
```

## 🎯 USO PRINCIPAL

```bash
# Simulação básica
./blackjack_sim -n 1000

# Análise de splits
./blackjack_sim -split -n 10000 -o resultado

# Análise de frequências do dealer
./blackjack_sim -hist26 -n 5000 -o freq_2_6
```

## 📊 SISTEMAS IMPLEMENTADOS

✅ **Simulador Principal** - Engine completo de blackjack  
✅ **Sistema de Apostas** - Progressão baseada em True Count  
✅ **Análise de Splits** - EV para todas as combinações  
✅ **Frequências do Dealer** - Análise por upcard e TC  
✅ **Shoe Counter** - Contagem de cartas em tempo real  
✅ **Lookup Tables** - Consultas otimizadas de dados  
✅ **Estratégia Básica** - Implementação super-otimizada  
✅ **Sistema de Desvios** - Desvios de estratégia por TC  

**Total:** ~50.000 linhas de código C otimizado para performance máxima. 