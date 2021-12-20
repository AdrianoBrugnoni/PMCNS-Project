#include <stdio.h>

typedef struct {
    int num_letti;  // letti totali per la terapia intensiva
    _letto* letto;  // array dei letti nella sala
    int bloccato;   // 0 == reparto utilizzabile
                    // 1 == reparto bloccato
} _reparto;


void prepara_letti_reparto(_reparto* r, int letti_nel_reparto) {

    r->num_letti = letti_nel_reparto;
    r->bloccato = 0;

    r->letto = malloc(sizeof(_letto) * r->num_letti);
    for(int i=0; i<r->num_letti; i++)
        prepara_letto(&r->letto[i]);
}

void blocca_reparto() {

}