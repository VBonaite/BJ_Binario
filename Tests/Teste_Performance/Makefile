CC = gcc
CFLAGS = -O3 -march=native -std=c11 -Wall -Wextra -flto -ffast-math -funroll-loops -I..
LDFLAGS = -lpthread -lm

# Objetos do diretório principal necessários para os testes
MAIN_OBJECTS = ../baralho.o ../rng.o ../simulacao.o ../constantes.o ../jogo.o ../saidas.o ../tabela_estrategia.o

# Objetos locais
LOCAL_OBJECTS = otimizacoes.o simulacao_otimizada.o

# Targets
all: teste_completo teste_insurance teste_performance dealer_blackjack_analyzer $(LOCAL_OBJECTS)

# Regra para teste completo
teste_completo: teste_completo.c $(MAIN_OBJECTS)
	$(CC) $(CFLAGS) -o teste_completo teste_completo.c $(MAIN_OBJECTS) $(LDFLAGS)

# Regra para analisador de blackjack do dealer
dealer_blackjack_analyzer: dealer_blackjack_analyzer.c $(MAIN_OBJECTS)
	$(CC) $(CFLAGS) -o dealer_blackjack_analyzer dealer_blackjack_analyzer.c $(MAIN_OBJECTS) $(LDFLAGS)

# Regra para teste de insurance
teste_insurance: teste_insurance.c $(MAIN_OBJECTS)
	$(CC) $(CFLAGS) -o teste_insurance teste_insurance.c $(MAIN_OBJECTS) $(LDFLAGS)

# Regra para teste de performance (precisa de otimizacoes)
teste_performance: teste_performance.c $(MAIN_OBJECTS) otimizacoes.o
	$(CC) $(CFLAGS) -o teste_performance teste_performance.c $(MAIN_OBJECTS) otimizacoes.o $(LDFLAGS)

# Compilar objetos locais
otimizacoes.o: otimizacoes.c
	$(CC) $(CFLAGS) -c otimizacoes.c -o otimizacoes.o

simulacao_otimizada.o: simulacao_otimizada.c
	$(CC) $(CFLAGS) -c simulacao_otimizada.c -o simulacao_otimizada.o

# Regra para executar o teste completo
test: teste_completo
	./teste_completo

# Regra para executar o teste de insurance
test_insurance: teste_insurance
	./teste_insurance

# Regra para executar o teste de performance
test_performance: teste_performance
	./teste_performance

clean:
	rm -f teste_completo dealer_blackjack_analyzer teste_insurance teste_performance $(LOCAL_OBJECTS)

.PHONY: all clean test test_insurance test_performance 