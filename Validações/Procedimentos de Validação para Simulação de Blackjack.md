# Procedimentos de Validação para Simulação de Blackjack

## 1. Visão Geral dos Procedimentos

Este documento estabelece os procedimentos operacionais para validar completamente uma simulação de blackjack, garantindo que todos os aspectos do jogo, cálculos matemáticos e coleta de estatísticas estejam funcionando corretamente.

## 2. Pré-Requisitos

### 2.1 Ambiente de Teste
- Sistema operacional: Linux/Unix (recomendado) ou Windows
- Compilador C: GCC 7.0+ ou equivalente
- Python 3.8+ com bibliotecas: numpy, pandas, scipy, matplotlib
- Espaço em disco: mínimo 1GB para dados de teste
- RAM: mínimo 4GB para simulações grandes

### 2.2 Arquivos Necessários
- Código fonte da simulação completo
- Framework de testes (`test_framework.c`)
- Script de testes estatísticos (`statistical_tests.py`)
- Dados de referência (se disponíveis)

### 2.3 Preparação do Ambiente
```bash
# Compilar framework de testes
gcc -o test_framework test_framework.c -lm

# Instalar dependências Python
pip install numpy pandas scipy matplotlib

# Criar diretórios de trabalho
mkdir -p ./Resultados
mkdir -p ./TestResults
```

## 3. Procedimento de Validação Completa

### 3.1 Fase 1: Testes Unitários Críticos

**Objetivo**: Validar componentes individuais do sistema.

**Passos**:
1. Executar framework de testes C:
   ```bash
   ./test_framework
   ```

2. Verificar saída:
   - Todos os testes de Prioridade 1 (Crítica) devem passar
   - Taxa de aprovação deve ser 100% para testes críticos

3. **Critérios de Aprovação**:
   - T1.1-T1.8: Integridade do shoe (100% aprovação)
   - T2.1-T2.7: Sequência de distribuição (100% aprovação)
   - T4.1-T4.5: Cálculo de mãos (100% aprovação)
   - T9.1-T9.10: True count (100% aprovação)

4. **Ação se Falhar**: Corrigir código antes de prosseguir.

### 3.2 Fase 2: Simulação de Teste Pequena

**Objetivo**: Validar integração de componentes com volume pequeno.

**Passos**:
1. Executar simulação pequena:
   ```bash
   ./simulacao -n 1000 -hist26 -hist70 -histA -split -l 500 -debug -o teste_pequeno
   ```

2. Verificar arquivos gerados:
   ```bash
   ls -la Resultados/
   wc -l Resultados/*.csv
   ```

3. **Critérios de Aprovação**:
   - Pasta `Resultados` criada
   - Arquivo de log com ~500 linhas
   - Arquivos de frequência para upcards 2-10 e A
   - Arquivos de split (se splits ocorreram)
   - Nenhuma mensagem de erro crítico

4. **Verificações Específicas**:
   ```bash
   # Verificar estrutura do log
   head -5 Resultados/log_teste_pequeno.csv
   
   # Verificar dados de frequência
   head -5 Resultados/freq_*_teste_pequeno.csv
   
   # Contar registros válidos
   grep -v "0.0000" Resultados/freq_*_teste_pequeno.csv | wc -l
   ```

### 3.3 Fase 3: Simulação de Teste Média

**Objetivo**: Validar estabilidade e precisão com volume médio.

**Passos**:
1. Executar simulação média:
   ```bash
   ./simulacao -n 5000 -hist26 -hist70 -histA -split -l 2000 -debug -o teste_medio
   ```

2. Executar testes estatísticos básicos:
   ```bash
   python3 statistical_tests.py ./Resultados
   ```

3. **Critérios de Aprovação**:
   - Taxa de aprovação dos testes estatísticos ≥ 80%
   - House edge entre 0.3% e 0.8%
   - Frequência de blackjack entre 4.3% e 5.3%
   - Distribuição de upcards aproximadamente uniforme

### 3.4 Fase 4: Simulação de Produção

