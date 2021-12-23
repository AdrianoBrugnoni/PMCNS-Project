#include <stdio.h>
#include <string.h>

typedef struct {
    
    int letti_per_reparto;      // numero di letti per reparto di terapia intensiva (uguale per ogni reparto)
    double soglia_aumento;      // valore di soglia per ampliamento reparto covid
    double soglia_riduzione;    // valore di soglia per riduzione reparto covid
    int ampliamento_in_corso;   // 0 = non c'è ampliamento del reparto covid in corso, 1 = viceversa 
    int riduzione_in_corso;     // 0 = non c'è ampliamento del reparto non covid in corso, 1 = viceversa 

    _coda_pr* coda;             // array di code con priorità per la terapia intensiva
                                // coda[COVID] e coda[NCOVID]

    int* num_min_reparti;       // numero minimo di reparti per covid e non covid

    int* num_reparti; 
    _reparto** reparto;         // array di array di reparti
                                // reparto[COVID][i] per i da 0 num_reparti[COVID]
                                // reparto[NCOVID][j] per j da 0 num_reparti[NCOVID]

} _ospedale;

void ottieni_prototipo_ospedale_1(_ospedale* o) {

    // definizione caratteristiche dell'ospedale

    int numero_code_pr_covid = 3;
    int numero_code_pr_normale = 2;

    double tasso_arrivo_coda_covid = 10;
    double tasso_arrivo_coda_normale = 15;

    int letti_per_reparto = 12;
    
    int num_reparti_covid = 1;
    int num_min_reparti_covid = 1;

    int num_reparti_normali = 8;
    int num_min_reparti_normali = 4;

    o->soglia_aumento = 80; 
    o->soglia_riduzione = 50;
    o->ampliamento_in_corso = 0;
    o->riduzione_in_corso = 0;

    // inizializza code con priorità

    o->coda = malloc(sizeof(_coda_pr) * 2);

    inizializza_coda_pr(&o->coda[COVID], numero_code_pr_covid, COVID, tasso_arrivo_coda_covid);
    inizializza_coda_pr(&o->coda[NCOVID], numero_code_pr_normale, NCOVID, tasso_arrivo_coda_normale);

    // inizializza reparti

    o->num_reparti = malloc(sizeof(int) * 2);
    o->letti_per_reparto = letti_per_reparto;
    o->num_reparti[COVID] = num_reparti_covid;
    o->num_reparti[NCOVID] = num_reparti_normali;

    o->num_min_reparti = malloc(sizeof(int) * 2);
    o->num_min_reparti[COVID] = num_min_reparti_covid;
    o->num_min_reparti[NCOVID] = num_min_reparti_normali;

    o->reparto = malloc(sizeof(_reparto*) * 2);

    o->reparto[COVID] = malloc(sizeof(_reparto) * o->num_reparti[COVID]);
    for(int i=0; i<o->num_reparti[COVID]; i++)
        prepara_letti_reparto(&o->reparto[COVID][i], o->letti_per_reparto);

    o->reparto[NCOVID] = malloc(sizeof(_reparto) * o->num_reparti[NCOVID]);
    for(int i=0; i<o->num_reparti[NCOVID]; i++)
        prepara_letti_reparto(&o->reparto[NCOVID][i], o->letti_per_reparto);
}

void prova_muovi_paziente_in_letto(_ospedale* o, double tempo_attuale, int tipo) {

    int a = -1;
    int b = -1;

    for(int i=0; i<o->num_reparti[tipo]; i++) { // per ogni reparto dell'ospedale

        if(o->reparto[tipo][i].bloccato == 0) { // valuta solo i reparti non bloccati
    
            for(int j=0; j<o->reparto[tipo][i].num_letti; j++) { // per ogni letto del reparto non bloccato
                
                if(o->reparto[tipo][i].letto[j].occupato == 0) { // se il letto è libero allora esci
                    a = i;
                    b = j;
                    goto fine_controllo_letti;
                }
                // altrimenti continua a cercare
            }
        }
    }
    fine_controllo_letti:

    // se non ho trovato un letto libero allora esco => prova fallita
    if(a == -1 || b == -1) {
        return;
    }

    // se ho un letto libero, rimuovo il primo paziente dalla coda
    int ret = rimuovi_primo_paziente(&o->coda[tipo], tempo_attuale);

    // solo se un paziente è stato rimosso dalla coda allora posso
    // occupare un posto letto
    if(ret == 0)
        occupa_letto(&o->reparto[tipo][a].letto[b], tempo_attuale, tipo);
}

int min(int a, int b) {
    if(a > b)
        return b;
    else
        return a;
}

