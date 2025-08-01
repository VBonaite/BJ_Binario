# Problemas Específicos Identificados no Código de Simulação

## 1. Problema Principal: Arquivos Temporários Não Encontrados

### Diagnóstico
A função `process_frequency_data()` procura por arquivos temporários com nomes como:
- `temp_total_upcard_X_batch_Y`
- `temp_result_upcard_X_final_Z_batch_Y`

Mas esses arquivos não existem porque:

1. **Problema de Nomenclatura**: O código gera arquivos com sufixos `.bin` ou `.lz4`, mas a função de processamento procura sem sufixo
2. **Problema de Timing**: Os arquivos temporários podem estar sendo removidos antes do processamento
3. **Problema de Condições**: As condições para gerar os arquivos podem não estar sendo atendidas

## 2. Problemas Específicos no Código

### 2.1 Inconsistência na Nomenclatura de Arquivos

**Problema**: Na função `flush_freq_buffer()` (simulacao.c ~linha 600), os arquivos são criados com nomes como:
```c
snprintf(result_filepath, sizeof(result_filepath), 
         "temp_result_upcard_%d_final_%s_batch_%d", 
         upcard_value, final_names[f], batch_id);
```

**Mas**: Na função `process_frequency_data()` (main.c ~linha 650), são procurados com o mesmo nome:
```c
snprintf(result_filename, sizeof(result_filename), 
         "temp_result_upcard_%d_final_%s_batch_%d", 
         upcard, final_names[final_val], batch);
```

**Solução**: O problema não é de nomenclatura, mas sim que os arquivos não estão sendo criados.

### 2.2 Problema de Condições de Ativação

**Problema**: As análises só são executadas se as flags específicas estão ativadas:
- `-hist26` para upcards 2-6
- `-hist70` para upcards 7-10  
- `-histA` para upcard A
- `-split` para análise de splits
- `-dealer` para análise de dealer (removido do código atual)

**Diagnóstico**: Pelos arquivos CSV gerados, apenas as análises de frequência foram ativadas.

### 2.3 Problema na Lógica de Coleta de Dados

**Problema**: Na função `simulacao_completa()`, a coleta de dados de frequência depende de várias condições:

```c
if (any_freq_analysis && !freq_data_collected_this_round) {
    // Lógica de coleta
}
```

**Possível Problema**: A variável `freq_data_collected_this_round` pode estar impedindo a coleta adequada de dados.

### 2.4 Problema de Buffers e Flush

**Problema**: Os dados são coletados em buffers locais e só são escritos quando o buffer está quase cheio:

```c
if (freq_buffer_count >= FREQ_BUFFER_THRESHOLD) {
    flush_freq_buffer();
}
```

**Possível Problema**: Se o buffer nunca fica cheio o suficiente, os dados podem não ser escritos.

## 3. Problemas de Implementação Específicos

### 3.1 Cálculo de True Count

**Problema**: O true count é calculado como:
```c
true_count = running_count / (cartas_restantes / 52.0)
```

**Possível Problema**: Se `cartas_restantes` for muito pequeno, o true count pode ficar muito alto e sair dos bins definidos.

### 3.2 Bins de True Count

**Definição Atual**:
```c
#define MAX_BINS 130
#define MIN_TC -6.5
#define MAX_TC 6.5
#define BIN_WIDTH 0.1
```

**Problema**: O range de -6.5 a +6.5 pode não ser suficiente para capturar todos os valores de true count.

### 3.3 Função get_bin_index_robust

**Problema**: A função pode estar retornando índices inválidos para valores extremos de true count.

## 4. Problemas de Arquivos Temporários

### 4.1 Sistema de Compressão

**Problema**: O código usa um sistema de compressão LZ4 que pode estar causando problemas:
- Arquivos podem não estar sendo fechados corretamente
- Compressão pode estar falhando
- Arquivos podem estar sendo criados com extensões incorretas

### 4.2 Limpeza Prematura

**Problema**: Os arquivos temporários podem estar sendo removidos antes do processamento final:

```c
// Remover arquivo temporário (tanto .bin quanto .lz4)
unlink(temp_bin_filename);
unlink(temp_lz4_filename);
```

## 5. Soluções Propostas

### 5.1 Verificação de Arquivos Temporários
- Adicionar logs para verificar se os arquivos estão sendo criados
- Verificar se os arquivos existem antes de tentar removê-los

### 5.2 Correção da Lógica de Flush
- Garantir que os buffers sejam sempre escritos no final da simulação
- Adicionar flush forçado no final de cada simulação

### 5.3 Correção do Range de True Count
- Expandir o range de bins para capturar valores extremos
- Adicionar tratamento para valores fora do range

### 5.4 Correção do Sistema de Arquivos
- Simplificar o sistema de compressão ou adicionar mais verificações
- Garantir que os arquivos sejam fechados corretamente antes do processamento

### 5.5 Adição de Logs de Debug
- Adicionar mais logs para rastrear o fluxo de dados
- Verificar se as condições de coleta estão sendo atendidas

