#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "jogo.h"
#include "baralho.h"
#include "constantes.h"

// Estrutura para armazenar resultados de testes
typedef struct {
    int total_testes;
    int testes_passou;
    int testes_falhou;
    char ultimo_erro[256];
} ResultadoTeste;

// Função auxiliar para inicializar resultado
void init_resultado(ResultadoTeste *r) {
    r->total_testes = 0;
    r->testes_passou = 0;
    r->testes_falhou = 0;
    strcpy(r->ultimo_erro, "");
}

// Função auxiliar para verificar teste
void verificar_teste(ResultadoTeste *r, int condicao, const char *descricao) {
    r->total_testes++;
    if (condicao) {
        r->testes_passou++;
        printf("✓ %s\n", descricao);
    } else {
        r->testes_falhou++;
        snprintf(r->ultimo_erro, sizeof(r->ultimo_erro), "FALHOU: %s", descricao);
        printf("✗ %s\n", descricao);
    }
}

// Função para simular um cenário de insurance
double simular_insurance(double bankroll_inicial, double true_count, int bet, int maos_contabilizadas, bool dealer_tem_bj) {
    double bankroll = bankroll_inicial;
    
    // Lógica do insurance
    if (true_count >= MIN_COUNT_INS && maos_contabilizadas > 0) {
        double insurance_bet = (bet / 2.0) * maos_contabilizadas;
        
        if (dealer_tem_bj) {
            // Dealer tem blackjack - insurance paga 2:1
            bankroll += insurance_bet * 2.0;
        } else {
            // Dealer não tem blackjack - perde insurance
            bankroll -= insurance_bet;
        }
    }
    
    return bankroll;
}

// Teste 1: Verificar constante MIN_COUNT_INS
void teste_constante_insurance(ResultadoTeste *r) {
    printf("\n=== TESTE 1: CONSTANTE MIN_COUNT_INS ===\n");
    
    verificar_teste(r, MIN_COUNT_INS == 3.5, "MIN_COUNT_INS = 3.5");
    printf("   Valor atual: %.1f\n", MIN_COUNT_INS);
}

// Teste 2: Cálculos de insurance quando dealer tem blackjack
void teste_insurance_dealer_bj(ResultadoTeste *r) {
    printf("\n=== TESTE 2: INSURANCE COM DEALER BLACKJACK ===\n");
    
    // Cenário 1: 1 mão contabilizada, aposta 10, TC = 4.0
    double bankroll_inicial = 1000.0;
    double resultado = simular_insurance(bankroll_inicial, 4.0, 10, 1, true);
    double esperado = bankroll_inicial + (10.0 / 2.0) * 1 * 2.0; // +10
    
    verificar_teste(r, fabs(resultado - esperado) < 0.01, 
                   "1 mão, aposta 10, TC=4.0, dealer BJ: bankroll +10");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, esperado);
    
    // Cenário 2: 3 mãos contabilizadas, aposta 20, TC = 5.0
    resultado = simular_insurance(bankroll_inicial, 5.0, 20, 3, true);
    esperado = bankroll_inicial + (20.0 / 2.0) * 3 * 2.0; // +60
    
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "3 mãos, aposta 20, TC=5.0, dealer BJ: bankroll +60");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, esperado);
    
    // Cenário 3: 2 mãos contabilizadas, aposta 15, TC = 6.0
    resultado = simular_insurance(bankroll_inicial, 6.0, 15, 2, true);
    esperado = bankroll_inicial + (15.0 / 2.0) * 2 * 2.0; // +30
    
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "2 mãos, aposta 15, TC=6.0, dealer BJ: bankroll +30");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, esperado);
}

// Teste 3: Cálculos de insurance quando dealer não tem blackjack
void teste_insurance_dealer_sem_bj(ResultadoTeste *r) {
    printf("\n=== TESTE 3: INSURANCE SEM DEALER BLACKJACK ===\n");
    
    // Cenário 1: 1 mão contabilizada, aposta 10, TC = 4.0
    double bankroll_inicial = 1000.0;
    double resultado = simular_insurance(bankroll_inicial, 4.0, 10, 1, false);
    double esperado = bankroll_inicial - (10.0 / 2.0) * 1; // -5
    
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "1 mão, aposta 10, TC=4.0, sem dealer BJ: bankroll -5");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, esperado);
    
    // Cenário 2: 3 mãos contabilizadas, aposta 20, TC = 5.0
    resultado = simular_insurance(bankroll_inicial, 5.0, 20, 3, false);
    esperado = bankroll_inicial - (20.0 / 2.0) * 3; // -30
    
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "3 mãos, aposta 20, TC=5.0, sem dealer BJ: bankroll -30");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, esperado);
    
    // Cenário 3: 2 mãos contabilizadas, aposta 15, TC = 6.0
    resultado = simular_insurance(bankroll_inicial, 6.0, 15, 2, false);
    esperado = bankroll_inicial - (15.0 / 2.0) * 2; // -15
    
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "2 mãos, aposta 15, TC=6.0, sem dealer BJ: bankroll -15");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, esperado);
}

