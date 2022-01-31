#define MAX_PR 3

typedef struct {    // dati per un singolo livello di priorità della coda

    unsigned long accessi_normali;          // numero pazienti che accedono alla coda con la modalità classica
    unsigned long accessi_altre_code;       // numero pazienti che accedono alla coda per via di un aggravamento in un'altra coda
    unsigned long accessi_altri_ospedali;   // numero pazienti che accedono alla coda per trasferimento da un altro ospedale

    unsigned long usciti_serviti;           // numero pazienti che lasciano la coda poichè portati in un letto
    unsigned long usciti_morti;             // numero pazienti che lasciano la coda poichè morti
    unsigned long usciti_aggravati;         // numero pazienti che lasciano la coda poichè aggravati e portati a priorità maggiore

    double area;
    double varianza_wel_numero_pazienti;    // valore intermedio per calcolo varianza welford
    int index_wel_numero_pazienti;         // indice welford
    double varianza_wel_attesa;             // valore intermedio per calcolo varianza welford
    int index_wel_attesa;                  // indice welford
    double permanenza_serviti;              // tempo complessivo passato in coda dai pazienti che vengono serviti
    double permanenza_morti;                // tempo complessivo passato in coda dai pazienti che muoiono
    double permanenza_aggravati;            // tempo complessivo passato in coda dai pazienti che abbandonano la coda per aggravamento
} dati_coda;

typedef struct {
    int tipo;               // il tipo della coda, covid o non covid
    double tasso_arrivo;    // tasso di arrivo alla coda
    double prossimo_arrivo; // il tempo in cui avverrà il prossimo arrivo in coda
    int livello_pr;         // numero di code con priorità presenti nella coda
    paziente* testa[MAX_PR];     // array di teste delle code con priorità
    dati_coda dati[MAX_PR];      // array di dati per ogni coda con priorità
} _coda_pr;


double ottieni_prossimo_arrivo_in_coda(double tasso) {
    return exponential(tasso);
}

void calcola_prossimo_arrivo_in_coda(_coda_pr* coda, double tempo_attuale) {
    if(coda->tasso_arrivo == 0)
        coda->prossimo_arrivo = INF;
    else
        coda->prossimo_arrivo = tempo_attuale + ottieni_prossimo_arrivo_in_coda(coda->tasso_arrivo);
}

double estrai_tasso_giornata(int giorno_attuale) {
    return estrai_ricoveri_giornata(giorno_attuale) / SAMPLINGRATE;
}

void aggiorna_flusso_covid(_coda_pr* coda, int giorno_attuale, double tempo_attuale) {
    // esci se la coda non è covid
    if(coda->tipo != COVID)
        return;
    
    // ottieni il nuovo tasso di arrivo in funzione dei dati
    coda->tasso_arrivo = estrai_tasso_giornata(giorno_attuale);

    // se il prossimo arrivo non era definito, allora significa che
    // il precedente tasso di arrivo era pari a 0. Adesso si prova 
    // a generare il prossimo nuovo arrivo
    if(coda->prossimo_arrivo == INF)
        calcola_prossimo_arrivo_in_coda(coda, tempo_attuale);
}

void inizializza_coda_pr(_coda_pr* coda, int livello_pr, double tasso, int tipo) {

    coda->tipo = tipo;
    coda->tasso_arrivo = tasso;
    coda->prossimo_arrivo = START + ottieni_prossimo_arrivo_in_coda(coda->tasso_arrivo);
    coda->livello_pr = livello_pr;

    for(int pr=0; pr<coda->livello_pr; pr++) {
        coda->testa[pr] = NULL;

        coda->dati[pr].accessi_normali = 0;
        coda->dati[pr].accessi_altre_code = 0;
        coda->dati[pr].accessi_altri_ospedali = 0;

        coda->dati[pr].usciti_serviti = 0;
        coda->dati[pr].usciti_morti = 0;
        coda->dati[pr].usciti_aggravati = 0;

        coda->dati[pr].permanenza_serviti = 0;
        coda->dati[pr].permanenza_morti = 0;
        coda->dati[pr].permanenza_aggravati = 0;

        coda->dati[pr].area = 0;
        coda->dati[pr].varianza_wel_numero_pazienti = 0;
        coda->dati[pr].index_wel_numero_pazienti = 1;
        coda->dati[pr].varianza_wel_attesa = 0;
        coda->dati[pr].index_wel_attesa = 1;
    }
}

