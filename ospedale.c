#include <stdio.h>
#include <string.h>

typedef struct {
    
    int letti_per_reparto;      // numero di letti per reparto di terapia intensiva
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

    // if(o->reparto[tipo][id_reparto].bloccato == 0)
    //      return

    // fai un ciclo e controlla se il reparto è vuoto
    // reparto[tipo][id_reparto] è il reparto in cui è avvenuto un completamento
    /*

        controlla se o->reparto[tipo][id_reparto].letto[i].occupato == 0 per ogni i da 0 a [...].num_letti
        
        se vero, allora rialloca le vari strutture dati preservando le 
        loro informazioni e cambiandone la dimensione
        si procede nel seguente modo:
            se crea "_reparto** nuovo_reparto" con le dimensioni corrette di reparti covid e non covid
            si inizializza copiando i dati dei reparti e dei letti vecchi
            si fa il free di o->reparto e tutte le strutture collegate
            si fa o->reparto = nuovo_reparto

        se tipo == COVID allora il reparto deve diventare non covid
        se tipo == NCOVID allora il reparto deve diventare covid

        // tieni in mente che non si possono avere 0 reparti covid o 0 reparti non covid
        // vi è un valore di soglia minima per ciascuno di questi
    */
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