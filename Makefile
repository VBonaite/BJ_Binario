CC = gcc
CFLAGS = -O3 -march=native -std=c11 -Wall -Wextra

OBJ = main.o baralho.o rng.o simulacao.o constantes.o jogo.o saidas.o tabela_estrategia.o

all: blackjack_sim

blackjack_sim: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) blackjack_sim 