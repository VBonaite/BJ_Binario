#ifndef BARALHO_H
#define BARALHO_H

#include <stdint.h>
#include <stddef.h>

typedef uint64_t Carta; // Representação de 39 bits

typedef struct {
    Carta *cartas;
    size_t total;
    size_t topo;
} Shoe;

void baralho_criar(Shoe *shoe);
void baralho_embaralhar(Shoe *shoe);
Carta baralho_comprar(Shoe *shoe);
void baralho_destruir(Shoe *shoe);

char carta_para_char(Carta c);

#endif // BARALHO_H 