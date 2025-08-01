# MAKEFILE PARA TESTES E EXEMPLOS
# Este Makefile compila os testes e exemplos dos sistemas de lookup e shoe counter

CC = gcc
CFLAGS = -O3 -march=native -std=c11 -Wall -Wextra -flto -ffast-math -funroll-loops -msse2
LDFLAGS = -lpthread -lm

# Diretório principal para incluir dependências
MAIN_DIR = ..

# Objetos necessários do diretório principal
MAIN_OBJECTS = $(MAIN_DIR)/baralho.o $(MAIN_DIR)/constantes.o $(MAIN_DIR)/rng.o
LOOKUP_OBJECTS = $(MAIN_DIR)/split_ev_lookup.o $(MAIN_DIR)/dealer_freq_lookup.o  
SHOE_COUNTER_OBJECTS = $(MAIN_DIR)/shoe_counter.o
EV_CALCULATOR_OBJECTS = $(MAIN_DIR)/ev_calculator.o

# Executáveis de teste
TEST_SHOE_COUNTER = test_shoe_counter
TEST_LOOKUP_TABLES = test_lookup_tables
EXEMPLO_USO_LOOKUP = exemplo_uso_lookup
EXEMPLO_INTEGRACAO_SHOE = exemplo_integracao_shoe_counter
EXEMPLO_CALCULO_EV = exemplo_calculo_ev

# Targets principais
all: $(TEST_SHOE_COUNTER) $(TEST_LOOKUP_TABLES) $(EXEMPLO_USO_LOOKUP) $(EXEMPLO_INTEGRACAO_SHOE) $(EXEMPLO_CALCULO_EV)

# Regra para garantir que objetos principais estão compilados
$(MAIN_OBJECTS) $(LOOKUP_OBJECTS) $(SHOE_COUNTER_OBJECTS) $(EV_CALCULATOR_OBJECTS):
	@echo "Compilando dependências principais..."
	@cd $(MAIN_DIR) && $(MAKE) -s $(notdir $@)

# Teste do shoe counter
$(TEST_SHOE_COUNTER): test_shoe_counter.o $(SHOE_COUNTER_OBJECTS) $(MAIN_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Teste das lookup tables  
$(TEST_LOOKUP_TABLES): test_lookup_tables.o $(LOOKUP_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Exemplo de uso das lookup tables
$(EXEMPLO_USO_LOOKUP): exemplo_uso_lookup.o $(LOOKUP_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Exemplo de integração do shoe counter
$(EXEMPLO_INTEGRACAO_SHOE): exemplo_integracao_shoe_counter.o $(SHOE_COUNTER_OBJECTS) $(MAIN_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Exemplo de cálculo de Expected Value
$(EXEMPLO_CALCULO_EV): exemplo_calculo_ev.o $(EV_CALCULATOR_OBJECTS) $(SHOE_COUNTER_OBJECTS) $(LOOKUP_OBJECTS) $(MAIN_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilação de objetos locais (inclui headers do diretório principal)
%.o: %.c
	$(CC) $(CFLAGS) -I$(MAIN_DIR) -c $< -o $@

# Targets individuais para facilitar uso
test_shoe: $(TEST_SHOE_COUNTER)
	@echo "✅ Teste do shoe counter compilado: ./$(TEST_SHOE_COUNTER)"

test_lookup: $(TEST_LOOKUP_TABLES)  
	@echo "✅ Teste das lookup tables compilado: ./$(TEST_LOOKUP_TABLES)"

exemplo_lookup: $(EXEMPLO_USO_LOOKUP)
	@echo "✅ Exemplo das lookup tables compilado: ./$(EXEMPLO_USO_LOOKUP)"

exemplo_shoe: $(EXEMPLO_INTEGRACAO_SHOE)
	@echo "✅ Exemplo do shoe counter compilado: ./$(EXEMPLO_INTEGRACAO_SHOE)"

exemplo_ev: $(EXEMPLO_CALCULO_EV)
	@echo "✅ Exemplo de cálculo de EV compilado: ./$(EXEMPLO_CALCULO_EV)"

# Executar todos os testes
run_tests: all
	@echo "🧪 Executando teste do shoe counter..."
	@./$(TEST_SHOE_COUNTER)
	@echo "\n🧪 Executando teste das lookup tables..."
	@./$(TEST_LOOKUP_TABLES)

# Executar todos os exemplos
run_examples: all
	@echo "📚 Executando exemplo das lookup tables..."
	@./$(EXEMPLO_USO_LOOKUP)
	@echo "\n📚 Executando exemplo do shoe counter..."
	@./$(EXEMPLO_INTEGRACAO_SHOE)

# Limpeza
clean:
	rm -f *.o $(TEST_SHOE_COUNTER) $(TEST_LOOKUP_TABLES) $(EXEMPLO_USO_LOOKUP) $(EXEMPLO_INTEGRACAO_SHOE) $(EXEMPLO_CALCULO_EV)

# Limpeza completa (inclui objetos principais)
clean_all: clean
	@cd $(MAIN_DIR) && $(MAKE) clean

# Ajuda
help:
	@echo "MAKEFILE DE TESTES E EXEMPLOS"
	@echo "============================="
	@echo ""
	@echo "Targets disponíveis:"
	@echo "  all              - Compila todos os testes e exemplos"
	@echo "  test_shoe        - Compila teste do shoe counter"
	@echo "  test_lookup      - Compila teste das lookup tables"
	@echo "  exemplo_lookup   - Compila exemplo das lookup tables"
	@echo "  exemplo_shoe     - Compila exemplo do shoe counter"
	@echo "  exemplo_ev       - Compila exemplo de cálculo de EV"
	@echo "  run_tests        - Executa todos os testes"
	@echo "  run_examples     - Executa todos os exemplos"
	@echo "  clean            - Remove arquivos compilados"
	@echo "  clean_all        - Remove arquivos locais e principais"
	@echo "  help             - Mostra esta ajuda"
	@echo ""
	@echo "Exemplos de uso:"
	@echo "  make test_shoe && ./test_shoe_counter"
	@echo "  make run_tests"
	@echo "  make run_examples"

.PHONY: all test_shoe test_lookup exemplo_lookup exemplo_shoe exemplo_ev run_tests run_examples clean clean_all help 