**Objetivo**: Validar sistema completo com volume de produção.

**Passos**:
1. Executar simulação completa:
   ```bash
   ./simulacao -n 15000 -hist26 -hist70 -histA -split -l 10000 -o producao
   ```

2. Executar bateria completa de testes:
   ```bash
   python3 statistical_tests.py ./Resultados
   ```

3. **Critérios de Aprovação**:
   - Taxa de aprovação dos testes estatísticos ≥ 90%
   - Todos os valores dentro de tolerâncias especificadas
   - Nenhum teste de aleatoriedade falhando (p-value > 0.05)

## 4. Procedimentos de Teste por Categoria

### 4.1 Validação de Integridade do Jogo

**Checklist de Verificação**:
- [ ] Shoe inicializado com composição correta
- [ ] Embaralhamento produz ordem aleatória
- [ ] Distribuição segue sequência correta
- [ ] Blackjacks detectados corretamente
- [ ] Valores de mãos calculados corretamente
- [ ] Splits processados independentemente
- [ ] Doubles funcionam corretamente
- [ ] Resultados determinados corretamente

**Procedimento**:
1. Executar `test_framework` e verificar testes T1.1-T8.6
2. Revisar logs de debug para anomalias
3. Verificar manualmente algumas mãos do log

### 4.2 Validação de Cálculos Matemáticos

**Checklist de Verificação**:
- [ ] True count calculado corretamente
- [ ] Running count atualizado adequadamente
- [ ] Bins de true count corretos
- [ ] Frequências calculadas corretamente
- [ ] Expected values de split corretos
- [ ] Estatísticas de cartas usadas corretas

**Procedimento**:
1. Executar testes T9.1-T10.10
2. Verificar fórmulas manualmente em amostra pequena
3. Comparar com cálculos independentes

### 4.3 Validação Estatística

**Checklist de Verificação**:
- [ ] House edge dentro do esperado
- [ ] Frequências de resultados corretas
- [ ] Distribuições seguem padrões teóricos
- [ ] Testes de aleatoriedade passam
- [ ] Autocorrelação baixa
- [ ] Dados consistentes entre execuções

**Procedimento**:
1. Executar `statistical_tests.py`
2. Comparar com valores teóricos conhecidos
3. Executar múltiplas simulações para consistência

## 5. Procedimentos de Diagnóstico

### 5.1 Diagnóstico de Falhas Comuns

**Problema**: Testes de integridade do shoe falhando
**Diagnóstico**:
```bash
# Verificar inicialização do shoe
grep -i "shoe" debug.log
grep -i "deck" debug.log
```
**Soluções Comuns**:
- Verificar função de inicialização do baralho
- Confirmar número correto de decks
- Verificar função de embaralhamento

**Problema**: Cálculos de true count incorretos
**Diagnóstico**:
```bash
# Verificar valores Wong Halves
grep -i "wong" debug.log
grep -i "true.*count" debug.log
```
**Soluções Comuns**:
- Verificar tabela de valores Wong Halves
- Confirmar cálculo de decks restantes
- Verificar timing da contagem

**Problema**: Estatísticas com muitos zeros
**Diagnóstico**:
```bash
# Verificar coleta de dados
grep -i "buffer" debug.log
grep -i "flush" debug.log
ls -la temp_*
```
**Soluções Comuns**:
- Verificar se buffers estão sendo escritos
- Confirmar que análises estão ativadas
- Verificar arquivos temporários

### 5.2 Verificação de Integridade de Dados

**Procedimento de Verificação**:
```bash
# Verificar arquivos CSV
for file in Resultados/*.csv; do
    echo "=== $file ==="
    wc -l "$file"
    head -3 "$file"
    tail -3 "$file"
    echo
done

# Verificar consistência de dados
python3 -c "
import pandas as pd
import glob

for file in glob.glob('Resultados/*.csv'):
    try:
        df = pd.read_csv(file)
        print(f'{file}: {len(df)} linhas, {len(df.columns)} colunas')
        if len(df) == 0:
            print(f'  AVISO: Arquivo vazio')
        if df.isnull().sum().sum() > 0:
            print(f'  AVISO: Valores nulos encontrados')
    except Exception as e:
        print(f'{file}: ERRO - {e}')
"
```

