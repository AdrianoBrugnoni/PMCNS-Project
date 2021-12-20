#include <stdio.h>

typedef struct {
    int occupato;           // 0 == letto libero, 1 == letto occupato

    double ingresso;        // tempo nel quale il paziente è entrato nel letto
    double servizio;        // tempo nel quale il paziente lascia il letto

    // output simulazione
    double tempo_occupazione;   // tempo per cui il letto è stato occupato
    double pazienti_serviti;    // numero di pazienti serviti su questo letto
} _letto;

double ottieni_tempo_servizio_letto(int tipo) {
    if(tipo == COVID)
        return exponential(2);
    else // if(tipo == NCOVID)
        return exponential(3);
}

void rilascia_paziente(_letto* l) {

    l->occupato = 0;
    l->ingresso = 0;
    l->servizio = INF;
    l->tempo_occupazione += l->servizio - l->ingresso;
    l->pazienti_serviti += 1;
}

void prepara_letto(_letto* l) {

    l->occupato = 0;
    l->ingresso = 0;
    l->servizio = INF;
    l->tempo_occupazione = 0;
    l->pazienti_serviti = 0;
}

void occupa_letto(_letto* l, double tempo_attuale, int tipo) {

    l->occupato = 1;
    l->ingresso = tempo_attuale;
    l->servizio = tempo_attuale + ottieni_tempo_servizio_letto(tipo);
}