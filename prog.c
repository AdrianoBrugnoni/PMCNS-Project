#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "stat.c"     
#include "paziente.c" // fa uso di commond.h, stat.c, 
#include "coda.c"     // fa uso di common.h, paziente.c
#include "letto.c"    // fa uso di commond.h, stat.c, 
#include "reparto.c"  // fa uso di commond.h, letto.c
#include "ospedale.c" // fa uso di commond.h, reparto.c, coda.c

#define FLUSSO_COVID_VARIABILE
#define TERAPIE_VARIABILI

#define ARRIVO 0                  // codice operativo dell'evento "arrivo di un paziente"
#define COMPLETAMENTO 1           // codice operativo dell'evento "completamento di un paziente"
#define TIMEOUT 2                 // codice operativo dell'evento "morte di un paziente in coda"

// definizione dati per la simulazione

typedef struct {
    int evento;                   // ARRIVO - COMPLETAMENTO - TIMEOUT
    double tempo_ne;              // tempo a cui avviene il prossimo evento

    int id_ospedale;              // indice dell'ospedale su cui è avvenuto l'evento
    int id_reparto;               // indice del reparto su cui è avvenuto l'evento
    int id_letto;                 // indice del letto su cui è avvenuto l'evento
    int id_priorita;              // indice del livello di priorità della coda su cui è avvenuto l'evento
    int id_paziente;              // id del paziente di cui è avvenuto il timeout in coda
    int tipo;                     // COVID - NCOVID

} descrittore_next_event;

// variabili globali

int num_ospedali;
_ospedale* ospedale;

double tempo_attuale;
double prossimo_giorno;
double tick_per_giorno;

void inizializza_variabili() {

    // inizializza ospedali

    num_ospedali = 2;
    ospedale = malloc(sizeof(ospedale) * num_ospedali);

    ottieni_prototipo_ospedale_1(&ospedale[0]);
    ottieni_prototipo_ospedale_1(&ospedale[1]);

    // inizializza variabili simulazione

    tempo_attuale = START;
    tick_per_giorno = 24;
    prossimo_giorno = tick_per_giorno; // 24 ore
}

void ottieni_next_event(descrittore_next_event* ne) {
    // cerca l'evento più prossimo tra tutti gli ospedali tra:
    /*
        - il primo paziente che muore in coda (covid e non-covid)
        - il primo paziente che entra in una coda (covid e non-covid)
        - il primo paziente che lascia un letto (covid e non-covid)
    */
    // cerco solo eventi che avvengono dopo il tempo attuale

    // il primo paziente che entra in una coda:
    /*
        per ogni ospedale i da 0 a num_ospedali
            per tipo = COVID e tipo = NCOVID (0 e 1)
                cerca il minimo di: ospedale[i].coda[tipo].prossimo_arrivo
    
        // scrive in ne i dati
        ne->tempo_ne = ospedale[i].coda[tipo].prossimo_arrivo
        ne->evento = ARRIVO
        ne->id_ospedale = i
        ne->tipo = COVID oppure NCODIV
    */

    // il primo paziente che esce da un letto:
    /*
        per ogni ospedale i da 0 a num_ospedali
            per tipo = COVID e tipo = NCOVID (0 e 1)
                per ogni reparto j da 0 a ospedale[i].num_reparti[tipo]
                    per ogni letto k da 0 a ospedale[i].reparto[tipo][j].num_letti
                        cerca il minimo di: ospedale[i].reparto[tipo][j].letto[k].servizio
          
        // scrive in ne i dati
        ne->tempo_ne = ospedale[i].reparto[tipo][j].letto[k].servizio
        ne->evento = COMPLETAMENTO
        ne->id_ospedale = i
        ne->id_reparto = j
        ne->id_letto = k
        ne->tipo = COVID oppure NCODIV
    */

    // il primo paziente che muore in coda:
    /*
        per ogni ospedale i da 0 a num_ospedali, cerca il minimo tra
            per tipo = COVID e tipo = NCOVID (0 e 1)
                per ogni coda priorita pr da 0 a ospedale[i].coda[tipo].livello_pr
                    paziente = ospedale[i].coda[tipo].testa[pr]
                    per ogni paziente nella lista (paziente = paziente->next)
                        cerco il minimo di: paziente->timeout
                
        // scrive in ne i dati
        ne->tempo_ne = paziente->timeout
        ne->evento = TIMEOUT
        ne->id_ospedale = i
        ne->id_priorita = pr
        ne->id_paziente = paziente->id
        ne->tipo = COVID oppure NCODIV
    */

    ne->evento = ARRIVO; 

    return;
}

void processa_arrivo(descrittore_next_event* ne) {
    // l'arrivo è in ospedale[ne->id_ospedale].coda_covid OPPURE ospedale[ne->id_ospedale].coda_normale in funzione di ne->tipo
    // invoca aggiungi_paziente(&ospedale[ne->id_ospedale].coda[ne->tipo], ne->tempo_ne);
    // in questo modo si aggiunge un paziente in coda

    // invoca prova_muovi_paziente_in_letto(&ospedale[ne->id_ospedale], ne->tempo_ne, ne->tipo)
    // in questo modo, si vede se c'è un posto libero dentro un letto ed in caso
    // si prende il paziente più importante e lo si ci mette
}