## 6. Critérios de Aceitação por Fase

### 6.1 Fase 1 - Testes Unitários
**Critérios Obrigatórios**:
- 100% dos testes críticos (Prioridade 1) devem passar
- Nenhum erro de segmentação ou crash
- Todas as funções básicas funcionando

**Critérios Opcionais**:
- 95% de todos os testes passando
- Performance adequada (< 1 segundo para testes)

### 6.2 Fase 2 - Teste Pequeno
**Critérios Obrigatórios**:
- Simulação completa sem crashes
- Arquivos de saída gerados
- Dados básicos coletados

**Critérios Opcionais**:
- Logs de debug sem erros
- Estatísticas preliminares razoáveis

### 6.3 Fase 3 - Teste Médio
**Critérios Obrigatórios**:
- 80% dos testes estatísticos passando
- House edge entre 0.2% e 1.0%
- Frequência de blackjack entre 4.0% e 5.5%

**Critérios Opcionais**:
- 90% dos testes estatísticos passando
- Todas as distribuições dentro de 2 desvios padrão

### 6.4 Fase 4 - Teste de Produção
**Critérios Obrigatórios**:
- 90% dos testes estatísticos passando
- Todos os valores críticos dentro de tolerâncias
- Testes de aleatoriedade passando (p > 0.05)

**Critérios Opcionais**:
- 95% dos testes estatísticos passando
- Performance otimizada (> 1000 simulações/segundo)

## 7. Documentação de Resultados

### 7.1 Relatório de Validação

Para cada execução de validação, documentar:

**Informações Básicas**:
- Data e hora da execução
- Versão do código testado
- Parâmetros de simulação usados
- Ambiente de teste (OS, compilador, etc.)

**Resultados dos Testes**:
- Taxa de aprovação por categoria
- Lista de testes que falharam
- Valores medidos vs esperados
- Mensagens de erro relevantes

**Análise Estatística**:
- House edge medido
- Frequências de resultados principais
- Resultados de testes de aleatoriedade
- Comparação com execuções anteriores

**Conclusões**:
- Status geral (APROVADO/REPROVADO)
- Problemas identificados
- Recomendações de correção
- Próximos passos

### 7.2 Template de Relatório

```
RELATÓRIO DE VALIDAÇÃO - SIMULAÇÃO DE BLACKJACK
===============================================

Data: [DATA]
Versão: [VERSÃO]
Executado por: [NOME]

CONFIGURAÇÃO DE TESTE:
- Simulações: [NÚMERO]
- Parâmetros: [PARÂMETROS]
- Ambiente: [DETALHES]

RESULTADOS DOS TESTES:
- Testes Unitários: [X/Y] ([%])
- Testes de Integração: [X/Y] ([%])
- Testes Estatísticos: [X/Y] ([%])

MÉTRICAS PRINCIPAIS:
- House Edge: [VALOR] (esperado: 0.5%)
- Freq. Blackjack: [VALOR] (esperado: 4.8%)
- Aleatoriedade: [STATUS]

PROBLEMAS IDENTIFICADOS:
[LISTA DE PROBLEMAS]

STATUS FINAL: [APROVADO/REPROVADO]

RECOMENDAÇÕES:
[LISTA DE RECOMENDAÇÕES]
```

## 8. Manutenção e Atualização

### 8.1 Revisão Periódica
- Executar validação completa a cada mudança significativa no código
- Revisar critérios de aceitação trimestralmente
- Atualizar valores teóricos conforme necessário

### 8.2 Melhoria Contínua
- Adicionar novos testes conforme problemas são descobertos
- Otimizar performance dos testes
- Automatizar execução quando possível

### 8.3 Versionamento
- Manter histórico de resultados de validação
- Documentar mudanças nos procedimentos
- Rastrear evolução da qualidade ao longo do tempo

