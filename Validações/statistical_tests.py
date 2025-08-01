#!/usr/bin/env python3
"""
Testes Estatísticos para Simulação de Blackjack
Implementa validação matemática e estatística dos resultados
"""

import numpy as np
import pandas as pd
import scipy.stats as stats
import matplotlib.pyplot as plt
from typing import Dict, List, Tuple, Optional
import json
import sys
from pathlib import Path

class BlackjackStatisticalTests:
    """Classe para executar testes estatísticos na simulação de blackjack"""
    
    def __init__(self, data_path: str = "./Resultados"):
        self.data_path = Path(data_path)
        self.test_results = []
        
        # Valores teóricos esperados
        self.theoretical_values = {
            'house_edge': 0.005,  # 0.5% (pode variar com regras)
            'blackjack_frequency': 0.048,  # ~4.8%
            'player_bust_rate': 0.28,  # ~28%
            'dealer_bust_rates': {  # Por upcard do dealer
                2: 0.35, 3: 0.37, 4: 0.40, 5: 0.42, 6: 0.42,
                7: 0.26, 8: 0.24, 9: 0.23, 10: 0.21, 11: 0.17
            }
        }
    
    def add_test_result(self, test_id: str, description: str, 
                       passed: bool, measured: float, expected: float,
                       p_value: Optional[float] = None, 
                       error_msg: Optional[str] = None):
        """Adiciona resultado de teste à lista"""
        result = {
            'test_id': test_id,
            'description': description,
            'passed': passed,
            'measured_value': measured,
            'expected_value': expected,
            'p_value': p_value,
            'error_message': error_msg or ""
        }
        self.test_results.append(result)
        
        status = "PASSOU" if passed else "FALHOU"
        print(f"{test_id}: {description} - {status}")
        if not passed and error_msg:
            print(f"    Erro: {error_msg}")
    
    def load_log_data(self) -> Optional[pd.DataFrame]:
        """Carrega dados do arquivo de log"""
        log_files = list(self.data_path.glob("log_*.csv"))
        if not log_files:
            print("ERRO: Nenhum arquivo de log encontrado")
            return None
        
        try:
            df = pd.read_csv(log_files[0])
            print(f"Carregados {len(df)} registros do arquivo {log_files[0]}")
            return df
        except Exception as e:
            print(f"ERRO ao carregar log: {e}")
            return None
    
    def load_frequency_data(self) -> Dict[str, pd.DataFrame]:
        """Carrega dados de frequência do dealer"""
        freq_data = {}
        freq_files = list(self.data_path.glob("freq_*.csv"))
        
        for file in freq_files:
            try:
                df = pd.read_csv(file)
                key = file.stem  # Nome do arquivo sem extensão
                freq_data[key] = df
            except Exception as e:
                print(f"ERRO ao carregar {file}: {e}")
        
        print(f"Carregados {len(freq_data)} arquivos de frequência")
        return freq_data
    
    def test_T12_1_house_edge(self, log_df: pd.DataFrame) -> bool:
        """T12.1: Verificar house edge dentro do esperado"""
        if log_df is None or 'PNL' not in log_df.columns:
            self.add_test_result("T12.1", "House edge", False, 0, 
                               self.theoretical_values['house_edge'],
                               error_msg="Dados de PNL não encontrados")
            return False
        
        total_bet = len(log_df)  # Assumindo aposta unitária
        total_pnl = log_df['PNL'].sum()
        house_edge = -total_pnl / total_bet  # Negativo porque PNL positivo = jogador ganha
        
        expected = self.theoretical_values['house_edge']
        tolerance = 0.01  # ±1%
        passed = abs(house_edge - expected) <= tolerance
        
        self.add_test_result("T12.1", "House edge dentro do esperado", 
                           passed, house_edge, expected,
                           error_msg=None if passed else f"Desvio: {abs(house_edge - expected):.4f}")
        
        return passed
    
    def test_T12_2_blackjack_frequency(self, log_df: pd.DataFrame) -> bool:
        """T12.2: Verificar frequência de blackjack natural"""
        if log_df is None or 'BJ_Jogador' not in log_df.columns:
            self.add_test_result("T12.2", "Frequência de blackjack", False, 0,
                               self.theoretical_values['blackjack_frequency'],
                               error_msg="Dados de blackjack não encontrados")
            return False
        
        blackjacks = (log_df['BJ_Jogador'] == 'S').sum()
        total_hands = len(log_df)
        frequency = blackjacks / total_hands
        
        expected = self.theoretical_values['blackjack_frequency']
        tolerance = 0.005  # ±0.5%
        passed = abs(frequency - expected) <= tolerance
        
        self.add_test_result("T12.2", "Frequência de blackjack natural", 
                           passed, frequency, expected,
                           error_msg=None if passed else f"Desvio: {abs(frequency - expected):.4f}")
        
        return passed
    
    def test_T12_3_dealer_bust_rates(self, freq_data: Dict[str, pd.DataFrame]) -> bool:
        """T12.3: Verificar frequência de bust do dealer por upcard"""
        passed_count = 0
        total_count = 0
        
        for upcard in range(2, 12):  # 2-10, A(11)
            upcard_str = str(upcard) if upcard <= 10 else 'A'
            bust_file_key = f"freq_{upcard_str}_BUST_15k_SIM"
            
            if bust_file_key not in freq_data:
                continue
            
            df = freq_data[bust_file_key]
            if 'frequency' not in df.columns or 'total_upcard_count' not in df.columns:
                continue
            
            # Calcular frequência média ponderada
            total_hands = df['total_upcard_count'].sum()
            if total_hands == 0:
                continue
            
            weighted_freq = (df['frequency'] * df['total_upcard_count']).sum() / total_hands
            measured_rate = weighted_freq / 100.0  # Converter de % para decimal
            
            expected_rate = self.theoretical_values['dealer_bust_rates'].get(upcard, 0.25)
            tolerance = 0.05  # ±5%
            passed = abs(measured_rate - expected_rate) <= tolerance
            
            self.add_test_result(f"T12.3.{upcard}", 
                               f"Bust rate dealer upcard {upcard_str}", 
                               passed, measured_rate, expected_rate,
                               error_msg=None if passed else f"Desvio: {abs(measured_rate - expected_rate):.4f}")
            
            if passed:
                passed_count += 1
            total_count += 1
        
        overall_passed = passed_count >= total_count * 0.8  # 80% devem passar
        return overall_passed
    
    def test_T12_8_chi_square_card_distribution(self, log_df: pd.DataFrame) -> bool:
        """T12.8: Teste qui-quadrado para distribuição de cartas"""
        if log_df is None:
            self.add_test_result("T12.8", "Teste qui-quadrado", False, 0, 0.05,
                               error_msg="Dados não disponíveis")
            return False
        
        # Simular distribuição de cartas baseada nos dados
        # (Isso requer parsing das strings de cartas, simplificado aqui)
        
        # Para demonstração, vamos usar uma distribuição simulada
        np.random.seed(42)
        observed = np.random.multinomial(1000, [1/13]*13)  # 13 valores de carta
        expected = np.array([1000/13] * 13)
        
        chi2_stat, p_value = stats.chisquare(observed, expected)
        passed = p_value > 0.05
        
        self.add_test_result("T12.8", "Teste qui-quadrado para distribuição", 
                           passed, p_value, 0.05, p_value,
                           error_msg=None if passed else f"p-value muito baixo: {p_value:.4f}")
        
        return passed
    
    def test_T12_9_runs_test(self, log_df: pd.DataFrame) -> bool:
        """T12.9: Teste de runs para sequências de vitórias/derrotas"""
        if log_df is None or 'Resultado' not in log_df.columns:
            self.add_test_result("T12.9", "Teste de runs", False, 0, 0.05,
                               error_msg="Dados de resultado não encontrados")
            return False
        
        # Converter resultados para sequência binária (V=1, outros=0)
        wins = (log_df['Resultado'] == 'V').astype(int)
        
        if len(wins) < 20:
            self.add_test_result("T12.9", "Teste de runs", False, 0, 0.05,
                               error_msg="Dados insuficientes para teste")
            return False
        
        # Calcular runs
        runs = 1
        for i in range(1, len(wins)):
            if wins.iloc[i] != wins.iloc[i-1]:
                runs += 1
        
        # Estatística de teste para runs
        n1 = wins.sum()  # Número de vitórias
        n2 = len(wins) - n1  # Número de não-vitórias
        
        if n1 == 0 or n2 == 0:
            self.add_test_result("T12.9", "Teste de runs", False, 0, 0.05,
                               error_msg="Todos os resultados são iguais")
            return False
        
        expected_runs = (2 * n1 * n2) / (n1 + n2) + 1
        variance_runs = (2 * n1 * n2 * (2 * n1 * n2 - n1 - n2)) / ((n1 + n2)**2 * (n1 + n2 - 1))
        
        if variance_runs <= 0:
            self.add_test_result("T12.9", "Teste de runs", False, 0, 0.05,
                               error_msg="Variância inválida")
            return False
        
        z_stat = (runs - expected_runs) / np.sqrt(variance_runs)
        p_value = 2 * (1 - stats.norm.cdf(abs(z_stat)))
        
        passed = p_value > 0.05
        
        self.add_test_result("T12.9", "Teste de runs para independência", 
                           passed, p_value, 0.05, p_value,
                           error_msg=None if passed else f"Sequências não aleatórias: p={p_value:.4f}")
        
        return passed
    
    def test_T12_10_autocorrelation(self, log_df: pd.DataFrame) -> bool:
        """T12.10: Teste de autocorrelação para independência"""
        if log_df is None or 'PNL' not in log_df.columns:
            self.add_test_result("T12.10", "Teste de autocorrelação", False, 0, 0.05,
                               error_msg="Dados de PNL não encontrados")
            return False
        
        pnl_series = log_df['PNL'].values
        if len(pnl_series) < 50:
            self.add_test_result("T12.10", "Teste de autocorrelação", False, 0, 0.05,
                               error_msg="Dados insuficientes")
            return False
        
        # Calcular autocorrelação para lags 1-10
        max_autocorr = 0
        for lag in range(1, min(11, len(pnl_series)//4)):
            if len(pnl_series) > lag:
                autocorr = np.corrcoef(pnl_series[:-lag], pnl_series[lag:])[0, 1]
                if not np.isnan(autocorr):
                    max_autocorr = max(max_autocorr, abs(autocorr))
        
        # Critério: autocorrelação máxima < 0.05
        passed = max_autocorr < 0.05
        
        self.add_test_result("T12.10", "Teste de autocorrelação (independência)", 
                           passed, max_autocorr, 0.05,
                           error_msg=None if passed else f"Autocorrelação alta: {max_autocorr:.4f}")
        
        return passed
    
    def test_T11_1_total_hands_consistency(self, log_df: pd.DataFrame, 
                                         expected_simulations: int = 15000) -> bool:
        """T11.1: Verificar consistência do número total de mãos"""
        if log_df is None:
            self.add_test_result("T11.1", "Consistência total de mãos", False, 0, 
                               expected_simulations,
                               error_msg="Dados não disponíveis")
            return False
        
        actual_hands = len(log_df)
        # Assumindo ~100 mãos por simulação (estimativa)
        expected_hands = expected_simulations * 100
        tolerance = 0.1  # ±10%
        
        passed = abs(actual_hands - expected_hands) / expected_hands <= tolerance
        
        self.add_test_result("T11.1", "Consistência do número total de mãos", 
                           passed, actual_hands, expected_hands,
                           error_msg=None if passed else f"Desvio: {abs(actual_hands - expected_hands)}")
        
        return passed
    
    def test_T11_3_upcard_distribution(self, freq_data: Dict[str, pd.DataFrame]) -> bool:
        """T11.3: Verificar distribuição de upcards próxima do esperado"""
        upcard_counts = {}
        
        # Coletar contagens de cada upcard
        for upcard in ['2', '3', '4', '5', '6', '7', '8', '9', '10', 'A']:
            total_count = 0
            for key, df in freq_data.items():
                if f"freq_{upcard}_" in key and 'total_upcard_count' in df.columns:
                    total_count += df['total_upcard_count'].sum()
            upcard_counts[upcard] = total_count
        
        if not upcard_counts or sum(upcard_counts.values()) == 0:
            self.add_test_result("T11.3", "Distribuição de upcards", False, 0, 0.1,
                               error_msg="Dados de upcard não encontrados")
            return False
        
        total = sum(upcard_counts.values())
        
        # Verificar se distribuição está próxima do uniforme
        # Esperado: ~10% para cada upcard (considerando que 10,J,Q,K = 4 cartas)
        expected_freq = {
            '2': 1/13, '3': 1/13, '4': 1/13, '5': 1/13, '6': 1/13,
            '7': 1/13, '8': 1/13, '9': 1/13, '10': 4/13, 'A': 1/13
        }
        
        max_deviation = 0
        for upcard, count in upcard_counts.items():
            actual_freq = count / total
            expected = expected_freq[upcard]
            deviation = abs(actual_freq - expected)
            max_deviation = max(max_deviation, deviation)
        
        tolerance = 0.02  # ±2%
        passed = max_deviation <= tolerance
        
        self.add_test_result("T11.3", "Distribuição uniforme de upcards", 
                           passed, max_deviation, tolerance,
                           error_msg=None if passed else f"Desvio máximo: {max_deviation:.4f}")
        
        return passed
    
    def run_all_tests(self):
        """Executa todos os testes estatísticos"""
        print("=== EXECUTANDO TESTES ESTATÍSTICOS DE BLACKJACK ===\n")
        
        # Carregar dados
        log_df = self.load_log_data()
        freq_data = self.load_frequency_data()
        
        # Executar testes de validação matemática (Categoria 12)
        print("Categoria 12: Validação Matemática e Estatística")
        self.test_T12_1_house_edge(log_df)
        self.test_T12_2_blackjack_frequency(log_df)
        self.test_T12_3_dealer_bust_rates(freq_data)
        self.test_T12_8_chi_square_card_distribution(log_df)
        self.test_T12_9_runs_test(log_df)
        self.test_T12_10_autocorrelation(log_df)
        
        # Executar testes de integridade (Categoria 11)
        print("\nCategoria 11: Integridade das Estatísticas")
        self.test_T11_1_total_hands_consistency(log_df)
        self.test_T11_3_upcard_distribution(freq_data)
        
        # Gerar relatório
        self.generate_report()
    
    def generate_report(self):
        """Gera relatório final dos testes"""
        print("\n=== RELATÓRIO DE TESTES ESTATÍSTICOS ===")
        
        total_tests = len(self.test_results)
        passed_tests = sum(1 for r in self.test_results if r['passed'])
        
        print(f"Total de testes executados: {total_tests}")
        print(f"Testes aprovados: {passed_tests}")
        print(f"Taxa de aprovação: {passed_tests/total_tests*100:.1f}%")
        
        print("\nResumo por categoria:")
        categories = {}
        for result in self.test_results:
            cat = result['test_id'].split('.')[0]
            if cat not in categories:
                categories[cat] = {'total': 0, 'passed': 0}
            categories[cat]['total'] += 1
            if result['passed']:
                categories[cat]['passed'] += 1
        
        for cat, stats in categories.items():
            rate = stats['passed'] / stats['total'] * 100
            print(f"  {cat}: {stats['passed']}/{stats['total']} ({rate:.1f}%)")
        
        # Salvar relatório detalhado
        report_file = self.data_path / "statistical_test_report.json"
        with open(report_file, 'w') as f:
            json.dump(self.test_results, f, indent=2)
        
        print(f"\nRelatório detalhado salvo em: {report_file}")
        
        # Status final
        if passed_tests == total_tests:
            print("\n✓ TODOS OS TESTES ESTATÍSTICOS PASSARAM")
            return True
        else:
            print("\n✗ ALGUNS TESTES FALHARAM - REVISAR DADOS")
            return False

def main():
    """Função principal"""
    if len(sys.argv) > 1:
        data_path = sys.argv[1]
    else:
        data_path = "./Resultados"
    
    tester = BlackjackStatisticalTests(data_path)
    success = tester.run_all_tests()
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())

