#!/usr/bin/env python3
"""
Script para fazer backup dos arquivos de splits antes da correção do EV.
"""

import os
import shutil
import glob
from datetime import datetime

def fazer_backup():
    """Cria backup dos arquivos de splits"""
    
    # Diretórios
    splits_dir = "/mnt/dados/BJ_Binario/Resultados/splits/"
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = f"/mnt/dados/BJ_Binario/Resultados/splits_backup_{timestamp}/"
    
    print(f"🔄 Criando backup em: {backup_dir}")
    
    # Criar diretório de backup
    os.makedirs(backup_dir, exist_ok=True)
    
    # Buscar todos os arquivos CSV
    pattern = os.path.join(splits_dir, "split_outcome_*.csv")
    arquivos = glob.glob(pattern)
    
    if not arquivos:
        print("❌ Nenhum arquivo encontrado para backup!")
        return False
    
    # Copiar arquivos
    for arquivo in arquivos:
        nome_arquivo = os.path.basename(arquivo)
        destino = os.path.join(backup_dir, nome_arquivo)
        shutil.copy2(arquivo, destino)
    
    print(f"✅ Backup concluído! {len(arquivos)} arquivos copiados")
    print(f"📁 Localização: {backup_dir}")
    
    return True

if __name__ == "__main__":
    fazer_backup() 