int numero_elementi_in_coda(_coda_pr* coda, int livello_pr) {

    // conta quante persone ci sono in "coda" per il livello di priorità indicato
    int num_pazienti = 0;

    paziente* counter = coda->testa[livello_pr];
    while (counter != NULL) {
        num_pazienti++;
        counter = counter->next;
    }

    return num_pazienti;
}

void aggiungi_in_coda(paziente** testa, paziente* p) {

    if(*testa == NULL) {
        *testa = p;
    } else {
        paziente* counter = *testa;

        while(counter->next != NULL) {
            counter = counter->next;
        }
        counter->next = p;
    }
}

paziente* rimuovi_per_id(paziente** testa, int id) {

    paziente* p = *testa; // paziente corrente
    paziente* q = NULL;   // paziente precedente

    if((*testa)->id == id) { // la testa ha l'id che cerchiamo
        (*testa) = (*testa)->next;
        p->next = NULL;
        return p;
    // asserisco che l'id è presente nella lista, quindi
    // la lista deve avere almeno due elementi
    } else {

        do {
            q = p;
            p = p->next;
            if(p->id == id) {
                q->next = p->next;
                p->next = NULL;
                return p;
            }
        }
        while(p->next != NULL);
    }

    return NULL;
}

void aggiungi_paziente(_coda_pr* coda, paziente* p, int tipo_ingresso) {

    // capisci in quale coda va inserito
    int num_coda;

    if(coda->livello_pr == 1) // caso simulazione semplificata
        num_coda = 0;
    else {  // inserisci il paziente nella coda in funzione della sua priorità
        if(coda->tipo == COVID)
            num_coda = p->classe_eta;
        else
            num_coda = p->gravita;
    }

    // aggiungilo in fondo alla coda e aggiorna statistiche di output
    aggiungi_in_coda(&coda->testa[num_coda], p);

    if(tipo_ingresso == TRASFERITO)
        coda->dati[num_coda].accessi_altri_ospedali++;
    else if(tipo_ingresso == DIRETTO)
        coda->dati[num_coda].accessi_normali++;
}

void segnala_morte_in_trasferimento(_coda_pr* coda, int pr) {
    coda->dati[pr].accessi_altri_ospedali++;
    coda->dati[pr].usciti_morti++;
}

void rimuovi_paziente(_coda_pr* coda, int id, int pr, double tempo_attuale) {

    // il paziente con id specificato è morto nella coda
    paziente* p = rimuovi_per_id(&coda->testa[pr], id);

    coda->dati[pr].permanenza_morti += tempo_attuale - p->ingresso;
    coda->dati[pr].usciti_morti++;

    free(p);
}

int rimuovi_primo_paziente(_coda_pr* coda, double tempo_attuale) {
    // cerca il paziente con priorità più alta nella coda
    // ed eliminalo (poichè verrà spostato dentro il servente)
    paziente* p;
    for (int pr = 0; pr < coda->livello_pr; pr++) {
        if (coda->testa[pr] != NULL) {

            // elimino la testa
            p = coda->testa[pr];
            coda->testa[pr] = coda->testa[pr]->next;

            // aggiorno dati output
            coda->dati[pr].permanenza_serviti += tempo_attuale - p->ingresso;
            coda->dati[pr].usciti_serviti++;
            free(p);

            return 0;
        }
    }
    return 1;
}

void cambia_priorita_paziente(_coda_pr* coda, int pr_iniziale, int pr_finale, int id_paziente, double tempo_attuale) {

    // rimuovi dalla prima coda e aggiorna statistiche di output

    paziente* p = rimuovi_per_id(&coda->testa[pr_iniziale], id_paziente);

    coda->dati[pr_iniziale].permanenza_aggravati += tempo_attuale - p->ingresso;
    coda->dati[pr_iniziale].usciti_aggravati++;

    // modifica il paziente, inseriscilo nel nuovo livello di priorità e aggiorna statistiche di output

    p->ingresso = tempo_attuale;
    p->aggravamento = INF;
    p->gravita = pr_finale;
    aggiungi_in_coda(&coda->testa[pr_finale], p);

    coda->dati[pr_finale].accessi_altre_code++;
}

#ifdef TESTCODA
void main(){

}
#endif
