#include <stdio.h>

typedef struct {
    int num_letti;  // letti totali per la terapia intensiva
    _letto* letto;  // array dei letti nella sala
    int bloccato;   // 0 == reparto utilizzabile
                    // 1 == reparto bloccato
} _reparto;

void copia_reparto(_reparto* r_src, _reparto* r_dst) {
    // asserisco che hanno lo stesso numero di letti

    for(int i=0; i<r_src->num_letti; i++) // per ogni letto del reparto
        copia_letto(&r_src->letto[i], &r_dst->letto[i]); // sorgente -> destinazione
}

void aggiungi_statistiche_reparto(_reparto* r_src, _reparto* r_dst) {
    // asserisco che hanno lo stesso numero di letti

    for(int i=0; i<r_src->num_letti; i++) // per ogni letto del reparto
        aggiungi_statistiche_letto(&r_src->letto[i], &r_dst->letto[i]); // sorgente -> destinazione
}

void prepara_letti_reparto(_reparto* r, int letti_nel_reparto) {

    r->num_letti = letti_nel_reparto;
    r->bloccato = 0;

    r->letto = malloc(sizeof(_letto) * r->num_letti);
    for(int i=0; i<r->num_letti; i++)
        prepara_letto(&r->letto[i]);
}

double ottieni_tempo_liberazione_reparto(_reparto* r) {

    double max_tempo_liberazione_letto = 0;

    for(int i=0; i<r->num_letti; i++) {    // per ogni letto del reparto

        if(r->letto[i].occupato == 1) { // se il letto è occupato
            if(r->letto[i].servizio > max_tempo_liberazione_letto)
                max_tempo_liberazione_letto = r->letto[i].servizio;
        }      
    }

    // se almeno un letto è occupato ritorna il massimo tra i tempi di liberazione dei letti
    // se nessun letto è occupato ritorna 0 
    return max_tempo_liberazione_letto;
}

void blocca_reparto(_reparto* r) {
    r->bloccato = 1;
}

void sblocca_reparto(_reparto* r) {
    r->bloccato = 0;
}