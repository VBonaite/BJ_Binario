#!/bin/bash

# Script de Automação para Validação Completa da Simulação de Blackjack
# Executa todos os testes necessários conforme o relatório executivo

set -e  # Parar em caso de erro

# Configurações
SIMULATION_BINARY="./simulacao"
TEST_FRAMEWORK="./test_framework"
STATISTICAL_TESTS="statistical_tests.py"
RESULTS_DIR="./Resultados"
TEST_RESULTS_DIR="./TestResults"
LOG_FILE="validation_log.txt"

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Função para logging
log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1" | tee -a "$LOG_FILE"
}

error() {
    echo -e "${RED}[ERRO]${NC} $1" | tee -a "$LOG_FILE"
}

success() {
    echo -e "${GREEN}[SUCESSO]${NC} $1" | tee -a "$LOG_FILE"
}

warning() {
    echo -e "${YELLOW}[AVISO]${NC} $1" | tee -a "$LOG_FILE"
}

# Função para verificar pré-requisitos
check_prerequisites() {
    log "Verificando pré-requisitos..."
    
    # Verificar arquivos necessários
    if [ ! -f "$SIMULATION_BINARY" ]; then
        error "Binário de simulação não encontrado: $SIMULATION_BINARY"
        exit 1
    fi
    
    if [ ! -f "$TEST_FRAMEWORK" ]; then
        warning "Framework de testes não encontrado. Tentando compilar..."
        if [ -f "test_framework.c" ]; then
            gcc -o test_framework test_framework.c -lm
            if [ $? -eq 0 ]; then
                success "Framework de testes compilado com sucesso"
            else
                error "Falha ao compilar framework de testes"
                exit 1
            fi
        else
            error "Código fonte do framework não encontrado: test_framework.c"
            exit 1
        fi
    fi
    
    if [ ! -f "$STATISTICAL_TESTS" ]; then
        error "Script de testes estatísticos não encontrado: $STATISTICAL_TESTS"
        exit 1
    fi
    
    # Verificar Python e dependências
    if ! command -v python3 &> /dev/null; then
        error "Python3 não encontrado"
        exit 1
    fi
    
    # Verificar bibliotecas Python
    python3 -c "import numpy, pandas, scipy, matplotlib" 2>/dev/null
    if [ $? -ne 0 ]; then
        warning "Algumas bibliotecas Python podem estar ausentes"
        log "Tentando instalar dependências..."
        pip3 install numpy pandas scipy matplotlib
    fi
    
    success "Pré-requisitos verificados"
}