void processa_completamento(descrittore_next_event* ne) {
    // prendi da "ne" il letto da cui è avvenuto il completamento -> Liberalo
    // per farlo si invoca rilascia_paziente(&ospedale[ne->id_ospedale].reparto_covid[ne->id_reparto].letto[ne->id_letto]);

    // invoca prova_muovi_paziente_in_letto(&ospedale[ne->id_ospedale], ne->tempo_ne, ne->tipo)
    // in questo modo, si vede se c'è un posto libero dentro un letto ed in caso
    // si prende il paziente più importante e lo si ci mette

    // invoca prova_transizione_reparto(&ospedale[ne->id_ospedale], ne->id_reparto, ne->tipo)
    // in questo modo, si controlla se il reparto bloccato è vuoto dopo questo completamento
    // Se cosi fosse, allora allora bisogna rimuovere quel reparto dalle TI normale e metterlo nelle TI covid (o viceversa)
}

void processa_timeout(descrittore_next_event* ne) {
    // elimina la persona dalla coda invocando 
    //    rimuovi_paziente(ospedale[ne->id_ospedale].coda[ne->tipo], ne->id_paziente, ne->id_priorita, ne->tempo_ne)
    // prendi da "ne" la coda in cui è morto il paziente ed il suo numero identificativo
}

void aggiorna_flussi_covid() {

    if(tempo_attuale >= prossimo_giorno) {
        prossimo_giorno += tick_per_giorno;

        // per ogni coda COVID e NCOVID di ogni ospedale invoca:
        // aggiorno_flusso_covid(&ospedale[i].coda[COVID], tempo_attuale);

    }
}

void controlla_livelli_occupazione() {

    for(int i=0; i<num_ospedali; i++) { // per ogni ospedale i

        // ottieni tasso di occupazione dei letti COVID di quell'ospedale
        double occupazione_reparto_covid = ottieni_occupazione_reparto_covid(&ospedale[i]);

        // se non ci sono ne ampliamenti ne riduzioni in corso per il reparto covid
        if(ospedale[i].ampliamento_in_corso == 0 && ospedale[i].riduzione_in_corso == 0) {  

            if(occupazione_reparto_covid > ospedale[i].soglia_aumento && 
                ospedale[i].num_reparti[NCOVID] > ospedale[i].num_min_reparti[NCOVID]) { // se ci sono le condizioni per ampliare reparto covid

                inizia_riduzione_reparto(&ospedale[i], NCOVID);
            }

            if(occupazione_reparto_covid < ospedale[i].soglia_riduzione && 
                ospedale[i].num_reparti[COVID] > ospedale[i].num_min_reparti[COVID]) {  // se ci sono le condizioni per ridurre reparto covid

                inizia_riduzione_reparto(&ospedale[i], COVID);
            }

        // se c'è un ampliamento reparto covid in corso
        } else if(ospedale[i].ampliamento_in_corso == 1) {

            if(occupazione_reparto_covid < ospedale[i].soglia_riduzione) {

                interrompi_riduzione_reparto(&ospedale[i], NCOVID);

                if(ospedale[i].num_reparti[COVID] > ospedale[i].num_min_reparti[COVID]) {
                    inizia_riduzione_reparto(&ospedale[i], COVID);
                }
            }

        // se c'è una riduzione reparto covid in corso
        } else if(ospedale[i].riduzione_in_corso == 1) {

            if(occupazione_reparto_covid > ospedale[i].soglia_aumento) {

                interrompi_riduzione_reparto(&ospedale[i], COVID);

                if(ospedale[i].num_reparti[NCOVID] > ospedale[i].num_min_reparti[NCOVID]) {
                    inizia_riduzione_reparto(&ospedale[i], NCOVID);
                }
            } 
        }

    }
}

void genera_output() {
    // somma i dati di tutti i letti e di tutte le code per ogni ospedale
    // mostra i valori medi della simulazione per ogni ospedale e nel complesso tra gli ospedali

    // metti l'output in un file csv in modo tale da poter estrarne tabelle e grafici
}

int main() {

    inizializza_variabili();

    descrittore_next_event* next_event = malloc(sizeof(descrittore_next_event));

    while (tempo_attuale < END) {

        ottieni_next_event(next_event); 

        if(next_event->evento == ARRIVO) {
            processa_arrivo(next_event);
        } else if(next_event->evento == COMPLETAMENTO) {
            processa_completamento(next_event);
        } else if(next_event->evento == TIMEOUT) {
            processa_timeout(next_event);
        }

        // se abilitato, cerca di aggiornare il flusso di entrata nelle code covid
        #ifdef FLUSSO_COVID_VARIABILE
        aggiorna_flussi_covid();
        #endif

        // se abilitato, controlla i livello di occupazioni degli ospedali
        // decidi se deve esserci un ampliamento o una riduzione del reparto covid in funzione dell'occupazione
        #ifdef TERAPIE_VARIABILI
        controlla_livelli_occupazione();
        #endif

        tempo_attuale = next_event->tempo_ne;  // manda avanti il tempo della simulazione
    }

    genera_output();

}
