typedef struct {
    
    int letti_per_reparto;      // numero di letti per reparto di terapia intensiva (uguale per ogni reparto)
    double soglia_aumento;      // valore di soglia per ampliamento reparto covid
    double soglia_riduzione;    // valore di soglia per riduzione reparto covid
    int ampliamento_in_corso;   // 0 = non c'è ampliamento del reparto covid in corso, 1 = viceversa 
    int riduzione_in_corso;     // 0 = non c'è ampliamento del reparto non covid in corso, 1 = viceversa 

    _coda_pr coda[NTYPE];       // array di code con priorità per la terapia intensiva
                                // coda[COVID] e coda[NCOVID]

    _storico_reparti storico[NTYPE];    // dati relativi ai letti storici (rimossi) dei reparti

    int num_min_reparti[NTYPE]; // numero minimo di reparti per covid e non covid

    int num_reparti[NTYPE];
    _reparto* reparto[NTYPE];   // array di array di reparti
                                // reparto[COVID][i] per i da 0 num_reparti[COVID]
                                // reparto[NCOVID][j] per j da 0 num_reparti[NCOVID]

} _ospedale;

typedef struct {

    double tasso_arrivo_coda_covid;
    double tasso_arrivo_coda_normale;
    int letti_per_reparto;
    int num_reparti_covid;
    int num_min_reparti_covid;
    int num_reparti_normali;
    int num_min_reparti_normali;
    int soglia_aumento;
    int soglia_riduzione; // non meno di 50 se si ha num_min_reparti_covid = 1 !!!

} _parametri_ospedale;

void evento_occupazione_cambiata(_ospedale*, double, int);


void inizializza_ospedale(_ospedale* o, _parametri_ospedale* param) {

    // definizione caratteristiche dell'ospedale

    o->soglia_aumento = param->soglia_aumento; 
    o->soglia_riduzione = param->soglia_riduzione;
    o->ampliamento_in_corso = 0;
    o->riduzione_in_corso = 0;

    // inizializza reparti

    o->letti_per_reparto = param->letti_per_reparto;
    o->num_reparti[COVID] = param->num_reparti_covid;
    o->num_reparti[NCOVID] = param->num_reparti_normali;
    o->num_min_reparti[COVID] = param->num_min_reparti_covid;
    o->num_min_reparti[NCOVID] = param->num_min_reparti_normali;

    for(int tipo=0; tipo<NTYPE; tipo++) {
        o->storico[tipo].pazienti_entrati = 0;
        o->storico[tipo].pazienti_usciti = 0;
        o->storico[tipo].tempo_vita_letti = 0;
        o->storico[tipo].tempo_occupazione_letti = 0;
    }

    o->reparto[COVID] = malloc(sizeof(_reparto) * o->num_reparti[COVID]);
    for(int i=0; i<o->num_reparti[COVID]; i++)
        prepara_letti_reparto(&o->reparto[COVID][i], o->letti_per_reparto, START);

    o->reparto[NCOVID] = malloc(sizeof(_reparto) * o->num_reparti[NCOVID]);
    for(int i=0; i<o->num_reparti[NCOVID]; i++)
        prepara_letti_reparto(&o->reparto[NCOVID][i], o->letti_per_reparto, START);

    // inizializza code con priorità

    #ifdef FLUSSO_COVID_VARIABILE
    param->tasso_arrivo_coda_covid = estrai_tasso_giornata(0);
    #else

    inizializza_coda_pr(&o->coda[COVID], NCODECOVID, param->tasso_arrivo_coda_covid, COVID);
    inizializza_coda_pr(&o->coda[NCOVID], NCODENCOVID, param->tasso_arrivo_coda_normale, NCOVID);
}

