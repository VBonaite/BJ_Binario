#!/usr/bin/env python3
"""
Script para validar se os valores de Expected Value foram corrigidos corretamente.
"""

import pandas as pd
import numpy as np

def validar_ev_arquivo(arquivo_path):
    """Valida se o EV está calculado corretamente"""
    
    df = pd.read_csv(arquivo_path)
    
    # Calcular EV manualmente
    p1 = df['mao1_lose&mao2_lose_frequency']
    p2 = df['mao1_win&mao2_win_frequency']
    p5 = df['mao1_lose&mao2_push_frequency']
    p7 = df['mao1_win&mao2_push_frequency']
    p8 = df['mao1_push&mao2_lose_frequency']
    p9 = df['mao1_push&mao2_win_frequency']
    
    ev_calculado = -2.0 * p1 + 2.0 * p2 - p5 + p7 - p8 + p9
    ev_arquivo = df['expected_value']
    
    # Verificar se são iguais (com tolerância para arredondamento)
    diferenca_maxima = abs(ev_calculado - ev_arquivo).max()
    
    return diferenca_maxima < 1e-6

def main():
    """Função principal de validação"""
    print("🔍 VALIDAÇÃO DOS VALORES DE EV CORRIGIDOS")
    print("=" * 50)
    
    # Testar arquivo exemplo
    arquivo_teste = "/mnt/dados/BJ_Binario/Resultados/splits/split_outcome_AA_vs_A_3M.csv"
    
    df = pd.read_csv(arquivo_teste)
    
    # Exemplo da primeira linha
    row = df.iloc[0]
    p1 = row['mao1_lose&mao2_lose_frequency']
    p2 = row['mao1_win&mao2_win_frequency']
    p5 = row['mao1_lose&mao2_push_frequency']
    p7 = row['mao1_win&mao2_push_frequency']
    p8 = row['mao1_push&mao2_lose_frequency']
    p9 = row['mao1_push&mao2_win_frequency']
    
    ev_manual = -2.0 * p1 + 2.0 * p2 - p5 + p7 - p8 + p9
    ev_arquivo = row['expected_value']
    
    print("VERIFICAÇÃO MANUAL (primeira linha):")
    print(f"  TC: {row['true_count_center']}")
    print(f"  P(1) lose/lose: {p1:.6f}")
    print(f"  P(2) win/win: {p2:.6f}")
    print(f"  P(5) lose/push: {p5:.6f}")
    print(f"  P(7) win/push: {p7:.6f}")
    print(f"  P(8) push/lose: {p8:.6f}")
    print(f"  P(9) push/win: {p9:.6f}")
    print()
    print(f"  EV manual: -2×{p1:.6f} + 2×{p2:.6f} - {p5:.6f} + {p7:.6f} - {p8:.6f} + {p9:.6f}")
    print(f"  EV manual: {ev_manual:.6f}")
    print(f"  EV arquivo: {ev_arquivo:.6f}")
    print(f"  Diferença: {abs(ev_manual - ev_arquivo):.8f}")
    
    if validar_ev_arquivo(arquivo_teste):
        print("\n✅ VALIDAÇÃO PASSOU! Os valores estão corretos.")
    else:
        print("\n❌ ERRO na validação!")
    
    # Verificar se soma das probabilidades = 1
    probabilidades = ['mao1_lose&mao2_lose_frequency', 'mao1_win&mao2_win_frequency', 
                     'mao1_push&mao2_push_frequency', 'mao1_lose&mao2_win_frequency',
                     'mao1_lose&mao2_push_frequency', 'mao1_win&mao2_lose_frequency',
                     'mao1_win&mao2_push_frequency', 'mao1_push&mao2_lose_frequency',
                     'mao1_push&mao2_win_frequency']
    
    soma_prob = df[probabilidades].sum(axis=1)
    print(f"\nVERIFICAÇÃO SOMA PROBABILIDADES:")
    print(f"  Soma mínima: {soma_prob.min():.6f}")
    print(f"  Soma máxima: {soma_prob.max():.6f}")
    print(f"  Soma média: {soma_prob.mean():.6f}")
    
    if abs(soma_prob.mean() - 1.0) < 0.001:
        print("  ✅ Probabilidades somam 100%")
    else:
        print("  ❌ Erro: Probabilidades não somam 100%")

if __name__ == "__main__":
    main() 