# Função para preparar ambiente
prepare_environment() {
    log "Preparando ambiente de teste..."
    
    # Criar diretórios
    mkdir -p "$RESULTS_DIR"
    mkdir -p "$TEST_RESULTS_DIR"
    
    # Limpar resultados anteriores
    rm -f "$RESULTS_DIR"/*.csv
    rm -f "$TEST_RESULTS_DIR"/*
    
    # Inicializar log
    echo "=== VALIDAÇÃO SIMULAÇÃO BLACKJACK - $(date) ===" > "$LOG_FILE"
    
    success "Ambiente preparado"
}

# Fase 1: Testes Unitários Críticos
run_unit_tests() {
    log "=== FASE 1: TESTES UNITÁRIOS CRÍTICOS ==="
    
    log "Executando framework de testes..."
    if ./"$TEST_FRAMEWORK" > "$TEST_RESULTS_DIR/unit_tests.log" 2>&1; then
        success "Framework de testes executado"
        
        # Verificar resultados
        if grep -q "TODOS OS TESTES CRÍTICOS PASSARAM" "$TEST_RESULTS_DIR/unit_tests.log"; then
            success "Todos os testes unitários críticos passaram"
            return 0
        else
            error "Alguns testes unitários falharam"
            cat "$TEST_RESULTS_DIR/unit_tests.log"
            return 1
        fi
    else
        error "Falha ao executar framework de testes"
        cat "$TEST_RESULTS_DIR/unit_tests.log"
        return 1
    fi
}

# Fase 2: Simulação de Teste Pequena
run_small_simulation() {
    log "=== FASE 2: SIMULAÇÃO DE TESTE PEQUENA ==="
    
    local sim_params="-n 1000 -hist26 -hist70 -histA -split -l 500 -debug -o teste_pequeno"
    
    log "Executando simulação pequena: $sim_params"
    if timeout 300 $SIMULATION_BINARY $sim_params > "$TEST_RESULTS_DIR/small_sim.log" 2>&1; then
        success "Simulação pequena concluída"
        
        # Verificar arquivos gerados
        local files_count=$(ls -1 "$RESULTS_DIR"/*.csv 2>/dev/null | wc -l)
        if [ "$files_count" -gt 0 ]; then
            success "Arquivos CSV gerados: $files_count"
            
            # Verificar log
            local log_file="$RESULTS_DIR/log_teste_pequeno.csv"
            if [ -f "$log_file" ]; then
                local log_lines=$(wc -l < "$log_file")
                log "Arquivo de log: $log_lines linhas"
                if [ "$log_lines" -gt 100 ]; then
                    success "Log com dados suficientes"
                    return 0
                else
                    warning "Log com poucas linhas: $log_lines"
                    return 1
                fi
            else
                error "Arquivo de log não encontrado"
                return 1
            fi
        else
            error "Nenhum arquivo CSV gerado"
            return 1
        fi
    else
        error "Simulação pequena falhou ou timeout"
        cat "$TEST_RESULTS_DIR/small_sim.log"
        return 1
    fi
}

# Fase 3: Simulação de Teste Média
run_medium_simulation() {
    log "=== FASE 3: SIMULAÇÃO DE TESTE MÉDIA ==="
    
    local sim_params="-n 5000 -hist26 -hist70 -histA -split -l 2000 -debug -o teste_medio"
    
    log "Executando simulação média: $sim_params"
    if timeout 1800 $SIMULATION_BINARY $sim_params > "$TEST_RESULTS_DIR/medium_sim.log" 2>&1; then
        success "Simulação média concluída"
        
        # Executar testes estatísticos básicos
        log "Executando testes estatísticos básicos..."
        if python3 "$STATISTICAL_TESTS" "$RESULTS_DIR" > "$TEST_RESULTS_DIR/medium_stats.log" 2>&1; then
            success "Testes estatísticos básicos concluídos"
            
            # Verificar taxa de aprovação
            if grep -q "Taxa de aprovação:" "$TEST_RESULTS_DIR/medium_stats.log"; then
                local approval_rate=$(grep "Taxa de aprovação:" "$TEST_RESULTS_DIR/medium_stats.log" | grep -o '[0-9.]*%')
                log "Taxa de aprovação: $approval_rate"
                
                # Extrair número da taxa
                local rate_num=$(echo "$approval_rate" | grep -o '[0-9.]*')
                if (( $(echo "$rate_num >= 80" | bc -l) )); then
                    success "Taxa de aprovação adequada: $approval_rate"
                    return 0
                else
                    warning "Taxa de aprovação baixa: $approval_rate"
                    return 1
                fi
            else
                warning "Taxa de aprovação não encontrada no log"
                return 1
            fi
        else
            error "Falha nos testes estatísticos básicos"
            cat "$TEST_RESULTS_DIR/medium_stats.log"
            return 1
        fi
    else
        error "Simulação média falhou ou timeout"
        cat "$TEST_RESULTS_DIR/medium_sim.log"
        return 1
    fi
}

# Fase 4: Simulação de Produção
run_production_simulation() {
    log "=== FASE 4: SIMULAÇÃO DE PRODUÇÃO ==="
    
    local sim_params="-n 15000 -hist26 -hist70 -histA -split -l 10000 -o producao"
    
    log "Executando simulação de produção: $sim_params"
    log "AVISO: Esta fase pode demorar vários minutos..."
    
    if timeout 3600 $SIMULATION_BINARY $sim_params > "$TEST_RESULTS_DIR/production_sim.log" 2>&1; then
        success "Simulação de produção concluída"
        
        # Executar bateria completa de testes
        log "Executando bateria completa de testes estatísticos..."
        if python3 "$STATISTICAL_TESTS" "$RESULTS_DIR" > "$TEST_RESULTS_DIR/production_stats.log" 2>&1; then
            success "Bateria completa de testes concluída"
            
            # Verificar taxa de aprovação
            if grep -q "Taxa de aprovação:" "$TEST_RESULTS_DIR/production_stats.log"; then
                local approval_rate=$(grep "Taxa de aprovação:" "$TEST_RESULTS_DIR/production_stats.log" | grep -o '[0-9.]*%')
                log "Taxa de aprovação final: $approval_rate"
                
                # Extrair número da taxa
                local rate_num=$(echo "$approval_rate" | grep -o '[0-9.]*')
                if (( $(echo "$rate_num >= 90" | bc -l) )); then
                    success "Taxa de aprovação excelente: $approval_rate"
                    return 0
                else
                    warning "Taxa de aprovação abaixo do ideal: $approval_rate"
                    return 1
                fi
            else
                warning "Taxa de aprovação não encontrada no log"
                return 1
            fi
        else
            error "Falha na bateria completa de testes"
            cat "$TEST_RESULTS_DIR/production_stats.log"
            return 1
        fi
    else
        error "Simulação de produção falhou ou timeout"
        cat "$TEST_RESULTS_DIR/production_sim.log"
        return 1
    fi
}

# Função para gerar relatório final
generate_final_report() {
    log "=== GERANDO RELATÓRIO FINAL ==="
    
    local report_file="$TEST_RESULTS_DIR/relatorio_validacao_final.txt"
    
    cat > "$report_file" << EOF
RELATÓRIO DE VALIDAÇÃO - SIMULAÇÃO DE BLACKJACK
===============================================

Data: $(date '+%Y-%m-%d %H:%M:%S')
Executado por: $(whoami)
Sistema: $(uname -a)

CONFIGURAÇÃO DE TESTE:
- Simulação binária: $SIMULATION_BINARY
- Framework de testes: $TEST_FRAMEWORK
- Testes estatísticos: $STATISTICAL_TESTS

RESULTADOS POR FASE:
EOF

    # Verificar resultados de cada fase
    if [ -f "$TEST_RESULTS_DIR/unit_tests.log" ]; then
        if grep -q "TODOS OS TESTES CRÍTICOS PASSARAM" "$TEST_RESULTS_DIR/unit_tests.log"; then
            echo "- Fase 1 (Testes Unitários): PASSOU" >> "$report_file"
        else
            echo "- Fase 1 (Testes Unitários): FALHOU" >> "$report_file"
        fi
    else
        echo "- Fase 1 (Testes Unitários): NÃO EXECUTADA" >> "$report_file"
    fi
    
    if [ -f "$TEST_RESULTS_DIR/small_sim.log" ]; then
        if [ -f "$RESULTS_DIR/log_teste_pequeno.csv" ]; then
            echo "- Fase 2 (Simulação Pequena): PASSOU" >> "$report_file"
        else
            echo "- Fase 2 (Simulação Pequena): FALHOU" >> "$report_file"
        fi
    else
        echo "- Fase 2 (Simulação Pequena): NÃO EXECUTADA" >> "$report_file"
    fi
    
    if [ -f "$TEST_RESULTS_DIR/medium_stats.log" ]; then
        local medium_rate=$(grep "Taxa de aprovação:" "$TEST_RESULTS_DIR/medium_stats.log" | grep -o '[0-9.]*%' || echo "N/A")
        echo "- Fase 3 (Simulação Média): Taxa $medium_rate" >> "$report_file"
    else
        echo "- Fase 3 (Simulação Média): NÃO EXECUTADA" >> "$report_file"
    fi
    
    if [ -f "$TEST_RESULTS_DIR/production_stats.log" ]; then
        local prod_rate=$(grep "Taxa de aprovação:" "$TEST_RESULTS_DIR/production_stats.log" | grep -o '[0-9.]*%' || echo "N/A")
        echo "- Fase 4 (Simulação Produção): Taxa $prod_rate" >> "$report_file"
    else
        echo "- Fase 4 (Simulação Produção): NÃO EXECUTADA" >> "$report_file"
    fi
    
    cat >> "$report_file" << EOF

ARQUIVOS GERADOS:
$(ls -la "$RESULTS_DIR"/*.csv 2>/dev/null | wc -l) arquivos CSV no diretório $RESULTS_DIR

LOGS DE EXECUÇÃO:
- Log principal: $LOG_FILE
- Logs de teste: $TEST_RESULTS_DIR/

PRÓXIMOS PASSOS:
1. Revisar logs detalhados em caso de falhas
2. Corrigir problemas identificados
3. Re-executar validação após correções
4. Documentar resultados finais

EOF

    success "Relatório final gerado: $report_file"
    
    # Mostrar resumo na tela
    log "=== RESUMO FINAL ==="
    cat "$report_file"
}

# Função principal
main() {
    log "Iniciando validação completa da simulação de blackjack..."
    
    local phase1_result=0
    local phase2_result=0
    local phase3_result=0
    local phase4_result=0
    
    # Verificar pré-requisitos
    check_prerequisites
    
    # Preparar ambiente
    prepare_environment
    
    # Executar fases sequencialmente
    if run_unit_tests; then
        phase1_result=1
        
        if run_small_simulation; then
            phase2_result=1
            
            if run_medium_simulation; then
                phase3_result=1
                
                if run_production_simulation; then
                    phase4_result=1
                fi
            fi
        fi
    fi
    
    # Gerar relatório final
    generate_final_report
    
    # Determinar status final
    if [ $phase1_result -eq 1 ] && [ $phase2_result -eq 1 ] && [ $phase3_result -eq 1 ] && [ $phase4_result -eq 1 ]; then
        success "=== VALIDAÇÃO COMPLETA: TODAS AS FASES PASSARAM ==="
        exit 0
    elif [ $phase1_result -eq 1 ] && [ $phase2_result -eq 1 ] && [ $phase3_result -eq 1 ]; then
        warning "=== VALIDAÇÃO PARCIAL: 3 DE 4 FASES PASSARAM ==="
        exit 1
    elif [ $phase1_result -eq 1 ] && [ $phase2_result -eq 1 ]; then
        warning "=== VALIDAÇÃO BÁSICA: 2 DE 4 FASES PASSARAM ==="
        exit 2
    elif [ $phase1_result -eq 1 ]; then
        warning "=== VALIDAÇÃO MÍNIMA: APENAS TESTES UNITÁRIOS PASSARAM ==="
        exit 3
    else
        error "=== VALIDAÇÃO FALHOU: TESTES UNITÁRIOS FALHARAM ==="
        exit 4
    fi
}

# Verificar se bc está disponível (para comparações numéricas)
if ! command -v bc &> /dev/null; then
    echo "Instalando bc para comparações numéricas..."
    sudo apt-get update && sudo apt-get install -y bc 2>/dev/null || true
fi

# Executar função principal
main "$@"

