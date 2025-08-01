CC = gcc
CFLAGS = -O3 -march=native -std=c11 -Wall -Wextra -flto -ffast-math -funroll-loops -msse2
LDFLAGS = -lpthread -lm

SOURCES = main.c baralho.c rng.c simulacao.c constantes.c jogo.c saidas.c tabela_estrategia.c split_ev_lookup.c dealer_freq_lookup.c shoe_counter.c ev_calculator.c real_time_ev.c realtime_strategy_integration.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = blackjack_sim

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Tests and examples moved to tests/ directory
# Use: cd tests && make [test_name] to build specific tests

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

teste_validacao_dados: teste_validacao_dados.c dealer_freq_lookup.o split_ev_lookup.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

teste_ev_debug: teste_ev_debug.c dealer_freq_lookup.o split_ev_lookup.o real_time_ev.o shoe_counter.o jogo.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

teste_rank_mapping: teste_rank_mapping.c shoe_counter.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

teste_ev_probabilidade: teste_ev_probabilidade.c shoe_counter.o real_time_ev.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

teste_correcao_ev: teste_correcao_ev.c shoe_counter.o real_time_ev.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

teste_ev_simples: teste_ev_simples.c shoe_counter.o real_time_ev.o jogo.o dealer_freq_lookup.o split_ev_lookup.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

teste_debug_ev: teste_debug_ev.c shoe_counter.o real_time_ev.o jogo.o dealer_freq_lookup.o split_ev_lookup.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

teste_ev_hit: teste_ev_hit.c shoe_counter.o real_time_ev.o jogo.o dealer_freq_lookup.o split_ev_lookup.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

teste_correcao_ev_tempo_real: teste_correcao_ev_tempo_real.c shoe_counter.o real_time_ev.o jogo.o dealer_freq_lookup.o split_ev_lookup.o tabela_estrategia.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

teste_estatistico_ev_tempo_real: teste_estatistico_ev_tempo_real.c shoe_counter.o real_time_ev.o jogo.o dealer_freq_lookup.o split_ev_lookup.o tabela_estrategia.o baralho.o rng.o constantes.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: all clean 