int prova_transizione_reparto(_ospedale* o, int id_reparto, int tipo, double tempo_attuale) {

    // reparto[tipo][id_reparto] è il reparto in cui è avvenuto un completamento

    // se il reparto su cui è avvenuto il completamento non è bloccato 
    // allora si esce: non serve fare la transizione di questo reparto
    if(o->reparto[tipo][id_reparto].bloccato == 0)
        return 1;

    // da adesso in poi so che il reparto: o->reparto[tipo][id_reparto] è bloccato
    
    // se c'è almeno un letto occupato allora non si può fare la transizione
    if(letti_occupati_reparto(&o->reparto[tipo][id_reparto]) > 0)
        return 1;
    
    // si procede con la transizione
    // si crea una nuovo ospedale.reparto (di tipo _reparto*[NTYPE]) a partire da quello iniziale con le opportune modifiche

    int nuovo_num_reparti_covid;
    int nuovo_num_reparti_normali;
    _reparto* nuovo_reparto[NTYPE];

    if(tipo == COVID) {
        nuovo_num_reparti_covid = o->num_reparti[COVID] - 1;
        nuovo_num_reparti_normali = o->num_reparti[NCOVID] + 1;
    }
    else {
        nuovo_num_reparti_covid = o->num_reparti[COVID] + 1;
        nuovo_num_reparti_normali = o->num_reparti[NCOVID] - 1;
    }

    // alloco il nuovo_reparto con le size diverse e inizializzo reparti e letti

    nuovo_reparto[COVID] = malloc(sizeof(_reparto) * nuovo_num_reparti_covid);
    for(int i=0; i<nuovo_num_reparti_covid; i++)
        prepara_letti_reparto(&nuovo_reparto[COVID][i], o->letti_per_reparto, tempo_attuale);

    nuovo_reparto[NCOVID] = malloc(sizeof(_reparto) * nuovo_num_reparti_normali);
    for(int i=0; i<nuovo_num_reparti_normali; i++)
        prepara_letti_reparto(&nuovo_reparto[NCOVID][i], o->letti_per_reparto, tempo_attuale);

    // copio i dati dei vecchi letti dentro i nuovi letti

    int j;
    for(int tipo_reparto=0; tipo_reparto<NTYPE; tipo_reparto++) { // itero per tipo_reparto = COVID e tipo_reparto = NCOVID

        // copia tutti i vecchi reparti dentro i nuovi reparti
        // non copiare il reparto bloccato
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
    // trasferito dentro le statistiche storiche del tipo reparto corrispondente
    aggiungi_statistiche_reparto_in_storico(&o->reparto[tipo][id_reparto], &o->storico[tipo], tempo_attuale);


    // sistemazione strutture dati

    free(o->reparto[COVID]);
    free(o->reparto[NCOVID]);

    o->ampliamento_in_corso = 0;
    o->riduzione_in_corso = 0;
    o->num_reparti[COVID] = nuovo_num_reparti_covid;
    o->num_reparti[NCOVID] = nuovo_num_reparti_normali;
    o->reparto[COVID] = nuovo_reparto[COVID];
    o->reparto[NCOVID] = nuovo_reparto[NCOVID];

    // ritorna transizione reparto riuscita
    return 0;
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

int inizia_riduzione_reparto(_ospedale* o, int tipo, double tempo_attuale) {

    // ottieni indice del reparto COVID/NCOVID più veloce da smaltire e bloccalo
    int k = ottieni_reparto_piu_veloce_da_smaltire(o, tipo);
    blocca_reparto(&o->reparto[tipo][k]);

    // segnala ampliamento/riduzione in corso
    if(tipo == NCOVID)
        o->ampliamento_in_corso = 1;
    else // if(tipo == COVID)
        o->riduzione_in_corso = 1;

    // se la transizione di un reparto va a buon fine
    if(prova_transizione_reparto(o, k, tipo, tempo_attuale) == 0) {
        return tipo_opposto(tipo);
    }
    return -1;
}

void interrompi_riduzione_reparto(_ospedale* o, int tipo) {

    for(int i=0; i<o->num_reparti[tipo]; i++) { // per ogni reparto COVID/NCOVID
        sblocca_reparto(&o->reparto[tipo][i]);  // sblocca il reparto
    }

    if(tipo == NCOVID)
        o->ampliamento_in_corso = 0;
    else if(tipo == COVID)
        o->riduzione_in_corso = 0;
}

double ottieni_occupazione_reparto_covid(_ospedale* o) {

    double letti_totali = 0;
    double letti_occupati = 0;

    for(int i=0; i<o->num_reparti[COVID]; i++) { // per ogni reparto covid dell'ospedale
        for(int j=0; j<o->reparto[COVID][i].num_letti; j++) { // per ogni letto di quel reparto
            
            // conta i letti totali ed i letti occupati
            letti_totali += 1;
            letti_occupati += o->reparto[COVID][i].letto[j].occupato;
        }   
    }

    return (letti_occupati / letti_totali) * 100;
}

double ottieni_livello_utilizzo_zona_covid(_ospedale* o) {

    double letti_totali = 0;
    double letti_occupati = 0;
    double pazienti_covid_in_coda = 0;

    for(int i=0; i<o->num_reparti[COVID]; i++) { // per ogni reparto covid dell'ospedale
        for(int j=0; j<o->reparto[COVID][i].num_letti; j++) { // per ogni letto di quel reparto
            
            // conta i letti totali ed i letti occupati
            letti_totali += 1;
            letti_occupati += o->reparto[COVID][i].letto[j].occupato;
        }   
    }

    for(int pr=0; pr<o->coda[COVID].livello_pr; pr++) {
        pazienti_covid_in_coda += numero_elementi_in_coda(&o->coda[COVID], pr);
    }

    return (letti_occupati + pazienti_covid_in_coda) / letti_totali; // questa è la formula del livello di utilizzo
}

int valuta_livello_occupazione(_ospedale* o, double tempo_attuale) {

    // ottieni tasso di occupazione dei letti COVID di quell'ospedale
    double occupazione_reparto_covid = ottieni_occupazione_reparto_covid(o);

    // se non ci sono ne ampliamenti ne riduzioni in corso per il reparto covid
    if(o->ampliamento_in_corso == 0 && o->riduzione_in_corso == 0) {  

        if(occupazione_reparto_covid > o->soglia_aumento && 
            o->num_reparti[NCOVID] > o->num_min_reparti[NCOVID]) { // se ci sono le condizioni per ampliare reparto covid

            return inizia_riduzione_reparto(o, NCOVID, tempo_attuale);
        }

        if(occupazione_reparto_covid < o->soglia_riduzione && 
            o->num_reparti[COVID] > o->num_min_reparti[COVID]) {  // se ci sono le condizioni per ridurre reparto covid

            return inizia_riduzione_reparto(o, COVID, tempo_attuale);
        }

    // se c'è un ampliamento reparto covid in corso
    } else if(o->ampliamento_in_corso == 1) {

        if(occupazione_reparto_covid < o->soglia_riduzione) {

            interrompi_riduzione_reparto(o, NCOVID);

            if(o->num_reparti[COVID] > o->num_min_reparti[COVID]) {
                return inizia_riduzione_reparto(o, COVID, tempo_attuale);
            }
        }

    // se c'è una riduzione reparto covid in corso
    } else if(o->riduzione_in_corso == 1) {

        if(occupazione_reparto_covid > o->soglia_aumento) {

            interrompi_riduzione_reparto(o, COVID);

            if(o->num_reparti[NCOVID] > o->num_min_reparti[NCOVID]) {
                return inizia_riduzione_reparto(o, NCOVID, tempo_attuale);
            }
        } 
    }

    return -1;
}

int prova_muovi_paziente_in_letto(_ospedale* o, double tempo_attuale, int tipo, int esegui_evento) {

    int a = -1;
    int b = -1;

    int k = discrete_uniform(1, o->num_reparti[tipo]) - 1; // estrai l'indice di un reparto casuale

    for(int i=0; i<o->num_reparti[tipo]; i++) { // per ogni reparto dell'ospedale

        if(o->reparto[tipo][k].bloccato == 0) { // valuta solo i reparti non bloccati
    
            for(int j=0; j<o->reparto[tipo][k].num_letti; j++) { // per ogni letto del reparto non bloccato
                
                if(o->reparto[tipo][k].letto[j].occupato == 0) { // se il letto è libero allora esci
                    a = k;
                    b = j;
                    goto fine_controllo_letti;
                }
                // altrimenti continua a cercare
            }
        }

        k = (k + 1) % o->num_reparti[tipo];
    }
    fine_controllo_letti:

    // se non ho trovato un letto libero allora esco => prova fallita
    if(a == -1 || b == -1)
        return 1;

    // se ho un letto libero, rimuovo il primo paziente dalla coda
    int ret = rimuovi_primo_paziente(&o->coda[tipo], tempo_attuale);

    // se non ci sono pazienti nella coda, allora non posso occupare un letto => prova fallita
    if(ret == 1)
        return 1;

    // solo se un paziente è stato rimosso dalla 
    // coda allora posso occupare un posto letto
    occupa_letto(&o->reparto[tipo][a].letto[b], tempo_attuale, tipo);

    if(esegui_evento == 0)
        evento_occupazione_cambiata(o, tempo_attuale, tipo);

    return 0;
}

void rilascia_paziente(_ospedale* o, _letto* l, double tempo_attuale, int tipo) {

    // libera letto su cui è avvenuto il completamento
    libera_letto(l);

    // poichè un letto si è liberato, si prova a mandare
    // un paziente dalla coda verso un letto libero
    prova_muovi_paziente_in_letto(o, tempo_attuale, tipo, 1);

    // attiva il gestore dell'evento dell'occupazione cambiata
    evento_occupazione_cambiata(o, tempo_attuale, tipo);
}

void evento_occupazione_cambiata(_ospedale* o, double tempo_attuale, int tipo) {

    #ifndef TERAPIE_VARIABILI
    return;
    #endif

    int tipo_ampliato;
    int paziente_spostato;
    int esito_transizione;

    prova_ad_ampliare_ancora:

    // controlla i livello di occupazioni degli ospedali decidi se deve esserci un
    // ampliamento o una riduzione del reparto covid in funzione dell'occupazione
    // valuta_livello_occupazione potrebbe invocare prova_transizione_reparto
    tipo_ampliato = valuta_livello_occupazione(o, tempo_attuale); 

    // se ho ampliato un reparto, allora ritorna dentro "tipo_ampliato" il tipo 
    // del reparto (COVID o NCOVID) che è stato ampliato
    if(tipo_esiste(tipo_ampliato) == 0) {

        paziente_spostato = 0;
        // provo a mettere pazienti nei nuovi letti fintanto che c'è modo 
        while(paziente_spostato == 0) {
            paziente_spostato = prova_muovi_paziente_in_letto(o, tempo_attuale, tipo_ampliato, 1);
        }
        goto prova_ad_ampliare_ancora;
    // ho valutato i livelli di occupazione senza ampliare alcun reparto
    // esiste ancora la possibilità che ci sia un reparto bloccato che si è svuotato
    // in quel caso, è necessario effettuare la transizione del reparto
    } else {

        for(int i=0; i<o->num_reparti[tipo]; i++) { // per ogni reparto (COVID o NCOVID a seconda di dove è variata l'occupazione)

            esito_transizione = prova_transizione_reparto(o, i, tipo, tempo_attuale); // provo a fare la transizione
            if(esito_transizione == 0) { // se la transizione è riuscita

                tipo_ampliato = tipo_opposto(tipo);
                
                paziente_spostato = 0;
                // provo a mettere pazienti nei nuovi letti fintanto che c'è modo 
                while(paziente_spostato == 0) {
                    paziente_spostato = prova_muovi_paziente_in_letto(o, tempo_attuale, tipo_ampliato, 1);
                }
                goto prova_ad_ampliare_ancora;
            }
        }

    }
}