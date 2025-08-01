CC = gcc
CFLAGS = -O3 -march=native -std=c11 -Wall -Wextra -flto -ffast-math -funroll-loops -msse2
LDFLAGS = -lpthread -lm

SOURCES = main.c baralho.c rng.c simulacao.c constantes.c jogo.c saidas.c tabela_estrategia.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = blackjack_sim

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean 