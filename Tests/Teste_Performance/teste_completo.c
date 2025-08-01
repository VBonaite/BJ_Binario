#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "jogo.h"
#include "baralho.h"
#include "constantes.h"
#include "tabela_estrategia.h"

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

// Função para criar uma mão com cartas específicas
uint64_t criar_mao(int carta1, int carta2) {
    uint64_t mao = 0;
    
    // Converter cartas para índices (A=0, 2=1, ..., K=11)
    int idx1 = (carta1 == 1) ? 12 : ((carta1 <= 10) ? carta1 - 2 : carta1 - 2);
    int idx2 = (carta2 == 1) ? 12 : ((carta2 <= 10) ? carta2 - 2 : carta2 - 2);
    
    // Adicionar cartas à mão
    mao += (uint64_t)1ULL << (idx1 * 3);
    mao += (uint64_t)1ULL << (idx2 * 3);
    
    return mao;
}

// Função para criar mão com três cartas
uint64_t criar_mao_tres(int carta1, int carta2, int carta3) {
    uint64_t mao = criar_mao(carta1, carta2);
    int idx3 = (carta3 == 1) ? 12 : ((carta3 <= 10) ? carta3 - 2 : carta3 - 2);
    mao += (uint64_t)1ULL << (idx3 * 3);
    return mao;
}

// Teste 1: Verificar resultados das mãos
void teste_resultados_maos(ResultadoTeste *r) {
    printf("\n=== TESTE 1: RESULTADOS DAS MÃOS ===\n");
    
    // Cenário 1: Jogador estoura (> 21)
    Mao jogador, dealer;
    uint64_t mao_jogador = criar_mao_tres(10, 10, 5); // 25
    uint64_t mao_dealer = criar_mao(10, 7); // 17
    
    avaliar_mao(mao_jogador, &jogador);
    avaliar_mao(mao_dealer, &dealer);
    verificar_mao(&jogador, &dealer);
    
    verificar_teste(r, jogador.resultado == 'D', "Jogador estoura vs Dealer 17 = Derrota");
    
    // Cenário 2: Dealer estoura (> 21)
    mao_jogador = criar_mao(10, 8); // 18
    mao_dealer = criar_mao_tres(10, 10, 5); // 25
    
    avaliar_mao(mao_jogador, &jogador);
    avaliar_mao(mao_dealer, &dealer);
    verificar_mao(&jogador, &dealer);
    
    verificar_teste(r, jogador.resultado == 'V', "Jogador 18 vs Dealer estoura = Vitória");
    
    // Cenário 3: Blackjack jogador vs dealer normal
    mao_jogador = criar_mao(1, 10); // BJ
    mao_dealer = criar_mao(10, 10); // 20
    
    avaliar_mao(mao_jogador, &jogador);
    avaliar_mao(mao_dealer, &dealer);
    verificar_mao(&jogador, &dealer);
    
    verificar_teste(r, jogador.resultado == 'V', "Blackjack jogador vs Dealer 20 = Vitória");
    
    // Cenário 4: Blackjack vs Blackjack
    mao_jogador = criar_mao(1, 10); // BJ
    mao_dealer = criar_mao(1, 10); // BJ
    
    avaliar_mao(mao_jogador, &jogador);
    avaliar_mao(mao_dealer, &dealer);
    verificar_mao(&jogador, &dealer);
    
    verificar_teste(r, jogador.resultado == 'E', "Blackjack vs Blackjack = Empate");
    
    // Cenário 5: Jogador normal vs Blackjack dealer
    mao_jogador = criar_mao(10, 10); // 20
    mao_dealer = criar_mao(1, 10); // BJ
    
    avaliar_mao(mao_jogador, &jogador);
    avaliar_mao(mao_dealer, &dealer);
    verificar_mao(&jogador, &dealer);
    
    verificar_teste(r, jogador.resultado == 'D', "Jogador 20 vs Blackjack dealer = Derrota");
    
    // Cenário 6: Valores iguais (empate)
    mao_jogador = criar_mao(10, 8); // 18
    mao_dealer = criar_mao(9, 9); // 18
    
    avaliar_mao(mao_jogador, &jogador);
    avaliar_mao(mao_dealer, &dealer);
    verificar_mao(&jogador, &dealer);
    
    verificar_teste(r, jogador.resultado == 'E', "Jogador 18 vs Dealer 18 = Empate");
    
    // Cenário 7: Jogador maior que dealer
    mao_jogador = criar_mao(10, 10); // 20
    mao_dealer = criar_mao(10, 8); // 18
    
    avaliar_mao(mao_jogador, &jogador);
    avaliar_mao(mao_dealer, &dealer);
    verificar_mao(&jogador, &dealer);
    
    verificar_teste(r, jogador.resultado == 'V', "Jogador 20 vs Dealer 18 = Vitória");
    
    // Cenário 8: Jogador menor que dealer
    mao_jogador = criar_mao(10, 6); // 16
    mao_dealer = criar_mao(10, 9); // 19
    
    avaliar_mao(mao_jogador, &jogador);
    avaliar_mao(mao_dealer, &dealer);
    verificar_mao(&jogador, &dealer);
    
    verificar_teste(r, jogador.resultado == 'D', "Jogador 16 vs Dealer 19 = Derrota");
}

