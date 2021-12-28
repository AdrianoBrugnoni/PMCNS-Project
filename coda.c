#define MAX_PR 3

typedef struct {
    unsigned long num_entrati;
    unsigned long num_usciti;
    unsigned long num_morti_in_coda;
    unsigned long tempo_occupazione;
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

double estrai_tasso_giornata(int giorno_attuale) {
    return estrai_ricoveri_giornata(giorno_attuale) / SAMPLINGRATE;
}

void aggiorna_flusso_covid(_coda_pr* coda, int giorno_attuale) {
    char* line;
    if(coda->tipo != COVID)
        return;
    coda->tasso_arrivo = estrai_tasso_giornata(giorno_attuale);
}

void inizializza_coda_pr(_coda_pr* coda, int livello_pr, double tasso, int tipo) {

    coda->tipo = tipo;
    coda->tasso_arrivo = tasso;
    coda->prossimo_arrivo = START + ottieni_prossimo_arrivo_in_coda(coda->tasso_arrivo);
    coda->livello_pr = livello_pr;

    for(int pr=0; pr<coda->livello_pr; pr++) {
        coda->testa[pr] = NULL;
        coda->dati[pr].num_entrati = 0;
        coda->dati[pr].num_usciti = 0;
        coda->dati[pr].num_morti_in_coda = 0;
        coda->dati[pr].tempo_occupazione = 0;
    }
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

void aggiungi_paziente(_coda_pr* coda, double tempo_attuale) {

    // crea un paziente
    paziente* p = genera_paziente(tempo_attuale, coda->tipo);

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

    // aggiungilo in fondo alla coda
    aggiungi_in_coda(&coda->testa[num_coda], p);
    coda->dati[num_coda].num_entrati++;
}

void rimuovi_paziente(_coda_pr* coda, int id, int pr, double tempo_attuale) {
    // il paziente presente in quella coda è morto :(
    // va rimosso facendo uso del suo id e lo si deve cercare nella
    // coda con priorità "coda" a livello di priorità indicato da "pr" (coda->testa[pr])

    // si aggiorna coda->dati[pr].tempo_occupazione += tempo_attuale - paziente_rimosso->ingresso;
    //             coda->dati[pr].num_usciti++;
}

//  non morto
int rimuovi_primo_paziente(_coda_pr* coda, double tempo_attuale) {
    // si è svuotato un letto e prendo il primo paziente che può usufruire del servizio

    // leggo tutte le code per pr=0; pr<coda->livello_pr; pr++
    // dalla prima coda che contiene almeno un paziente in testa vado a prelevare quel paziente
    
    // rimuovo quel paziente creando una nuova testa

    // aggiorno i dati della coda i° che è stata toccata
    // in particolare   coda->dati[i].tempo_occupazione += tempo_attuale - paziente_rimosso->ingresso;
    //                  coda->dati[i].num_usciti++;


    // return 0 se un paziente è stato rimosso dalla coda (su un qualsiasi livello di priorità)
    // return 1 se non vi erano pazienti in coda (per nessun livello di priorità)
    return 0;
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

void cambia_priorita_paziente(_coda_pr* coda, int pr_iniziale, int pr_finale, int id_paziente, double tempo_attuale) {
    /* 
        cerca il paziente con id "id_paziente" nella coda: coda->testa[pr_iniziale]
        salva i dati di quel paziente in "paziente* p_tmp"
        rimuovilo da quella coda con: rimuovi_paziente(...) 
            in questo modo vengono aggiornate le statistiche di output. 
            PROBLEMA: invocando rimuovi_paziente(...) viene indicato come fosse morto (num_morti++), quindi si 
            può passare un flag a quella funzione per fargli capire che è solo uscito e non morto

        
        genera un nuovo paziente: paziente* p_finale = genera_paziente(...)
        forza il paziente ad avere dei dati coerenti:
            p_finale->gravita = URGENTE;
            p_finale->timeout = p_tmp->timeout

        aggiungi p_finale in fondo alla coda: coda->testa[pr_finale]
        aggiorna le statistiche di output: coda->dati[pr_finale].num_entrati++

        NOTA: possiamo aggiungere delle statistiche di output rilative ai trasferimenti??
                coda->dati[pr].num_entrati_da_trasferimento
                coda->dati[pr].num_usciti_da_trasferimento
    */
}

void calcola_prossimo_arrivo_in_coda(_coda_pr* coda, double tempo_attuale) {
    coda->prossimo_arrivo = tempo_attuale + ottieni_prossimo_arrivo_in_coda(coda->tasso_arrivo);
}

#ifdef TESTCODA
void main(){

}
#endif