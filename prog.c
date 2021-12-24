#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "common.h"
#include "stat.c"     
#include "paziente.c" // fa uso di commond.h, stat.c, 
#include "coda.c"     // fa uso di common.h, paziente.c
#include "letto.c"    // fa uso di commond.h, stat.c, 
#include "reparto.c"  // fa uso di commond.h, letto.c
#include "ospedale.c" // fa uso di commond.h, reparto.c, coda.c
#include "util.c"

#define FLUSSO_COVID_VARIABILE
#define TERAPIE_VARIABILI

#define ARRIVO 0                  // codice operativo dell'evento "arrivo di un paziente"
#define COMPLETAMENTO 1           // codice operativo dell'evento "completamento di un paziente"
#define TIMEOUT 2                 // codice operativo dell'evento "morte di un paziente in coda"
#define AGGRAVAMENTO 3            // codice operativo dell'evento "aggravamento di un paziente e cambio coda"

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
    int min = -1;
    // cerca l'evento più prossimo tra tutti gli ospedali tra:
    /*
        - il primo paziente che muore in coda (covid e non-covid)
        - il primo paziente che entra in una coda (covid e non-covid)
        - il primo paziente che lascia un letto (covid e non-covid)
        - il primo paziente che necessita un cambio coda (non-covid)
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
    for (int i = 0; i < num_ospedali; i++) {
        for (int t = 0; t < NTYPE; t++) {
            if (min > ospedale[i].coda[t].prossimo_arrivo){
                min = ospedale[i].coda[t].prossimo_arrivo;
                ne->tempo_ne = min;
                ne->tipo = t;
                ne->id_ospedale = i;
                ne->evento = ARRIVO;
            }
        }
    }

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
    for (int i = 0; i < num_ospedali; i++) {
        for (int t = 0; t < NTYPE; t++) {
            for (int j = 0; j < ospedale[i].num_reparti[t]; j++) {
                for (int k = 0; j < ospedale[j].reparto[t][j].num_letti; k++) {
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
    
    // il primo paziente che si aggrava:
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

    return;
}

void processa_arrivo(descrittore_next_event* ne) {

    _ospedale* ospedale_di_arrivo = &ospedale[ne->id_ospedale];
    _coda_pr* coda_di_arrivo = &ospedale[ne->id_ospedale].coda[ne->tipo];
    double tempo_di_arrivo = ne->tempo_ne;
    int tipo_di_arrivo = ne->tipo; // COVID o NCOVID

    aggiungi_paziente(coda_di_arrivo, tempo_di_arrivo); // in questo modo si aggiunge un paziente in coda    
    calcola_prossimo_arrivo_in_coda(coda_di_arrivo, tempo_di_arrivo); // genera il tempo del prossimo arrivo nella coda

    // poichè un nuovo paziente è entrata in coda, si controlla 
    // se c'è modo di muovere un paziente in un letto libero

    prova_muovi_paziente_in_letto(ospedale_di_arrivo, tempo_di_arrivo, tipo_di_arrivo);
}

void processa_completamento(descrittore_next_event* ne) {

    _ospedale* ospedale_di_completamento = &ospedale[ne->id_ospedale];
    _letto* letto_di_completamento = &ospedale[ne->id_ospedale].reparto[ne->tipo][ne->id_reparto].letto[ne->id_letto];
    double tempo_di_completamento = ne->tempo_ne;
    int tipo_di_completamento = ne->tipo; // COVID o NCOVID 
    int id_reparto_completamento = ne->id_reparto;

    rilascia_paziente(letto_di_completamento); // libera il letto su cui è avvenuto il completamento

    // poichè un letto si è liberato, si prova a mandare
    // un paziente dalla coda verso un letto libero

    prova_muovi_paziente_in_letto(ospedale_di_completamento, tempo_di_completamento, tipo_di_completamento);

    // poichè un letto si è liberato, si prova a controllare se un reparto bloccato 
    // è anche vuoto. Se fosse vuoto, allora è necessario effettuare la transizione

    prova_transizione_reparto(ospedale_di_completamento, id_reparto_completamento, tipo_di_completamento);
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

    cambia_priorita_paziente(coda_di_aggravamento, pr_iniziale, pr_finale, id_paziente, tempo_aggravamento);
}

void aggiorna_flussi_covid(double tempo_attuale) {

    if(tempo_attuale >= prossimo_giorno) {
        prossimo_giorno += tick_per_giorno;

        // per ogni coda COVID e NCOVID di ogni ospedale invoca:
        for(int i=0; i<num_ospedali; i++)
            aggiorna_flusso_covid(&ospedale[i].coda[COVID], tempo_attuale);
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
    genera_output();
}

#else
int main() {

    inizializza_variabili();

    descrittore_next_event* next_event = malloc(sizeof(descrittore_next_event));

    while (tempo_attuale < END) {

        ottieni_next_event(next_event); 

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

        // se abilitato, controlla i livello di occupazioni degli ospedali
        // decidi se deve esserci un ampliamento o una riduzione del reparto covid in funzione dell'occupazione
        #ifdef TERAPIE_VARIABILI
        controlla_livelli_occupazione();
        #endif

        tempo_attuale = next_event->tempo_ne;  // manda avanti il tempo della simulazione
    }

    genera_output();
}
#endif