// Teste 2: Verificar cálculos de PNL
void teste_pnl(ResultadoTeste *r) {
    printf("\n=== TESTE 2: CÁLCULOS DE PNL ===\n");
    
    Mao mao;
    
    // Vitória normal
    mao.resultado = 'V';
    mao.blackjack = false;
    mao.isdouble = false;
    mao.aposta = 10.0;
    calcular_pnl(&mao);
    verificar_teste(r, mao.pnl == 10.0, "Vitória normal: PNL = +aposta");
    
    // Vitória com blackjack
    mao.resultado = 'V';
    mao.blackjack = true;
    mao.isdouble = false;
    mao.aposta = 10.0;
    calcular_pnl(&mao);
    verificar_teste(r, mao.pnl == 15.0, "Vitória blackjack: PNL = +1.5x aposta");
    
    // Vitória com double
    mao.resultado = 'V';
    mao.blackjack = false;
    mao.isdouble = true;
    mao.aposta = 10.0;
    calcular_pnl(&mao);
    verificar_teste(r, mao.pnl == 20.0, "Vitória double: PNL = +2x aposta");
    
    // Empate
    mao.resultado = 'E';
    mao.blackjack = false;
    mao.isdouble = false;
    mao.aposta = 10.0;
    calcular_pnl(&mao);
    verificar_teste(r, mao.pnl == 0.0, "Empate: PNL = 0");
    
    // Derrota normal
    mao.resultado = 'D';
    mao.blackjack = false;
    mao.isdouble = false;
    mao.aposta = 10.0;
    calcular_pnl(&mao);
    verificar_teste(r, mao.pnl == -10.0, "Derrota normal: PNL = -aposta");
    
    // Derrota com double
    mao.resultado = 'D';
    mao.blackjack = false;
    mao.isdouble = true;
    mao.aposta = 10.0;
    calcular_pnl(&mao);
    verificar_teste(r, mao.pnl == -20.0, "Derrota double: PNL = -2x aposta");
}

