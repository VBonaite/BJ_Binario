#include "tabela_estrategia.h"
#include <stddef.h>

#define H  ACAO_HIT
#define S  ACAO_STAND
#define DH ACAO_DOUBLE_OR_HIT
#define DS ACAO_DOUBLE_OR_STAND
#define PH ACAO_SPLIT_OR_HIT
#define PS ACAO_SPLIT_OR_STAND
#define D  ACAO_DOUBLE
#define P  ACAO_SPLIT

// Hard totals 3-21 (index 0->3)
static const AcaoEstrategia hard_table[19][10] = {
    /*3*/ { H,H,H,H,H,H,H,H,H,H },
    /*4*/ { H,H,H,H,H,H,H,H,H,H },
    /*5*/ { H,H,H,H,H,H,H,H,H,H },
    /*6*/ { H,H,H,H,H,H,H,H,H,H },
    /*7*/ { H,H,H,H,H,H,H,H,H,H },
    /*8*/ { H,H,H,H,H,H,H,H,H,H },
    /*9*/ { H,DH,DH,DH,DH,H,H,H,H,H },
    /*10*/{ DH,DH,DH,DH,DH,DH,DH,DH,H,H },
    /*11*/{ DH,DH,DH,DH,DH,DH,DH,DH,DH,H },
    /*12*/{ H,H,S,S,S,H,H,H,H,H },
    /*13*/{ S,S,S,S,S,H,H,H,H,H },
    /*14*/{ S,S,S,S,S,H,H,H,H,H },
    /*15*/{ S,S,S,S,S,H,H,H,H,H },
    /*16*/{ S,S,S,S,S,H,H,H,H,H },
    /*17*/{ S,S,S,S,S,S,S,S,S,S },
    /*18*/{ S,S,S,S,S,S,S,S,S,S },
    /*19*/{ S,S,S,S,S,S,S,S,S,S },
    /*20*/{ S,S,S,S,S,S,S,S,S,S },
    /*21*/{ S,S,S,S,S,S,S,S,S,S }
};

// Soft totals 13-20 (A2-A9)
static const AcaoEstrategia soft_table[8][10] = {
    /*13*/{ H,H,H,DH,DH,H,H,H,H,H }, // A2
    /*14*/{ H,H,H,DH,DH,H,H,H,H,H }, // A3
    /*15*/{ H,H,DH,DH,DH,H,H,H,H,H }, // A4
    /*16*/{ H,H,DH,DH,DH,H,H,H,H,H }, // A5
    /*17*/{ H,DH,DH,DH,DH,H,H,H,H,H }, // A6
    /*18*/{ S,DS,DS,DS,DS,S,S,H,H,H }, // A7
    /*19*/{ S,S,S,S,DS,S,S,S,S,S },    // A8
    /*20*/{ S,S,S,S,S,S,S,S,S,S }      // A9
};

// Pairs 2-A
static const AcaoEstrategia pair_table[10][10] = {
    /*2,2*/{ H,H,PH,PH,PH,PH,H,H,H,H },
    /*3,3*/{ H,H,PH,PH,PH,PH,H,H,H,H },
    /*4,4*/{ H,H,H,H,H,H,H,H,H,H },
    /*5,5*/{ DH,DH,DH,DH,DH,DH,DH,DH,H,H },
    /*6,6*/{ H,PH,PH,PH,PH,H,H,H,H,H },
    /*7,7*/{ PS,PS,PS,PS,PS,PH,H,H,H,H },
    /*8,8*/{ PS,PS,PS,PS,PS,PH,PH,PH,PH,PH },
    /*9,9*/{ PS,PS,PS,PS,PS,S,PS,PS,S,S },
    /*10,10*/{ S,S,S,S,S,S,S,S,S,S },
    /*A,A*/{ PS,PS,PS,PS,PS,PS,PS,PS,PS,PS }
};

AcaoEstrategia estrategia_hard(int total, int upcard) {
    if (total < 3 || total > 21 || upcard < 2 || upcard > 11) return ACAO_HIT;
    return hard_table[total - 3][upcard - 2];
}

AcaoEstrategia estrategia_soft(int total, int upcard) {
    if (total < 13 || total > 20 || upcard < 2 || upcard > 11) return ACAO_HIT;
    return soft_table[total - 13][upcard - 2];
}

AcaoEstrategia estrategia_par(int par_rank, int upcard) {
    if (par_rank < 2 || par_rank > 11 || upcard < 2 || upcard > 11) return ACAO_HIT;
    return pair_table[par_rank - 2][upcard - 2];
} 