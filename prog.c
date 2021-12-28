#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"
#include "util.c"
#include "stat.c"
#include "paziente.c"
#include "coda.c"
#include "letto.c"
#include "reparto.c"
#include "ospedale.c"

#define ARRIVO 0                  // codice operativo dell'evento "arrivo di un paziente"
#define COMPLETAMENTO 1           // codice operativo dell'evento "completamento di un paziente"
#define TIMEOUT 2                 // codice operativo dell'evento "morte di un paziente in coda"
#define AGGRAVAMENTO 3            // codice operativo dell'evento "aggravamento di un paziente e cambio coda"

// struttura dati che contiene informazioni sul next event

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

#ifdef SIM_INTERATTIVA
#include "interattivo.c"  // fa uso di tutte le struct di sopra
#endif

// variabili globali

#define num_ospedali 1
_ospedale ospedale[num_ospedali];

double tempo_attuale;
double prossimo_giorno;
double tick_per_giorno;

void inizializza_variabili() {

    // inizializza generatore numeri casuali

    PlantSeeds(112233445);
    SelectStream(2); // opzionale

    // inizializza ospedali
    for(int i=0; i< num_ospedali; i++)
        ottieni_prototipo_ospedale_1(&ospedale[i]);

    // inizializza variabili simulazione

    timeout_paziente[COVID] = 30;
    timeout_paziente[NCOVID] = 15;

    servizio_paziente[COVID] = 48;
    servizio_paziente[NCOVID] = 30;

    tempo_attuale = START;
    tick_per_giorno = 24;
    prossimo_giorno = tick_per_giorno; // 24 ore
}

void ottieni_next_event(descrittore_next_event* ne) {

    ne->tempo_ne = INF;

    // cerca il prossimo paziente che entra in una coda
    for (int i = 0; i < num_ospedali; i++) {
        for (int t = 0; t < NTYPE; t++) {
            // se il tasso è nullo non avrò alcun arrivo. Controllo da fare per evitare che il next-event sia sempre un arrivo.
            if (ospedale[i].coda[t].tasso_arrivo != 0 && ne->tempo_ne > ospedale[i].coda[t].prossimo_arrivo) { 
                ne->tempo_ne = ospedale[i].coda[t].prossimo_arrivo;
                ne->tipo = t;
                ne->id_ospedale = i;
                ne->evento = ARRIVO;
            }
        }
    }

    // cerca il prossimo paziente che esce da un letto
    for (int i = 0; i < num_ospedali; i++) {
        for (int t = 0; t < NTYPE; t++) {
            for (int j = 0; j < ospedale[i].num_reparti[t]; j++) {
                for (int k = 0; k < ospedale[i].reparto[t][j].num_letti; k++) {
                    if (ne->tempo_ne > ospedale[i].reparto[t][j].letto[k].servizio) {
                        ne->tempo_ne = ospedale[i].reparto[t][j].letto[k].servizio;
                        ne->evento = COMPLETAMENTO;
                        ne->id_ospedale = i;
                        ne->id_reparto = j;
                        ne->id_letto = k;
                        ne->tipo = t;
                    }
                }
            }
        }
    }

    // cerca il prossimo paziente che muore in attesa in coda
    #ifdef ABILITA_TIMEOUT
    for (int i = 0; i < num_ospedali; i++) {
        for (int t = 0; t < NTYPE; t++) {
            for (int pr = 0; pr < ospedale[i].coda[t].livello_pr; pr++) {
                paziente* counter = ospedale[i].coda[t].testa[pr];
                while (counter != NULL) {
                    if (ne->tempo_ne > counter->timeout) {
                        ne->tempo_ne = counter->timeout;
                        ne->evento = TIMEOUT;
                        ne->id_ospedale = i;
                        ne->id_priorita = pr;
                        ne->id_paziente = counter->id;
                        ne->tipo = t;
                    }
                    counter = counter->next;
                }
            }
        }
    }
    #endif

    // cerca il prossimo che si aggrava
    #ifdef ABILITA_AGGRAVAMENTO
    for (int i = 0; i < num_ospedali; i++) {
        for (int t = 0; t < NTYPE; t++) {
            for (int pr = 0; pr < ospedale[i].coda[t].livello_pr; pr++) {
                paziente* counter = ospedale[i].coda[t].testa[pr];
                while (counter != NULL) {
                    if (ne->tempo_ne > counter->aggravamento) {
                        ne->tempo_ne = counter->aggravamento;
                        ne->evento = AGGRAVAMENTO;
                        ne->id_ospedale = i;
                        ne->id_priorita = pr;
                        ne->id_paziente = counter->id;
                        ne->tipo = t;
                    }
                    counter = counter->next;
                }
            }
        }
    }
    #endif
}