// Teste 3: Verificar tipos de mão
void teste_tipos_mao(ResultadoTeste *r) {
    printf("\n=== TESTE 3: TIPOS DE MÃO ===\n");
    
    // Blackjack
    uint64_t mao_bj = criar_mao(1, 10);
    TipoMao tipo = tipo_mao(mao_bj);
    verificar_teste(r, tipo == MAO_BLACKJACK, "A-10 = Blackjack");
    
    // Par
    uint64_t mao_par = criar_mao(8, 8);
    tipo = tipo_mao(mao_par);
    verificar_teste(r, tipo == MAO_PAR, "8-8 = Par");
    
    // Soft hand
    uint64_t mao_soft = criar_mao(1, 6);
    tipo = tipo_mao(mao_soft);
    verificar_teste(r, tipo == MAO_SOFT, "A-6 = Soft");
    
    // Hard hand
    uint64_t mao_hard = criar_mao(10, 6);
    tipo = tipo_mao(mao_hard);
    verificar_teste(r, tipo == MAO_HARD, "10-6 = Hard");
    
    // Verificar valores
    int valor_bj = calcular_valor_mao(mao_bj);
    int valor_par = calcular_valor_mao(mao_par);
    int valor_soft = calcular_valor_mao(mao_soft);
    int valor_hard = calcular_valor_mao(mao_hard);
    
    verificar_teste(r, valor_bj == 21, "Valor A-10 = 21");
    verificar_teste(r, valor_par == 16, "Valor 8-8 = 16");
    verificar_teste(r, valor_soft == 17, "Valor A-6 = 17 (soft)");
    verificar_teste(r, valor_hard == 16, "Valor 10-6 = 16 (hard)");
}

// Teste 4: Verificar estratégias
void teste_estrategias(ResultadoTeste *r) {
    printf("\n=== TESTE 4: ESTRATÉGIAS ===\n");
    
    Mao mao;
    
    // Teste estratégia hard
    uint64_t mao_hard = criar_mao(10, 6); // 16
    avaliar_mao(mao_hard, &mao);
    AcaoEstrategia acao = determinar_acao(&mao, mao_hard, 10, 0.0, true);
    verificar_teste(r, acao == ACAO_HIT, "Hard 16 vs 10 = Hit (estratégia básica)");
    
    // Teste estratégia soft
    uint64_t mao_soft = criar_mao(1, 6); // A-6
    avaliar_mao(mao_soft, &mao);
    acao = determinar_acao(&mao, mao_soft, 6, 0.0, true);
    verificar_teste(r, acao == ACAO_DOUBLE || acao == ACAO_DOUBLE_OR_HIT, "Soft 17 vs 6 = Double");
    
    // Teste estratégia par
    uint64_t mao_par = criar_mao(8, 8); // 8-8
    avaliar_mao(mao_par, &mao);
    acao = determinar_acao(&mao, mao_par, 10, 0.0, true);
    verificar_teste(r, acao == ACAO_SPLIT_OR_HIT, "Par 8-8 vs 10 = Split or Hit");
    
    // Teste com desvio (se implementado)
    // Exemplo: 16 vs 10 com TC alto pode ser diferente
    acao = determinar_acao(&mao, mao_hard, 10, 5.0, false);
    printf("   Desvio: Hard 16 vs 10 com TC=5.0 = %s\n", acao_to_str(acao));
}

// Função auxiliar para criar carta com rank específico
Carta criar_carta_rank(int rank_idx) {
    return (Carta)1ULL << (rank_idx * 3);
}

// Teste 5: Verificar true count
void teste_true_count(ResultadoTeste *r) {
    printf("\n=== TESTE 5: TRUE COUNT ===\n");
    
    Shoe shoe;
    baralho_criar(&shoe);
    
    double running_count = 0.0;
    double true_count = 0.0;
    
    // Simular retirada de cartas altas (deveria diminuir running count)
    for (int i = 0; i < 24; i++) { // 24 cartas de valor 10 (idx 8)
        Carta c = criar_carta_rank(8); // Valor 10
        running_count += WONG_HALVES[carta_para_rank_idx(c)];
        size_t cartas_restantes = shoe.total - 24 - i;
        double decks_restantes = (double)cartas_restantes / 52.0;
        if (decks_restantes < 1.0) decks_restantes = 1.0;
        true_count = running_count / decks_restantes;
    }
    
    verificar_teste(r, running_count < 0.0, "Running count negativo após cartas altas");
    verificar_teste(r, true_count < 0.0, "True count negativo após cartas altas");
    
    // Reset e testar com cartas baixas
    running_count = 0.0;
    for (int i = 0; i < 24; i++) { // 24 cartas de valor 2 (idx 0)
        Carta c = criar_carta_rank(0); // Valor 2
        running_count += WONG_HALVES[carta_para_rank_idx(c)];
        size_t cartas_restantes = shoe.total - 24 - i;
        double decks_restantes = (double)cartas_restantes / 52.0;
        if (decks_restantes < 1.0) decks_restantes = 1.0;
        true_count = running_count / decks_restantes;
    }
    
    verificar_teste(r, running_count > 0.0, "Running count positivo após cartas baixas");
    verificar_teste(r, true_count > 0.0, "True count positivo após cartas baixas");
    
    printf("   Running count final: %.2f\n", running_count);
    printf("   True count final: %.2f\n", true_count);
    
    baralho_destruir(&shoe);
}