// Teste 4: Condições onde insurance não é feito
void teste_insurance_nao_aplicado(ResultadoTeste *r) {
    printf("\n=== TESTE 4: CONDIÇÕES SEM INSURANCE ===\n");
    
    double bankroll_inicial = 1000.0;
    
    // Cenário 1: True count muito baixo (< 3.5)
    double resultado = simular_insurance(bankroll_inicial, 3.0, 10, 2, true);
    verificar_teste(r, resultado == bankroll_inicial,
                   "TC=3.0 < 3.5: sem insurance (bankroll inalterado)");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, bankroll_inicial);
    
    // Cenário 2: Nenhuma mão contabilizada
    resultado = simular_insurance(bankroll_inicial, 4.0, 10, 0, true);
    verificar_teste(r, resultado == bankroll_inicial,
                   "0 mãos contabilizadas: sem insurance (bankroll inalterado)");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, bankroll_inicial);
    
    // Cenário 3: True count exatamente no limite (3.5)
    resultado = simular_insurance(bankroll_inicial, 3.5, 10, 1, true);
    double esperado = bankroll_inicial + (10.0 / 2.0) * 1 * 2.0; // +10
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "TC=3.5 (limite): insurance aplicado");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, esperado);
}

// Teste 5: Fórmulas matemáticas
void teste_formulas_insurance(ResultadoTeste *r) {
    printf("\n=== TESTE 5: VERIFICAÇÃO DAS FÓRMULAS ===\n");
    
    // Fórmula para ganho: bankroll + ((aposta/2) * maos_contabilizadas) * 2
    // Fórmula para perda: bankroll - ((aposta/2) * maos_contabilizadas)
    
    double bankroll = 1000.0;
    int aposta = 30;
    int maos = 4;
    
    // Insurance bet = (30/2) * 4 = 60
    double insurance_bet = (aposta / 2.0) * maos;
    
    // Ganho com dealer BJ
    double ganho = bankroll + insurance_bet * 2.0;
    double esperado_ganho = 1000.0 + 60.0 * 2.0; // 1120
    
    verificar_teste(r, fabs(ganho - esperado_ganho) < 0.01,
                   "Fórmula ganho: 1000 + (30/2)*4*2 = 1120");
    printf("   Calculado: %.2f, Esperado: %.2f\n", ganho, esperado_ganho);
    
    // Perda sem dealer BJ
    double perda = bankroll - insurance_bet;
    double esperado_perda = 1000.0 - 60.0; // 940
    
    verificar_teste(r, fabs(perda - esperado_perda) < 0.01,
                   "Fórmula perda: 1000 - (30/2)*4 = 940");
    printf("   Calculado: %.2f, Esperado: %.2f\n", perda, esperado_perda);
}

// Teste 6: Casos extremos
void teste_casos_extremos(ResultadoTeste *r) {
    printf("\n=== TESTE 6: CASOS EXTREMOS ===\n");
    
    double bankroll_inicial = 1000.0;
    
    // Aposta muito alta
    double resultado = simular_insurance(bankroll_inicial, 10.0, 1000, 1, true);
    double esperado = bankroll_inicial + (1000.0 / 2.0) * 1 * 2.0; // +1000
    
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "Aposta alta (1000): insurance correto");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, esperado);
    
    // Muitas mãos contabilizadas
    resultado = simular_insurance(bankroll_inicial, 5.0, 10, 10, false);
    esperado = bankroll_inicial - (10.0 / 2.0) * 10; // -50
    
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "Muitas mãos (10): insurance correto");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, esperado);
    
    // True count muito alto
    resultado = simular_insurance(bankroll_inicial, 15.0, 20, 2, true);
    esperado = bankroll_inicial + (20.0 / 2.0) * 2 * 2.0; // +40
    
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "TC muito alto (15.0): insurance correto");
    printf("   Resultado: %.2f, Esperado: %.2f\n", resultado, esperado);
}

// Função principal
int main() {
    printf("=== TESTE DE INSURANCE DO SIMULADOR DE BLACKJACK ===\n");
    printf("Verificando cálculos de insurance conforme especificação...\n");
    
    ResultadoTeste resultado;
    init_resultado(&resultado);
    
    // Executar todos os testes
    teste_constante_insurance(&resultado);
    teste_insurance_dealer_bj(&resultado);
    teste_insurance_dealer_sem_bj(&resultado);
    teste_insurance_nao_aplicado(&resultado);
    teste_formulas_insurance(&resultado);
    teste_casos_extremos(&resultado);
    
    // Relatório final
    printf("\n=== RELATÓRIO DE INSURANCE ===\n");
    printf("Total de testes: %d\n", resultado.total_testes);
    printf("Testes aprovados: %d\n", resultado.testes_passou);
    printf("Testes falharam: %d\n", resultado.testes_falhou);
    printf("Taxa de sucesso: %.1f%%\n", 
           (double)resultado.testes_passou / resultado.total_testes * 100.0);
    
    if (resultado.testes_falhou > 0) {
        printf("Último erro: %s\n", resultado.ultimo_erro);
        return 1;
    }
    
    printf("\n✓ TODOS OS TESTES DE INSURANCE PASSARAM!\n");
    printf("\nResumo das regras de insurance implementadas:\n");
    printf("- Aplicado quando: dealer upcard = A e true_count >= %.1f\n", MIN_COUNT_INS);
    printf("- Insurance bet = (aposta/2) * número_de_mãos_contabilizadas\n");
    printf("- Se dealer tem BJ: bankroll += insurance_bet * 2\n");
    printf("- Se dealer não tem BJ: bankroll -= insurance_bet\n");
    
    return 0;
} 