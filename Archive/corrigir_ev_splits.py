#!/usr/bin/env python3
"""
Script para corrigir os valores de Expected Value nos arquivos CSV de splits já gerados.

Fórmula correta do EV:
EV = -2*P(lose/lose) + 2*P(win/win) + 0*P(push/push) + 0*P(lose/win) 
     + (-1)*P(lose/push) + 0*P(win/lose) + 1*P(win/push) + (-1)*P(push/lose) + 1*P(push/win)

Ou seja:
EV = -2*P(1) + 2*P(2) - P(5) + P(7) - P(8) + P(9)

onde:
P(1) = mao1_lose&mao2_lose_frequency
P(2) = mao1_win&mao2_win_frequency  
P(5) = mao1_lose&mao2_push_frequency
P(7) = mao1_win&mao2_push_frequency
P(8) = mao1_push&mao2_lose_frequency
P(9) = mao1_push&mao2_win_frequency
"""

import os
import pandas as pd
import glob
from pathlib import Path

def corrigir_ev_arquivo(arquivo_path):
    """Corrige o EV de um arquivo CSV específico"""
    print(f"Processando: {arquivo_path}")
    
    try:
        # Ler CSV
        df = pd.read_csv(arquivo_path)
        
        # Verificar se tem as colunas necessárias
        colunas_necessarias = [
            'mao1_lose&mao2_lose_frequency',
            'mao1_win&mao2_win_frequency', 
            'mao1_lose&mao2_push_frequency',
            'mao1_win&mao2_push_frequency',
            'mao1_push&mao2_lose_frequency',
            'mao1_push&mao2_win_frequency',
            'expected_value'
        ]
        
        for coluna in colunas_necessarias:
            if coluna not in df.columns:
                print(f"  ❌ ERRO: Coluna '{coluna}' não encontrada!")
                return False
        
        # Calcular EV corrigido
        p1 = df['mao1_lose&mao2_lose_frequency']
        p2 = df['mao1_win&mao2_win_frequency']
        p5 = df['mao1_lose&mao2_push_frequency']
        p7 = df['mao1_win&mao2_push_frequency']
        p8 = df['mao1_push&mao2_lose_frequency']
        p9 = df['mao1_push&mao2_win_frequency']
        
        # Fórmula correta: EV = -2*P(1) + 2*P(2) - P(5) + P(7) - P(8) + P(9)
        ev_corrigido = -2.0 * p1 + 2.0 * p2 - p5 + p7 - p8 + p9
        
        # Verificar diferenças
        ev_original = df['expected_value']
        diferenca_media = abs(ev_corrigido - ev_original).mean()
        diferenca_maxima = abs(ev_corrigido - ev_original).max()
        
        print(f"  📊 Diferença média: {diferenca_media:.6f}")
        print(f"  📊 Diferença máxima: {diferenca_maxima:.6f}")
        
        # Atualizar coluna de EV
        df['expected_value'] = ev_corrigido
        
        # Salvar arquivo corrigido
        df.to_csv(arquivo_path, index=False, float_format='%.6f')
        print(f"  ✅ Arquivo corrigido e salvo!")
        
        return True
        
    except Exception as e:
        print(f"  ❌ ERRO ao processar {arquivo_path}: {e}")
        return False

def main():
    """Função principal"""
    print("🔧 CORREÇÃO DE EXPECTED VALUE - ARQUIVOS DE SPLITS")
    print("=" * 60)
    
    # Diretório dos arquivos de splits
    splits_dir = "/mnt/dados/BJ_Binario/Resultados/splits/"
    
    # Verificar se diretório existe
    if not os.path.exists(splits_dir):
        print(f"❌ ERRO: Diretório {splits_dir} não encontrado!")
        return
    
    # Buscar todos os arquivos CSV de splits
    pattern = os.path.join(splits_dir, "split_outcome_*.csv")
    arquivos = glob.glob(pattern)
    
    if not arquivos:
        print(f"❌ ERRO: Nenhum arquivo CSV encontrado em {splits_dir}")
        return
    
    print(f"📁 Encontrados {len(arquivos)} arquivos para corrigir")
    print()
    
    # Processar cada arquivo
    sucessos = 0
    falhas = 0
    
    for arquivo in sorted(arquivos):
        if corrigir_ev_arquivo(arquivo):
            sucessos += 1
        else:
            falhas += 1
        print()
    
    # Relatório final
    print("=" * 60)
    print("📋 RELATÓRIO FINAL:")
    print(f"  ✅ Arquivos corrigidos: {sucessos}")
    print(f"  ❌ Falhas: {falhas}")
    print(f"  📊 Total processado: {len(arquivos)}")
    
    if falhas == 0:
        print("\n🎉 SUCESSO! Todos os arquivos foram corrigidos!")
    else:
        print(f"\n⚠️  ATENÇÃO: {falhas} arquivo(s) tiveram problemas!")

def verificar_antes_depois():
    """Função para verificar um arquivo específico antes e depois da correção"""
    arquivo_teste = "/mnt/dados/BJ_Binario/Resultados/splits/split_outcome_AA_vs_A_3M.csv"
    
    if not os.path.exists(arquivo_teste):
        print(f"Arquivo de teste não encontrado: {arquivo_teste}")
        return
    
    print("🔍 VERIFICAÇÃO - ANTES vs DEPOIS (primeiras 5 linhas)")
    print("=" * 80)
    
    df = pd.read_csv(arquivo_teste)
    
    # Mostrar algumas linhas originais
    print("DADOS ORIGINAIS:")
    colunas_importantes = ['true_count_center', 'mao1_lose&mao2_lose_frequency', 
                          'mao1_win&mao2_win_frequency', 'expected_value']
    print(df[colunas_importantes].head())
    
    print("\nCALCULO CORRETO:")
    # Calcular EV correto
    p1 = df['mao1_lose&mao2_lose_frequency']
    p2 = df['mao1_win&mao2_win_frequency']
    p5 = df['mao1_lose&mao2_push_frequency']
    p7 = df['mao1_win&mao2_push_frequency']
    p8 = df['mao1_push&mao2_lose_frequency']
    p9 = df['mao1_push&mao2_win_frequency']
    
    ev_correto = -2.0 * p1 + 2.0 * p2 - p5 + p7 - p8 + p9
    
    # Mostrar comparação
    comparacao = df[['true_count_center']].copy()
    comparacao['EV_original'] = df['expected_value']
    comparacao['EV_correto'] = ev_correto
    comparacao['diferenca'] = abs(comparacao['EV_correto'] - comparacao['EV_original'])
    
    print(comparacao.head())
    print(f"\nDiferença média: {comparacao['diferenca'].mean():.6f}")
    print(f"Diferença máxima: {comparacao['diferenca'].max():.6f}")

if __name__ == "__main__":
    # Mostrar verificação primeiro
    verificar_antes_depois()
    print("\n" + "="*80 + "\n")
    
    # Executar correção
    main() 