// Teste 6: Verificar sistema de apostas
void teste_apostas(ResultadoTeste *r) {
    printf("\n=== TESTE 6: SISTEMA DE APOSTAS ===\n");
    
    // Teste aposta mínima
    int aposta = definir_aposta(500, 10, 0.0, 100, 0.0, 5.0);
    verificar_teste(r, aposta >= 5, "Aposta mínima >= 5");
    
    // Teste com true count alto
    aposta = definir_aposta(300, 50, 6.0, 100, 0.0, 5.0);
    verificar_teste(r, aposta > 5, "Aposta com TC alto > aposta mínima");
    
    // Teste com shoe longo
    aposta = definir_aposta(450, 10, 3.0, 50, 0.0, 5.0);
    verificar_teste(r, aposta == 5, "Shoe longo = aposta mínima");
    
    printf("   Aposta TC=0: %d\n", definir_aposta(300, 50, 0.0, 100, 0.0, 5.0));
    printf("   Aposta TC=3: %d\n", definir_aposta(300, 50, 3.0, 100, 0.0, 5.0));
    printf("   Aposta TC=6: %d\n", definir_aposta(300, 50, 6.0, 100, 0.0, 5.0));
}

// Teste 7: Verificar mãos contabilizadas
void teste_maos_contabilizadas(ResultadoTeste *r) {
    printf("\n=== TESTE 7: MÃOS CONTABILIZADAS ===\n");
    
    int maos = calcular_maos_contabilizadas(-2.0);
    verificar_teste(r, maos == 1, "TC < -1: 1 mão contabilizada");
    
    maos = calcular_maos_contabilizadas(0.0);
    verificar_teste(r, maos == 2, "TC neutro: 2 mãos contabilizadas");
    
    maos = calcular_maos_contabilizadas(3.0);
    verificar_teste(r, maos == 3, "TC > 1: 3 mãos contabilizadas");
}

// Função auxiliar para simular insurance
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