void processa_arrivo(descrittore_next_event* ne) {

    _ospedale* ospedale_di_arrivo = &ospedale[ne->id_ospedale];
    _coda_pr* coda_di_arrivo = &ospedale[ne->id_ospedale].coda[ne->tipo];
    double tempo_di_arrivo = ne->tempo_ne;
    int tipo_di_arrivo = ne->tipo; // COVID o NCOVID

    #ifdef COPERAZIONE_OSPEDALI
    // qui si decide se il paziente deve essere trasferito nella coda di un altro
    // ospedale oppure se può essere inserito nella coda dell'ospedale attuale
    #endif

    aggiungi_paziente(coda_di_arrivo, tempo_di_arrivo); // in questo modo si aggiunge un paziente in coda
    calcola_prossimo_arrivo_in_coda(coda_di_arrivo, tempo_di_arrivo); // genera il tempo del prossimo arrivo nella coda

    // poichè un nuovo paziente è entrata in coda, si controlla
    // se c'è modo di muovere un paziente in un letto libero

    prova_muovi_paziente_in_letto(ospedale_di_arrivo, tempo_di_arrivo, tipo_di_arrivo, 0);
}

void processa_completamento(descrittore_next_event* ne) {

    _ospedale* ospedale_di_completamento = &ospedale[ne->id_ospedale];
    _letto* letto_di_completamento = &ospedale[ne->id_ospedale].reparto[ne->tipo][ne->id_reparto].letto[ne->id_letto];
    double tempo_di_completamento = ne->tempo_ne;
    int tipo_di_completamento = ne->tipo; // COVID o NCOVID

    // gestisci l'operazione del rilascio di un paziente

    rilascia_paziente(ospedale_di_completamento, letto_di_completamento, tempo_di_completamento, tipo_di_completamento);
}

void processa_timeout(descrittore_next_event* ne) {

    _coda_pr* coda_di_timeout = &ospedale[ne->id_ospedale].coda[ne->tipo];
    int id_paziente = ne->id_paziente;
    int livello_priorita = ne->id_priorita;
    int tempo_di_timeout = ne->tempo_ne;

    // elimina la persona dalla coda
    rimuovi_paziente(coda_di_timeout, id_paziente, livello_priorita, tempo_di_timeout);
}

void processa_aggravamento(descrittore_next_event* ne) {

    _coda_pr* coda_di_aggravamento = &ospedale[ne->id_ospedale].coda[ne->tipo];
    int pr_iniziale = ne->id_priorita;
    int pr_finale = pr_iniziale - 1;
    int id_paziente = ne->id_paziente;
    int tempo_aggravamento = ne->tempo_ne;

    // muovi il paziente su un livello di priorità diverso nella coda
    cambia_priorita_paziente(coda_di_aggravamento, pr_iniziale, pr_finale, id_paziente, tempo_aggravamento);
}

void aggiorna_flussi_covid(double tempo_attuale) {

    if(tempo_attuale >= prossimo_giorno) {
        prossimo_giorno += tick_per_giorno;

        // per ogni coda COVID e NCOVID di ogni ospedale invoca:
        for(int i=0; i<num_ospedali; i++)
            aggiorna_flusso_covid(&ospedale[i].coda[COVID], (prossimo_giorno/tick_per_giorno)-1);
    }
}

void genera_output() {
    // somma i dati di tutti i letti e di tutte le code per ogni ospedale
    // mostra i valori medi della simulazione per ogni ospedale e nel complesso tra gli ospedali

    for (int i = 0; i < num_ospedali; i++) {
        for (int t = 0; t < NTYPE; t++) {
            for (int j = 0; j < ospedale[i].num_reparti[t]; j++) {

                for (int l = 0; l < NTYPE; l++) {
                   // ospedale[i].reparto[t][j].letto[l].tempo_occupazione
                }
            }
        }
    }


    char *v[] = {"colonna1","col2","COLONNA_3"};
    genera_csv((char **) v, 3);
  //  riempi_csv();

    // metti l'output in un file csv in modo tale da poter estrarne tabelle e grafici
}

#ifdef TEST
int main() {

}
#else
int main() {
    inizializza_variabili();
    descrittore_next_event* next_event = malloc(sizeof(descrittore_next_event));
    while (tempo_attuale < END) {

        ottieni_next_event(next_event);

        // se abilitato, ferma la simulazione, mostra lo stato
        // degli ospedali ed il prossimo evento. Mettiti in attesa
        // del carattere "invio" prima di processare il next event
        #ifdef SIM_INTERATTIVA
        if(next_event->tempo_ne >= END)
            step_simulazione(ospedale, num_ospedali, tempo_attuale, next_event, 0);
        else
            step_simulazione(ospedale, num_ospedali, tempo_attuale, next_event, 1);
        #endif


        // se abilitato, cerca di aggiornare il flusso di entrata
        // nelle code covid in funzione del giorno attuale
        #ifdef FLUSSO_COVID_VARIABILE
        aggiorna_flussi_covid(next_event->tempo_ne);
        #endif

        // gestisci eventi
        if(next_event->evento == ARRIVO) {
            processa_arrivo(next_event);
        } else if(next_event->evento == COMPLETAMENTO) {
            processa_completamento(next_event);
        } else if(next_event->evento == TIMEOUT) {
            processa_timeout(next_event);
        } else if(next_event->evento == AGGRAVAMENTO) {
            processa_aggravamento(next_event);
        }
        tempo_attuale = next_event->tempo_ne;  // manda avanti il tempo della simulazione
    }
    genera_output();
}
#endif
