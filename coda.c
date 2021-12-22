#include <stdio.h>
#include <string.h>

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
    paziente** testa;       // array di teste delle code con priorità
    dati_coda* dati;        // array di dati per ogni coda con priorità
} _coda_pr;

double ottieni_prossimo_arrivo_in_coda(int tasso) {
    return exponential(tasso);
}

void aggiorno_flusso_covid(_coda_pr* coda, double tempo_attuale) {

    if(coda->tipo != COVID)
        return;

    // cambia coda->tasso_arrivo in funzione del  
    // tempo attuale e dei dati storici analizzati
}

void inizializza_coda_pr(_coda_pr* coda, int livello_pr, int tasso, int tipo) {

    coda->tipo = tipo;
    coda->tasso_arrivo = tasso;
    coda->prossimo_arrivo = START + ottieni_prossimo_arrivo_in_coda(coda->tasso_arrivo);
    coda->livello_pr = livello_pr;
    coda->dati = malloc(sizeof(dati_coda) * coda->livello_pr);
    coda->testa = malloc(sizeof(paziente*) * coda->livello_pr);

    for(int pr=0; pr<coda->livello_pr; pr++) {
        coda->testa[pr] = NULL;
        coda->dati[pr].num_entrati = 0;
        coda->dati[pr].num_usciti = 0;
        coda->dati[pr].num_morti_in_coda = 0;
        coda->dati[pr].tempo_occupazione = 0;
    }

}

void aggiungi_paziente(_coda_pr* coda, double tempo_attuale) {

    // crea un paziente
    paziente* p = genera_paziente(tempo_attuale, coda->tipo);

    // qui il paziente potrebbe essere trasferito in un'altra coda

    // inseriscilo nell'apposito coda
    int num_coda;

    if(coda->livello_pr == 1) // caso simulazione semplificata
        num_coda = 0;
    else {  // inserisci il paziente nella coda in funzione della sua priorità
        if(coda->tipo == COVID)
            num_coda = p->classe_eta;
        else
            num_coda = p->gravita;
    } 

    paziente* counter = coda->testa[num_coda];
    while (counter != NULL) {
        counter = counter->next;
    }
    counter = p;

    coda->dati[num_coda].num_entrati++;
}

void rimuovi_paziente(_coda_pr* coda, int id, int pr, double tempo_attuale) {
    // il paziente presente in quella coda è morto
    // va rimosso facendo uso del suo id e lo si deve cercare nella
    // coda con priorità "coda" a livello di priorità indicato da "pr" (coda->testa[pr])

    // si aggiorna coda->dati[pr].tempo_occupazione += tempo_attuale - paziente_rimosso->ingresso;
    //             coda->dati[pr].num_usciti++;
}

int rimuovi_primo_paziente(_coda_pr* coda, double tempo_attuale) {
    // si è svuotato un letto e prendo il primo paziente che può usufruire del servizio

    // leggo tutte le code per pr=0; pr<coda->livello_pr; pr++
    // dalla prima coda che contiene almeno un paziente in testa vado a prelevare quel paziente
    
    // rimuovo quel paziente creando una nuova testa

    // aggiorno i dati della coda i° che è stata toccata
    // in particolare   coda->dati[i].tempo_occupazione += tempo_attuale - paziente_rimosso->ingresso;
    //                  coda->dati[i].num_usciti++;


    // return 0 se un paziente è stato rimosso dalla coda
    // return 1 se non vi erano pazienti in coda
    return 0;
}

void calcola_prossimo_arrivo_in_coda(_coda_pr* coda, double tempo_attuale) {
    coda->prossimo_arrivo = tempo_attuale + ottieni_prossimo_arrivo_in_coda(coda->tasso_arrivo);
}