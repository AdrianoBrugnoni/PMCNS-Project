#include <stdio.h>
#include <string.h>

typedef struct {
    
    int letti_per_reparto;      // numero di letti per reparto di terapia intensiva

    _coda_pr* coda;             // array di code con priorità per la terapia intensiva
                                // coda[COVID] e coda[NCOVID]

    int* num_min_reparti;       // numero minimo di reparti per covid e non covid

    int* num_reparti; 
    _reparto** reparto;         // array di array di reparti
                                // reparto[COVID][i] per i da 0 num_reparti[COVID]
                                // reparto[NCOVID][j] per j da 0 num_reparti[NCOVID]

} _ospedale;

void ottieni_prototipo_ospedale_1(_ospedale* o) {

    // inizializza code con priorità

    o->coda = malloc(sizeof(_coda_pr) * 2);

    inizializza_coda_pr(&o->coda[COVID], 3, COVID);
    inizializza_coda_pr(&o->coda[NCOVID], 2, NCOVID);

    // inizializza reparti

    o->num_reparti = malloc(sizeof(int) * 2);
    o->letti_per_reparto = 12;
    o->num_reparti[COVID] = 1;
    o->num_reparti[NCOVID] = 8;

    o->num_min_reparti = malloc(sizeof(int) * 2);
    o->num_min_reparti[COVID] = 1;
    o->num_min_reparti[NCOVID] = 4;

    o->reparto = malloc(sizeof(_reparto*) * 2);

    o->reparto[COVID] = malloc(sizeof(_reparto) * o->num_reparti[COVID]);
    for(int i=0; i<o->num_reparti[COVID]; i++)
        prepara_letti_reparto(&o->reparto[COVID][i], o->letti_per_reparto);

    o->reparto[NCOVID] = malloc(sizeof(_reparto) * o->num_reparti[NCOVID]);
    for(int i=0; i<o->num_reparti[NCOVID]; i++)
        prepara_letti_reparto(&o->reparto[NCOVID][i], o->letti_per_reparto);
}

void prova_muovi_paziente_in_letto(_ospedale* o, double tempo_attuale, int tipo) {

    // cerca un letto libero nel reparto COVID o NCOVID (usa parametro "tipo")
    /*
        per i da 0 a o->num_reparti[tipo]

            controlla se o->reparto[tipo][i].bloccato == 0 
            se si, procedi
            se no, rifai il ciclo con i++
        
            per j da 0 a o->reparto[tipo][i]->num_letti
                controlla se o->reparto[tipo][i].letto[j].occupato == 0
                se si, esci dal ciclo e salva (i,j)
                se no, continua
        
        Se ho trovato un letto con coordinate (i,j) libero vado a rimuovere il primo paziente dalla coda
        Infine, occupo il letto se vi era almeno un paziente in coda

            int ret = rimuovi_primo_paziente(&o->coda[tipo], tempo_attuale);
            
            // non mi basta sapere che c'è un letto libero, ma con "ret" vado anche
            // a vedere se c'era un paziente in coda da spostare
            if(ret == 0)    // se c'è stata la rimozione dalla coda
                occupa_letto(&o->reparto[tipo][i].letto[j], tempo_attuale, COVID)

        Se non c'era un letto libero allora esco
    */
}

void prova_transizione_reparto(_ospedale* o, int id_reparto, int tipo) {

    // se il reparto su cui è avvenuto il completamento era bloccato allora si procede
    // altrimenti si esce

    // if(ospedale[ne->id_ospedale].reparto_covid[ne->id_reparto].bloccato == 0)
    //      return

    // fai un ciclo e controlla se il reparto è vuoto
    // reparto[tipo][id_reparto] è il reparto in cui è avvenuto un completamento
    /*

        controlla se o->reparto[tipo][id_reparto].letto[i].occupato == 0 per ogni i da 0 a [...].num_letti
        
        se vero, allora rialloca le vari strutture dati preservando le 
        loro informazioni e cambiandone la dimensione

        se tipo == COVID allora il reparto deve diventare non covid
        se tipo == NCOVID allora il reparto deve diventare covid

        // tieni in mente che non si possono avere 0 reparti covid o 0 reparti non covid
        // vi è un valore di soglia minima per ciascuno di questi
    */
}