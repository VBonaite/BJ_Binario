# Simulador de Blackjack - Sistema de Estatísticas Avançadas

## Visão Geral

Este é um simulador de blackjack de alta performance desenvolvido em C com suporte a análise estatística avançada. O sistema implementa estratégia básica otimizada, desvios de estratégia baseados em true count, e 12 categorias diferentes de coleta de estatísticas.

## Características Principais

- **Alta Performance**: ~100-150 simulações/segundo com otimizações nativas
- **Multithreading**: Suporte automático para múltiplas threads
- **Estratégia Otimizada**: Implementação super-otimizada (38% mais rápida) da estratégia básica
- **Desvios de Estratégia**: Sistema completo de desvios baseado em true count
- **Análise Estatística**: 12 categorias diferentes de coleta de dados
- **Sistema de Lotes**: Processamento eficiente com arquivos temporários

## Compilação

```bash
make clean && make
```

## Uso Básico

```bash
# Simulação básica
./blackjack_sim -n 1000

# Com múltiplas threads
./blackjack_sim -n 10000 -t 8

# Com análise estatística
./blackjack_sim -n 5000 -hist3 -hist5 -hist7 -o minha_analise

# Ajuda completa
./blackjack_sim -h
```

## Opções de Linha de Comando

| Opção | Descrição |
|-------|-----------|
| `-n <num>` | Número de simulações (default: 1000) |
| `-t <num>` | Número de threads (default: CPUs disponíveis) |
| `-l <num>` | Linhas de log total (0 = sem log) |
| `-o <suffix>` | Sufixo para arquivos de saída |
| `-d` | Desativar desvios de estratégia |
| `-dealer` | Ativar análise de dealer |
| `-hist` | Ativar histograma de dealer (legado) |
| `-histX` | Ativar coleta estatística X (1-12) |
| `-h` | Mostrar ajuda |

## Sistema de Estatísticas

### Categorias Implementadas (1-10)

#### 1. **Dealer Outcomes** (`-hist1`)
- **Arquivo**: `dealer_outcome_upcard_X_result_Y_sufixo.csv`
- **Descrição**: Frequência de resultados do dealer por upcard e true count
- **Dados**: Bins de true count vs resultados (17,18,19,20,21,BJ,bust)

#### 2. **Dealer BJ Frequencies** (`-hist2`)
- **Arquivo**: `dealer_blackjack_sufixo.csv`
- **Descrição**: Frequência de blackjack do dealer por true count
- **Dados**: Bins de true count vs probabilidade de blackjack

#### 3. **Player Bust Hard** (`-hist3`)
- **Arquivo**: `player_bust_hard_X_sufixo.csv` (X = 12-16)
- **Descrição**: Análise de busto em totais duros
- **Dados**: Estatísticas de bust por true count (frequência, win/push/lose)

#### 4. **Player Bust Soft** (`-hist4`)
- **Arquivo**: `player_bust_soft_X_sufixo.csv` (X = 13-19)
- **Descrição**: Análise de busto em totais suaves
- **Dados**: Similar ao bust hard, mas para mãos suaves

#### 5. **Double Outcome Hard** (`-hist5`)
- **Arquivo**: `double_outcome_hard_X_sufixo.csv` (X = 9,10,11)
- **Descrição**: Resultados de doubles em totais duros
- **Dados**: Distribuição de resultados finais, win/push/lose, EV

#### 6. **Double Outcome Soft** (`-hist6`)
- **Arquivo**: `double_outcome_soft_X_sufixo.csv` (X = 13-18)
- **Descrição**: Resultados de doubles em totais suaves
- **Dados**: Similar ao double hard, mas para mãos suaves

#### 7. **Splits Críticos** (`-hist7`)
- **Arquivos**: 
  - `split_outcome_XX_sufixo.csv` (resultado líquido)
  - `split_hands_XX_sufixo.csv` (mãos individuais)
- **Pares**: AA, 88, 99, 77
- **Descrição**: Análise de splits críticos em dois níveis

#### 8. **Splits Secundários** (`-hist8`)
- **Arquivos**: 
  - `split_secondary_outcome_XX_sufixo.csv`
  - `split_secondary_hands_XX_sufixo.csv`
- **Pares**: 66, 22, 33, 44
- **Descrição**: Análise de splits secundários em dois níveis

#### 9. **Shoe Composition** (`-hist9`)
- **Arquivo**: `shoe_composition_by_tc_sufixo.csv`
- **Descrição**: Composição do shoe por true count
- **Dados**: Contagem média de cada rank (A,2-K) por bin de TC