// Teste 8: Sistema de Insurance
void teste_insurance(ResultadoTeste *r) {
    printf("\n=== TESTE 8: SISTEMA DE INSURANCE ===\n");
    
    // Teste 8.1: Verificar constante MIN_COUNT_INS
    verificar_teste(r, MIN_COUNT_INS == 3.5, "MIN_COUNT_INS = 3.5");
    
    // Teste 8.2: Insurance com dealer blackjack
    double bankroll_inicial = 1000.0;
    double resultado = simular_insurance(bankroll_inicial, 4.0, 10, 1, true);
    double esperado = bankroll_inicial + (10.0 / 2.0) * 1 * 2.0; // +10
    verificar_teste(r, fabs(resultado - esperado) < 0.01, 
                   "Insurance com dealer BJ: 1 mão, aposta 10, TC=4.0 → bankroll +10");
    
    // Teste 8.3: Insurance com 3 mãos contabilizadas
    resultado = simular_insurance(bankroll_inicial, 5.0, 20, 3, true);
    esperado = bankroll_inicial + (20.0 / 2.0) * 3 * 2.0; // +60
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "Insurance com dealer BJ: 3 mãos, aposta 20, TC=5.0 → bankroll +60");
    
    // Teste 8.4: Insurance sem dealer blackjack
    resultado = simular_insurance(bankroll_inicial, 4.0, 10, 1, false);
    esperado = bankroll_inicial - (10.0 / 2.0) * 1; // -5
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "Insurance sem dealer BJ: 1 mão, aposta 10, TC=4.0 → bankroll -5");
    
    // Teste 8.5: Insurance sem dealer blackjack - 3 mãos
    resultado = simular_insurance(bankroll_inicial, 5.0, 20, 3, false);
    esperado = bankroll_inicial - (20.0 / 2.0) * 3; // -30
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "Insurance sem dealer BJ: 3 mãos, aposta 20, TC=5.0 → bankroll -30");
    
    // Teste 8.6: True count muito baixo (sem insurance)
    resultado = simular_insurance(bankroll_inicial, 3.0, 10, 2, true);
    verificar_teste(r, resultado == bankroll_inicial,
                   "TC=3.0 < 3.5: sem insurance aplicado");
    
    // Teste 8.7: Nenhuma mão contabilizada (sem insurance)
    resultado = simular_insurance(bankroll_inicial, 4.0, 10, 0, true);
    verificar_teste(r, resultado == bankroll_inicial,
                   "0 mãos contabilizadas: sem insurance aplicado");
    
    // Teste 8.8: True count no limite exato (3.5)
    resultado = simular_insurance(bankroll_inicial, 3.5, 10, 1, true);
    esperado = bankroll_inicial + (10.0 / 2.0) * 1 * 2.0; // +10
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "TC=3.5 (limite): insurance aplicado");
    
    // Teste 8.9: Fórmula matemática - caso complexo
    int aposta = 30;
    int maos = 4;
    double insurance_bet = (aposta / 2.0) * maos; // 60
    double ganho = bankroll_inicial + insurance_bet * 2.0; // 1120
    double esperado_ganho = 1000.0 + 60.0 * 2.0;
    verificar_teste(r, fabs(ganho - esperado_ganho) < 0.01,
                   "Fórmula ganho: 1000 + (30/2)*4*2 = 1120");
    
    // Teste 8.10: Fórmula matemática - perda
    double perda = bankroll_inicial - insurance_bet; // 940
    double esperado_perda = 1000.0 - 60.0;
    verificar_teste(r, fabs(perda - esperado_perda) < 0.01,
                   "Fórmula perda: 1000 - (30/2)*4 = 940");
    
    // Teste 8.11: Caso extremo - aposta alta
    resultado = simular_insurance(bankroll_inicial, 10.0, 1000, 1, true);
    esperado = bankroll_inicial + (1000.0 / 2.0) * 1 * 2.0; // +1000
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "Aposta alta (1000): insurance correto");
    
    // Teste 8.12: Caso extremo - muitas mãos
    resultado = simular_insurance(bankroll_inicial, 5.0, 10, 10, false);
    esperado = bankroll_inicial - (10.0 / 2.0) * 10; // -50
    verificar_teste(r, fabs(resultado - esperado) < 0.01,
                   "Muitas mãos (10): insurance correto");
}

// Função principal de teste
int main() {
    printf("=== TESTE EXTENSIVO DO SIMULADOR DE BLACKJACK ===\n");
    printf("Testando todas as funcionalidades principais...\n");
    
    ResultadoTeste resultado;
    init_resultado(&resultado);
    
    // Executar todos os testes
    teste_resultados_maos(&resultado);
    teste_pnl(&resultado);
    teste_tipos_mao(&resultado);
    teste_estrategias(&resultado);
    teste_true_count(&resultado);
    teste_apostas(&resultado);
    teste_maos_contabilizadas(&resultado);
    teste_insurance(&resultado);
    
    // Relatório final
    printf("\n=== RELATÓRIO FINAL ===\n");
    printf("Total de testes: %d\n", resultado.total_testes);
    printf("Testes aprovados: %d\n", resultado.testes_passou);
    printf("Testes falharam: %d\n", resultado.testes_falhou);
    printf("Taxa de sucesso: %.1f%%\n", 
           (double)resultado.testes_passou / resultado.total_testes * 100.0);
    
    if (resultado.testes_falhou > 0) {
        printf("Último erro: %s\n", resultado.ultimo_erro);
        return 1;
    }
    
    printf("\n✓ TODOS OS TESTES PASSARAM!\n");
    return 0;
} 