void prova_transizione_reparto(_ospedale* o, int id_reparto, int tipo) {

    // reparto[tipo][id_reparto] è il reparto in cui è avvenuto un completamento

    // se il reparto su cui è avvenuto il completamento non è bloccato 
    // allora si esce: non serve fare la transizione di questo reparto
    if(o->reparto[tipo][id_reparto].bloccato == 0)
        return;

    // da adesso in poi so che il reparto: o->reparto[tipo][id_reparto] è bloccato

    int letti_occupati = 0;
    for(int i=0; i<o->reparto[tipo][id_reparto].num_letti; i++) // per ogni letto del reparto su cui è avvenuto il completamento
        letti_occupati += o->reparto[tipo][id_reparto].letto[i].occupato;
    
    // se c'è almeno un letto occupato allora non si può fare la transizione
    if(letti_occupati > 0)
        return;
    
    // si procede con la transizione
    // si crea una nuovo ospedale.reparto (di tipo _reparto**) a partire da quello iniziale con le opportune modifiche

    int nuovo_num_reparti_covid;
    int nuovo_num_reparti_normali;
    _reparto** nuovo_reparto;

    if(tipo == COVID) {
        nuovo_num_reparti_covid = o->num_reparti[COVID] - 1;
        nuovo_num_reparti_normali = o->num_reparti[NCOVID] + 1;
    }
    else {
        nuovo_num_reparti_covid = o->num_reparti[COVID] + 1;
        nuovo_num_reparti_normali = o->num_reparti[NCOVID] - 1;
    }

    // alloco il nuovo_reparto con le size diverse e inizializzo reparti e letti

    nuovo_reparto = malloc(sizeof(_reparto*) * 2);
    nuovo_reparto[COVID] = malloc(sizeof(_reparto) * nuovo_num_reparti_covid);
    for(int i=0; i<nuovo_num_reparti_covid; i++)
        prepara_letti_reparto(&nuovo_reparto[COVID][i], o->letti_per_reparto);

    nuovo_reparto[NCOVID] = malloc(sizeof(_reparto) * nuovo_num_reparti_normali);
    for(int i=0; i<nuovo_num_reparti_normali; i++)
        prepara_letti_reparto(&nuovo_reparto[NCOVID][i], o->letti_per_reparto);

    // copio i dati dei vecchi letti dentro i nuovi letti

    int j;
    for(int tipo_reparto=0; tipo_reparto<2; tipo_reparto++) { // itero per tipo_reparto = COVID e tipo_reparto = NCOVID

        j = 0;
        for(int i=0; i<o->num_reparti[tipo_reparto]; i++) {

            if(o->reparto[tipo_reparto][i].bloccato == 0) {
                copia_reparto(&o->reparto[tipo_reparto][i], &nuovo_reparto[tipo_reparto][j]); // sorgente -> destinazione
                j++;
            } 
        }
    }

    // il reparto COVID o NCOVID sta perdendo un reparto
    // si aggiungono le statistiche di output dei letti del reparto che sta per essere 
    // trasferito dentro le statistiche di output dei letti di un qualsiasi altro reparto (il primo)
    aggiungi_statistiche_reparto(&o->reparto[tipo][id_reparto], &nuovo_reparto[tipo][0]);


    // sistemazione strutture dati

    free(o->reparto[COVID]);
    free(o->reparto[NCOVID]);
    free(o->reparto);

    o->ampliamento_in_corso = 0;
    o->riduzione_in_corso = 0;
    o->num_reparti[COVID] = nuovo_num_reparti_covid;
    o->num_reparti[NCOVID] = nuovo_num_reparti_normali;
    o->reparto = nuovo_reparto;
}

int ottieni_reparto_piu_veloce_da_smaltire(_ospedale* o, int tipo) {

    double tempo;
    double tempo_min = INF;
    double indice_min = 0;

    for(int i=0; i<o->num_reparti[tipo]; i++) { // per ogni reparto covid/ncovid dell'ospedale
        tempo = ottieni_tempo_liberazione_reparto(&o->reparto[tipo][i]);
        if(tempo < tempo_min) {
            tempo_min = tempo;
            indice_min = i;
        }
    }

    return indice_min;
}

void inizia_riduzione_reparto(_ospedale* o, int tipo) {

    // ottieni indice del reparto COVID/NCOVID più veloce da smaltire e bloccalo
    int k = ottieni_reparto_piu_veloce_da_smaltire(o, tipo);
    blocca_reparto(&o->reparto[tipo][k]);

    // segnala ampliamento/riduzione in corso
    if(tipo == NCOVID)
        o->ampliamento_in_corso = 1;
    else // if(tipo == COVID)
        o->riduzione_in_corso = 1;
}

void interrompi_riduzione_reparto(_ospedale* o, int tipo) {

    for(int i=0; i<o->num_reparti[tipo]; i++) { // per ogni reparto COVID/NCOVID
        sblocca_reparto(&o->reparto[tipo][i]);  // sblocca il reparto
    }

    if(tipo == NCOVID)
        o->ampliamento_in_corso = 0;
    else // if(tipo == COVID)
        o->riduzione_in_corso = 0;
}

double ottieni_occupazione_reparto_covid(_ospedale* o) {

    int letti_totali = 0;
    int letti_occupati = 0;

    for(int i=0; i<o->num_reparti[COVID]; i++) { // per ogni reparto covid dell'ospedale
        for(int j=0; j<o->reparto[COVID][i].num_letti; j++) { // per ogni letto di quel reparto
            
            // conta i letti totali ed i letti occupati
            letti_totali += 1;
            letti_occupati += o->reparto[COVID][i].letto[j].occupato;
        }   
    }

    return (letti_occupati / letti_totali) * 100;
}