#### 10. **Validation Stats** (`-hist10`)
- **Arquivo**: `validation_statistics_sufixo.csv`
- **Descrição**: Estatísticas de validação do sistema
- **Dados**: Métricas gerais, distribuição de cartas, edge da casa

### Categorias Implementadas (11)

#### 11. **Advanced Metrics** (`-hist11`)
- **Arquivo**: `advanced_metrics_sufixo.csv`
- **Descrição**: Métricas avançadas de performance e risco
- **Dados**: Análise por bins de true count com métricas de PNL, betting spread, bankroll management, variância por tipo de mão, desvios, splits/doubles, insurance, e métricas de risco (Sharpe ratio, Kelly fraction, risk of ruin)

### Categorias Não Implementadas (12)

#### 12. **Custom Analysis** (`-hist12`)
- Status: Não implementado
- Proposta: Análise customizada pelo usuário

## Estrutura de Dados

### Bins de True Count
- **Range**: -6.5 a +6.5
- **Largura**: 0.1
- **Total**: 131 bins

### Sistema de Lotes
- **Tamanho**: 10.000 simulações por lote
- **Arquivos temporários**: `temp_*_batch_X.csv`
- **Processamento**: Consolidação automática no final

### Mãos Contabilizadas
- Apenas mãos com `contabilizada = true` são incluídas nas estatísticas
- Critério baseado em true count e distribuição de apostas

## Arquivos de Saída

### Estrutura de Diretórios
```
./Resultados/
├── dealer_blackjack_*.csv
├── dealer_outcome_*.csv
├── player_bust_hard_*.csv
├── player_bust_soft_*.csv
├── double_outcome_hard_*.csv
├── double_outcome_soft_*.csv
├── split_outcome_*.csv
├── split_hands_*.csv
├── split_secondary_*.csv
├── shoe_composition_*.csv
├── validation_statistics_*.csv
└── analise_constantes.txt
```

### Formato de Dados
- **Separador**: Vírgula (CSV)
- **Encoding**: UTF-8
- **Headers**: Sempre incluídos
- **Precisão**: 6 casas decimais para percentuais

## Exemplos de Uso

### Análise Básica de Dealer
```bash
./blackjack_sim -n 5000 -hist1 -hist2 -o dealer_analysis
```

### Análise de Doubles
```bash
./blackjack_sim -n 10000 -hist5 -hist6 -o double_analysis
```

### Análise Completa de Splits
```bash
./blackjack_sim -n 20000 -hist7 -hist8 -o split_analysis
```

### Análise de Validação
```bash
./blackjack_sim -n 50000 -hist10 -o validation_test
```

### Métricas Avançadas
```bash
./blackjack_sim -n 100000 -hist11 -o advanced_metrics_test
```

### Análise Completa
```bash
./blackjack_sim -n 100000 -hist1 -hist2 -hist3 -hist4 -hist5 -hist6 -hist7 -hist8 -hist9 -hist10 -hist11 -o full_analysis
```

## Performance

### Benchmarks Típicos
- **Simulações simples**: ~150 sim/s
- **Com estatísticas**: ~100-120 sim/s
- **Análise completa**: ~80-100 sim/s

### Otimizações
- Estratégia básica em lookup tables
- Avaliação de mãos otimizada com bitwise
- Processamento em lotes para I/O
- Multithreading automático

## Limitações

1. **Contagem de cartas**: Apenas validação básica implementada (bug conhecido em Q/K)
2. **Memória**: Uso intensivo para análises completas
3. **Precisão**: Limitada pela implementação de ponto flutuante
4. **Compatibilidade**: Testado apenas em Linux

## Desenvolvimento

### Estrutura do Código
- `main.c`: Controle principal e processamento de dados
- `simulacao.c`: Engine de simulação e coleta de dados
- `jogo.c`: Lógica de jogo e estratégia
- `baralho.c`: Manipulação de cartas e shoe
- `tabela_estrategia.c`: Estratégia básica otimizada

### Contribuições
- Implementação de categorias 11-12
- Correção do bug de contagem de cartas
- Otimizações de performance
- Testes de validação adicionais

## Licença

Este projeto é distribuído sob licença MIT. Veja o arquivo LICENSE para detalhes.

## Changelog

### v1.0.0 (2024-07-09)
- Implementação completa de 10 categorias de estatísticas
- Sistema de parsing de argumentos robusto
- Documentação completa
- Otimizações de performance
- Sistema de lotes para I/O eficiente 