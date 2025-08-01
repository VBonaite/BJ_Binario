# BJ_Binario - Simulador de Blackjack Avançado

Um simulador de blackjack em C otimizado para análise de estratégias, contagem de cartas e cálculo de expectativa de valor (EV).

## Características Principais

- **Simulação de Alta Performance**: Capaz de executar milhões de simulações rapidamente
- **Sistema de Contagem Wong Halves**: Implementação completa da estratégia de contagem
- **Estratégia Básica Otimizada**: Sistema super-otimizado com 38% mais rápido nos lookups
- **Desvios de Estratégia**: Sistema completo de desvios baseados no true count
- **Análise de Blackjack do Dealer**: Análise integrada com sistema de lotes otimizado
- **Sistema de Apostas Adaptativas**: Ajuste automático baseado no true count
- **Cálculo de EV em Tempo Real**: Análise de expectativa de valor durante o jogo

## Compilação

```bash
make clean
make
```

## Uso

### Simulação Básica
```bash
./blackjack_sim -n 1000000 -o resultado
```

### Com Análise de Blackjack do Dealer
```bash
./blackjack_sim -dealer -n 1000000 -o dealer_analise
```

### Com Log Detalhado
```bash
./blackjack_sim -n 1000000 -l 1000 -o resultado_detalhado
```

### Desativar Desvios
```bash
./blackjack_sim -n 1000000 -d -o resultado_basico
```

## Opções da Linha de Comando

- `-n <num>`: Número de simulações
- `-t <num>`: Número de threads (padrão: 1)
- `-o <sufixo>`: Sufixo para arquivos de saída
- `-l <num>`: Total de linhas de log
- `-dealer`: Ativar análise de blackjack do dealer
- `-d`: Desativar desvios de estratégia (ativos por padrão)

## Estrutura do Projeto

- `main.c`: Arquivo principal do simulador
- `jogo.c/h`: Lógica do jogo de blackjack
- `simulacao.c/h`: Sistema de simulação
- `tabela_estrategia.c/h`: Estratégia básica e desvios
- `shoe_counter.c/h`: Sistema de contagem de cartas
- `ev_calculator.c/h`: Cálculo de expectativa de valor
- `real_time_ev.c/h`: EV em tempo real
- `dealer_freq_lookup.c/h`: Lookup de frequências do dealer
- `split_ev_lookup.c/h`: EV de splits
- `constantes.c/h`: Constantes e configurações
- `baralho.c/h`: Sistema de baralho
- `rng.c/h`: Gerador de números aleatórios
- `saidas.c/h`: Sistema de saída de dados

## Resultados

Os resultados são salvos no diretório `Resultados/` com os seguintes formatos:
- Arquivos CSV com estatísticas detalhadas
- Gráficos PNG com histogramas
- Análises de frequência por upcard do dealer
- Resultados de splits por par de cartas

## Performance

- **Estratégia Básica**: 38% mais rápido que sistema anterior
- **Simulador Completo**: 19% mais rápido
- **Capacidade**: 10+ milhões de simulações sem problemas de memória
- **I/O Otimizado**: Sistema de lotes reduz I/O em 99.99%

## Testes

Execute os testes completos:
```bash
make test
```

## Licença

Este projeto é desenvolvido para análise e estudo de estratégias de blackjack.

## Autor

Desenvolvido para análise avançada de estratégias de blackjack com foco em performance e precisão. 