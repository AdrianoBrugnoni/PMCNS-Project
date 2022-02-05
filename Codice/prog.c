#include <stdio.h>
#ifdef WIN
#include <Windows.h>
#elif MAC_OS
#include <pthread.h>
#include <unistd.h>
#else
#include <pthread.h>
#include <threads.h>
#include <unistd.h>
#endif
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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
#include "trasferimento.c"

#define ARRIVO 0                    // codice operativo dell'evento "arrivo di un paziente"
#define COMPLETAMENTO 1             // codice operativo dell'evento "completamento di un paziente"
#define TIMEOUT 2                   // codice operativo dell'evento "morte di un paziente in coda"
#define AGGRAVAMENTO 3              // codice operativo dell'evento "aggravamento di un paziente e cambio coda"
#define TRASFERIMENTO 4             // codice operativo dell'evento "arrivo di un paziente trasferito tra ospedali"
#define AGGIORNAMENTO 5             // codice operativo dell'evento "aggiornamento del flusso covid"

// struttura dati che contiene informazioni sul next event
typedef struct {
    int evento;                     // ARRIVO - COMPLETAMENTO - TIMEOUT
    double tempo_ne;                // tempo a cui avviene il prossimo evento

    int id_ospedale;                // indice dell'ospedale su cui è avvenuto l'evento
    int id_reparto;                 // indice del reparto su cui è avvenuto l'evento
    int id_letto;                   // indice del letto su cui è avvenuto l'evento
    int id_priorita;                // indice del livello di priorità della coda su cui è avvenuto l'evento
    unsigned long id_paziente;      // id del paziente di cui è avvenuto il timeout in coda
    int tipo;                       // COVID - NCOVID

    int id_ospedale_partenza;       // indice dell'ospedale da cui è partito il trasferimento
    int id_ospedale_destinazione;   // indice dell'ospedale su cui arriva il paziente trasferito
    paziente* paziente_trasferito;  // puntatore al paziente che è stato trasferito
} descrittore_next_event;

#ifdef SIM_INTERATTIVA
#include "interattivo.c"            // fa uso di tutte le struct di sopra
#endif

// variabili globali
double tempo_trasferimento[NOSPEDALI][NOSPEDALI];
double soglia_utilizzo;
int nsimulation;
double checkpoint_tempo = 0;            // utilizzato solamente nel calcolo delle statistiche per le Batch Means. Se sono disattivate le Batch Means varrà sempre 0 e non avrà alcuna semantica.
#ifdef BATCH
unsigned int tick_globale = 0;          // conteggio di tutti i tick per le batch means
unsigned int b = 0;                     // coppia (b, k)
unsigned int k = 0;
#endif

// variabili globali per thread
#ifdef MAC_OS
__thread _ospedale ospedale[NOSPEDALI];
__thread _trasferimento* testa_trasferiti;

__thread int fd_code_globale;
__thread int fd_reparti_globale;
__thread int* fd_code[NOSPEDALI];
__thread int* fd_code_global[NOSPEDALI];
__thread int* fd_reparti[NOSPEDALI];
__thread int* fd_reparti_global[NOSPEDALI];

__thread double tempo_attuale;
__thread double prossimo_giorno;
__thread double tick_per_giorno;
#elif WIN
__declspec(thread) _ospedale ospedale[NOSPEDALI];
__declspec(thread) _trasferimento* testa_trasferiti;

__declspec(thread) int fd_code_globale;
__declspec(thread) int fd_reparti_globale;
__declspec(thread) int* fd_code[NOSPEDALI];
__declspec(thread) int* fd_code_global[NOSPEDALI];
__declspec(thread) int* fd_reparti[NOSPEDALI];
__declspec(thread) int* fd_reparti_global[NOSPEDALI];

__declspec(thread) double tempo_attuale;
__declspec(thread) double prossimo_giorno;
__declspec(thread) double tick_per_giorno;
#else
thread_local _ospedale ospedale[NOSPEDALI];
thread_local _trasferimento* testa_trasferiti;

thread_local int fd_code_globale;
thread_local int fd_reparti_globale;
thread_local int* fd_code[NOSPEDALI];
thread_local int* fd_code_global[NOSPEDALI];
thread_local int* fd_reparti[NOSPEDALI];
thread_local int* fd_reparti_global[NOSPEDALI];

thread_local double tempo_attuale;
thread_local double prossimo_giorno;
thread_local double tick_per_giorno;
#endif

void inizializza_variabili() {

    // inizializza generatore numeri casuali
    PlantSeeds(-1);

    // inizializza tempi trasferimento ospedali
    for(int i=0; i<NOSPEDALI; i++) {
        for(int j=0; j<NOSPEDALI; j++) {
            if(i==j)
                tempo_trasferimento[i][j] = 0; // ogni ospedale ha distanza 0 da se stesso
            else
                tempo_trasferimento[i][j] = 1; // tutti gli ospedali sono distanti 1
        }
    }

    // inizializza variabili simulazione
    timeout_paziente[COVID] = 3;
    timeout_paziente[NCOVID] = 3;

    servizio_paziente[COVID] = 480; 
    servizio_paziente[NCOVID] = 410; 

    soglia_utilizzo = 0.5;

    #ifdef SIM_INTERATTIVA
    ultimo_trasferimento.analizzato = 0;
    #endif
}

void inizializza_variabili_per_simulazione(int stream) {

    // selezione del proprio stream
    SelectStream(stream);

    // inizializza ospedali
    _parametri_ospedale param;
    for(int i=0; i<NOSPEDALI; i++) {

        if(i==0) {
            param.media_interarrivo_coda_covid = 1; 
            param.media_interarrivo_coda_normale = 20.8997;
            param.letti_per_reparto = 8;
            param.num_reparti_covid = 1;
            param.num_min_reparti_covid = 1;
            param.num_reparti_normali = 5;
            param.num_min_reparti_normali = 2;
            param.soglia_aumento = 70;
            param.soglia_riduzione = 30;
            param.peso_ospedale = 0.037483;
        } else {
            param.media_interarrivo_coda_covid = 6;
            param.media_interarrivo_coda_normale = 5;
            param.letti_per_reparto = 3;
            param.num_reparti_covid = 1;
            param.num_min_reparti_covid = 1;
            param.num_reparti_normali = 3;
            param.num_min_reparti_normali = 1;
            param.soglia_aumento = 80;
            param.soglia_riduzione = 50;
            param.peso_ospedale = 0.1;
        }

        #ifdef CONDIZIONI_INIZIALI
        inizializza_ospedale_condizioni_iniziali(&ospedale[i], &param);
        #else
        inizializza_ospedale(&ospedale[i], &param);
        #endif
    }

    tempo_attuale = START;
    tick_per_giorno = 24;
    prossimo_giorno = tick_per_giorno; // 24 ore

    // variabili
    testa_trasferiti = NULL;
}

double ottieni_tempo_trasferimento(int from, int to) {
    return tempo_trasferimento[from][to];
    // oppure, genera un valore casuale da una distribuzione che ha
    // come media proprio il valore tempo_trasferimento[from][to]
}

void ottieni_next_event(descrittore_next_event* ne) {

    ne->tempo_ne = INF;

    // cerca il prossimo paziente che entra in una coda
    for (int i = 0; i < NOSPEDALI; i++) {
        for (int t = 0; t < NTYPE; t++) {
            // se il tasso è nullo non avrò alcun arrivo. Controllo da fare per evitare che il next-event sia sempre un arrivo.
            if (ne->tempo_ne > ospedale[i].coda[t].prossimo_arrivo) {
                ne->tempo_ne = ospedale[i].coda[t].prossimo_arrivo;
                ne->tipo = t;
                ne->id_ospedale = i;
                ne->evento = ARRIVO;
            }
        }
    }

    // cerca il prossimo paziente che esce da un letto
    for (int i = 0; i < NOSPEDALI; i++) {
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
    for (int i = 0; i < NOSPEDALI; i++) {
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
    for (int i = 0; i < NOSPEDALI; i++) {
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

    #ifdef COOPERAZIONE_OSPEDALI
    _trasferimento* t = testa_trasferiti;
    while(t != NULL) {
        if(ne->tempo_ne > t->p->ingresso) {
            ne->tempo_ne = t->p->ingresso;
            ne->evento = TRASFERIMENTO;
            ne->id_ospedale_partenza = t->ospedale_partenza;
            ne->id_ospedale_destinazione = t->ospedale_destinazione;
            ne->paziente_trasferito = copia_paziente(t->p);
        }
        t = t->next;
    }
    #endif

    #ifdef FLUSSO_COVID_VARIABILE
    if(ne->tempo_ne > prossimo_giorno) {
        ne->tempo_ne = prossimo_giorno;
        ne->evento = AGGIORNAMENTO;
    }
    #endif
}

void processa_arrivo(descrittore_next_event* ne) {

    _ospedale* ospedale_di_arrivo = &ospedale[ne->id_ospedale];
    _coda_pr* coda_di_arrivo = &ospedale[ne->id_ospedale].coda[ne->tipo];
    double tempo_di_arrivo = ne->tempo_ne;
    int tipo_di_arrivo = ne->tipo; // COVID o NCOVID

    // crea un paziente
    paziente* p = genera_paziente(tempo_di_arrivo, tipo_di_arrivo);

    // se definito, si decide se il paziente deve essere trasferito nella coda di un
    // altro ospedale oppure se può essere inserito nella coda dell'ospedale attuale
    #ifdef COOPERAZIONE_OSPEDALI
    if(tipo_di_arrivo == COVID && NOSPEDALI > 1) {

        // utilizzo_attuale = utilizzo dell'ospedale su cui è avvenuto l'arrivo
        // utilizzo_min = utilizzo più basso tra tutti gli altri ospedali
        // tempo_nuovo_arrivo = tempo nel quale il paziente arriverà all'ospedale prescelto
        // id_nuovo_ospedale = ospedale prescelto

        double utilizzo_attuale = ottieni_livello_utilizzo_zona_covid(ospedale_di_arrivo);
        double utilizzo_min = INT_MAX;
        double tempo_nuovo_arrivo;
        int id_nuovo_ospedale;

        double utilizzo_tmp;

        // cerca il valore minimo di utilizzo tra tutti gli ospedali diversi da quello su cui
        // è avvenuto l'arrivo e tra gli ospedali che possono essere raggiunti prima della morte del paziente
        for(int i=0; i<NOSPEDALI; i++) {

            if(i != ne->id_ospedale) {
                utilizzo_tmp = ottieni_livello_utilizzo_zona_covid(&ospedale[i]);
                if(utilizzo_tmp < utilizzo_min) {
                    utilizzo_min = utilizzo_tmp;
                    tempo_nuovo_arrivo = tempo_di_arrivo + ottieni_tempo_trasferimento(ne->id_ospedale, i);
                    id_nuovo_ospedale = i;
                }
            }
        }

        // se è stato trovato almeno un ospedale che è più libero di
        // una certa quantità allora si effettua il trasferimento
        if(utilizzo_attuale - soglia_utilizzo > utilizzo_min) {

            p->ingresso = tempo_nuovo_arrivo;

            _trasferimento* trasferimento = malloc(sizeof(_trasferimento));
            trasferimento->p = p;
            trasferimento->ospedale_partenza = ne->id_ospedale;
            trasferimento->ospedale_destinazione = id_nuovo_ospedale;
            trasferimento->next = NULL;

            aggiungi_trasferimento(&testa_trasferiti, trasferimento);
            calcola_prossimo_arrivo_in_coda(coda_di_arrivo, tempo_di_arrivo); // genera il tempo del prossimo arrivo nella coda

            #ifdef SIM_INTERATTIVA
            // scrivo i dati relativi all'ultimo trasferimento della simulazione in modo
            // tale da porteli mostrare sulla console nell'analisi dell'evento successivo
            ultimo_trasferimento.ospedale_partenza = trasferimento->ospedale_partenza;
            ultimo_trasferimento.ospedale_destinazione = trasferimento->ospedale_destinazione;
            ultimo_trasferimento.tempo_trasferimento = trasferimento->p->ingresso;
            ultimo_trasferimento.id_paziente = trasferimento->p->id;
            ultimo_trasferimento.analizzato = 1;
            #endif

            // esci
            return;
        }
    }
    #endif

    aggiungi_paziente(coda_di_arrivo, p, DIRETTO); // si aggiunge un paziente in coda
    calcola_prossimo_arrivo_in_coda(coda_di_arrivo, tempo_di_arrivo); // si genera il tempo del prossimo arrivo nella coda

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
    double tempo_di_timeout = ne->tempo_ne;

    // elimina la persona dalla coda
    rimuovi_paziente(coda_di_timeout, id_paziente, livello_priorita, tempo_di_timeout);
}

void processa_aggravamento(descrittore_next_event* ne) {

    _coda_pr* coda_di_aggravamento = &ospedale[ne->id_ospedale].coda[ne->tipo];
    int pr_iniziale = ne->id_priorita;
    int pr_finale = pr_iniziale - 1;
    int id_paziente = ne->id_paziente;
    double tempo_aggravamento = ne->tempo_ne;

    // muovi il paziente su un livello di priorità diverso nella coda
    cambia_priorita_paziente(coda_di_aggravamento, pr_iniziale, pr_finale, id_paziente, tempo_aggravamento);
}

void processa_trasferimento(descrittore_next_event* ne) {

    _ospedale* ospedale_di_destinazione = &ospedale[ne->id_ospedale_destinazione];
    _coda_pr* coda_di_destinazione = &ospedale[ne->id_ospedale_destinazione].coda[COVID];
    paziente* paziente_trasferito = ne->paziente_trasferito;
    unsigned long id_paziente_trasferito = ne->paziente_trasferito->id;
    double tempo_di_arrivo = ne->tempo_ne;

    // il paziente trasferito è appena acceduto all'ospedale di destinazione
    // controllo se il paziente è ancora vivo
    if(paziente_trasferito->timeout > tempo_di_arrivo) {

        // porta il paziente dentro la coda dell'ospedale di arrivo e
        // prova a muovere un paziente dalla coda ad un letto
        aggiungi_paziente(coda_di_destinazione, paziente_trasferito, TRASFERITO);
        prova_muovi_paziente_in_letto(ospedale_di_destinazione, tempo_di_arrivo, COVID, 0);
    } else {

        // il paziente non è sopravvissuto al viaggio
        segnala_morte_in_trasferimento(coda_di_destinazione, paziente_trasferito->classe_eta);
    }

    // rimuovo il paziente dalla lista dei pazienti che devono essere trasferiti
    rimuovi_da_pazienti_in_trasferimento(&testa_trasferiti, id_paziente_trasferito);
}

void processa_aggiorna_flussi_covid(descrittore_next_event* ne) {

    prossimo_giorno += tick_per_giorno; // definisci l'istante in cui verrà ri-scaturito l'evento

    // per ogni coda COVID e NCOVID di ogni ospedale invoca:
    for(int i=0; i<NOSPEDALI; i++)
        aggiorna_flusso_covid(&ospedale[i].coda[COVID], (prossimo_giorno/tick_per_giorno)-1, ospedale[i].peso_ospedale, ne->tempo_ne);
}

// inizializzazione dei csv che memorizzano i dati real-time(RT) inerenti le code
void inizializza_csv_code(int numero_simulazione, char* estensione) {
    char* directory_base = "./output/";
    char base_titolo1[100];
    char* base_titolo2 = "_coda";
    char titolo1[100];
    char titolo2[100];


    strcpy(base_titolo1, directory_base);
    strcat(base_titolo1, int_to_string(numero_simulazione));
#ifdef WIN
    mkdir(base_titolo1);
#else
    mkdir_p(base_titolo1);
#endif
    strcat(base_titolo1, "/");
    strcat(base_titolo1, "dati_ospedale");
    for (int i = 0; i < NOSPEDALI; i++) {
        if(!strstr(estensione,"global"))
            fd_code[i] = (int*)malloc(sizeof(int) * (ospedale[i].coda[COVID].livello_pr + ospedale[i].coda[NCOVID].livello_pr));
        else
            fd_code_global[i] = (int*)malloc(sizeof(int) * (ospedale[i].coda[COVID].livello_pr + ospedale[i].coda[NCOVID].livello_pr));
        strcpy(titolo1, base_titolo1);
        strcat(titolo1, int_to_string(i));
        for (int t = 0; t < NTYPE; t++) {
            for (int pr = 0; pr < ospedale[i].coda[t].livello_pr; pr++) {
                strcpy(titolo2, titolo1);
                strcat(titolo2, base_titolo2);
                strcat(titolo2, "_");
                strcat(titolo2, nome_coda(t, pr));
                strcat(titolo2, estensione);
                if (!strstr(estensione, "global"))
                    fd_code[i][3*t+pr] = inizializza_csv(titolo2, (char**)colonne_dati_code, NCOLONNECODE);
                else
                    fd_code_global[i][3*t+pr] = inizializza_csv(titolo2, (char**)colonne_dati_code, NCOLONNECODE);
            }
        }

    }
}

// inizializzazione dei csv che memorizzano i dati real-time(RT) inerenti i reparti
void inizializza_csv_reparti(int numero_simulazione, char* estensione) {
    char* directory_base = "./output/";
    char base_titolo1[27];
    char* base_titolo2 = "_reparto";
    char titolo1[100];
    char titolo2[100];


    strcpy(base_titolo1, directory_base);
    strcat(base_titolo1, int_to_string(numero_simulazione));
    strcat(base_titolo1, "/");
    strcat(base_titolo1, "dati_ospedale");
    for (int i = 0; i < NOSPEDALI; i++) {
        if (!strstr(estensione, "global"))
            fd_reparti[i] = (int*)malloc(sizeof(int) * NTYPE);
        else
            fd_reparti_global[i] = (int*)malloc(sizeof(int) * NTYPE);
        strcpy(titolo1, base_titolo1);
        strcat(titolo1, int_to_string(i));
        for (int t = 0; t < NTYPE; t++) {
            strcpy(titolo2, titolo1);
            strcat(titolo2, base_titolo2);
            strcat(titolo2, "_");
            strcat(titolo2, nome_da_tipo(t));
            strcat(titolo2, estensione);
            if (!strstr(estensione, "global"))
                fd_reparti[i][t] = inizializza_csv(titolo2, (char**)colonne_dati_reparti, NCOLONNEREPARTI);
            else
                fd_reparti_global[i][t] = inizializza_csv(titolo2, (char**)colonne_dati_reparti, NCOLONNEREPARTI);
        }
    }
}

void genera_output(int tipo_output) {

    // salvataggio dati code
    char** dati = (char**)malloc(sizeof(char*) * NCOLONNECODE);
    int index;
    for (int i = 0; i < NOSPEDALI; i++) {
        index = 0;
        for (int t = 0; t < NTYPE; t++) {
            for (int pr = 0; pr < ospedale[i].coda[t].livello_pr; pr++) {
                dati[0] = int_to_string(ospedale[i].coda[t].dati[pr].accessi_normali);
                dati[1] = int_to_string(ospedale[i].coda[t].dati[pr].accessi_altre_code);
                dati[2] = int_to_string(ospedale[i].coda[t].dati[pr].accessi_altri_ospedali);
                dati[3] = int_to_string(ospedale[i].coda[t].dati[pr].usciti_serviti);
                dati[4] = int_to_string(ospedale[i].coda[t].dati[pr].usciti_morti);
                dati[5] = int_to_string(ospedale[i].coda[t].dati[pr].usciti_aggravati);
                dati[6] = int_to_string(ospedale[i].coda[t].dati[pr].permanenza_serviti);
                dati[7] = int_to_string(ospedale[i].coda[t].dati[pr].permanenza_morti);
                dati[8] = int_to_string(ospedale[i].coda[t].dati[pr].permanenza_aggravati);
                dati[9] = int_to_string(t);
                dati[10] = double_to_string(ospedale[i].coda[t].dati[pr].area / (tempo_attuale - checkpoint_tempo));     //pazienti medi
                dati[11] = double_to_string(sqrt(ospedale[i].coda[t].dati[pr].varianza_wel_numero_pazienti/ ospedale[i].coda[t].dati[pr].index_wel_numero_pazienti));  //varianza num pazienti
#ifdef BATCH
                if(ospedale[i].coda[t].dati[pr].accessi_batch != 0)
                {

                    dati[12] = double_to_string(ospedale[i].coda[t].dati[pr].area / ospedale[i].coda[t].dati[pr].accessi_batch);
#else
                if (ospedale[i].coda[t].dati[pr].accessi_normali +                                   //attesa media
                    ospedale[i].coda[t].dati[pr].accessi_altre_code +
                    ospedale[i].coda[t].dati[pr].accessi_altri_ospedali != 0)
                {

                    dati[12] = double_to_string(ospedale[i].coda[t].dati[pr].area / (ospedale[i].coda[t].dati[pr].accessi_normali+
                                                                                    ospedale[i].coda[t].dati[pr].accessi_altre_code+
                                                                                    ospedale[i].coda[t].dati[pr].accessi_altri_ospedali));
#endif
                }
                else
                    dati[12] = double_to_string(0.0);

                dati[13] = double_to_string(sqrt(ospedale[i].coda[t].dati[pr].varianza_wel_attesa / ospedale[i].coda[t].dati[pr].index_wel_attesa)); //varianza attesa
                dati[14] = double_to_string(tempo_attuale);                                         //tempo simulazione
                if(tipo_output == 0)
                    riempi_csv(fd_code[i][index], dati, NCOLONNECODE);
                else
                    riempi_csv(fd_code_global[i][index], dati, NCOLONNECODE);
                for (int k = 0; k < NCOLONNECODE; k++)
                   free(dati[k]);
                index++;
            }
        }
    }
    free(dati);

    // salvataggio dati reparti
    dati = (char**)malloc(sizeof(char*) * NCOLONNEREPARTI);
    double dati_temp[NCOLONNEREPARTI] = { 0.0 };
    int num_letti = 0;
    for (int i = 0; i < NOSPEDALI; i++) {
        for (int t = 0; t < NTYPE; t++) {

            // leggo i dati relativi ai letti presenti in questo istante della simulazione
            for (int j = 0; j < ospedale[i].num_reparti[t]; j++) {
                for (int k = 0; k < ospedale[i].reparto[t][j].num_letti; k++) {
                    dati_temp[0] += ((double)ospedale[i].reparto[t][j].letto[k].tempo_occupazione); // tempo di occupazione del letto
                    dati_temp[1] += ((double)ospedale[i].reparto[t][j].letto[k].num_entrati); // numero entrati nel letto
                    dati_temp[2] += ((double)ospedale[i].reparto[t][j].letto[k].num_usciti); // numero usciti dal letto
                    
                    // l'ultimo evento della simulazione potrebbe aver comportato uno switch di reparti
                    // in questo caso ci saranno dei letti con tempo di vita pari a 0 e ne va tenuto conto
                    if((tempo_attuale - ospedale[i].reparto[t][j].letto[k].tempo_nascita) != 0) {
                        dati_temp[3] += ((double)ospedale[i].reparto[t][j].letto[k].tempo_occupazione/
                                            (tempo_attuale - ospedale[i].reparto[t][j].letto[k].tempo_nascita)); // percentuale di tempo per cui il letto è stato occupato
                        num_letti++;
                    }
                }
            }

            // leggo i dati relativi ai letti portati nello storico ad opera della transizione di un reparto
            dati_temp[0] += ospedale[i].storico[t].tempo_occupazione_letti;
            dati_temp[1] += ospedale[i].storico[t].pazienti_entrati;
            dati_temp[2] += ospedale[i].storico[t].pazienti_usciti;
            dati_temp[3] += ospedale[i].storico[t].somma_utilizzazione_letti;

            dati[0] = int_to_string((int)dati_temp[0]);
            dati[1] = int_to_string((int)dati_temp[1]);
            dati[2] = int_to_string((int)dati_temp[2]);
            dati[3] = double_to_string(dati_temp[3] / (num_letti + ospedale[i].storico[t].numero_letti_dismessi));
            if (tipo_output == 0)
                riempi_csv(fd_reparti[i][t], dati, NCOLONNEREPARTI);
            else
                riempi_csv(fd_reparti_global[i][t], dati, NCOLONNEREPARTI);
            for (int k = 0; k < NCOLONNEREPARTI; k++) {
                free(dati[k]);
                dati_temp[k] = 0;
            }
            num_letti = 0;
        }
    }
    free(dati);
}

void genera_output_parziale() {
    genera_output(0);
}

void genera_output_globale() {
    genera_output(1);
}

void distruttore() {
    // Chiudi tutti i canali di I/O
#ifdef FLUSSO_COVID_VARIABILE
    __close();
#endif
#ifndef BATCH
    //code
    int index;
    for (int i = 0; i < NOSPEDALI; i++) {
        index = 0;
        for (int t = 0; t < NTYPE; t++) {
            for (int pr = 0; pr < ospedale[i].coda[t].livello_pr; pr++) {
                close(fd_code_global[i][index]);
                index++;
            }
        }
    }
    //reparti
    for (int i = 0; i < NOSPEDALI; i++) {
        for (int t = 0; t < NTYPE; t++) {
            close(fd_reparti_global[i][t]);
        }
    }
#endif
}

void update_stats(double time_next_event) {
    int diff;
    int index;
    for (int i = 0; i < NOSPEDALI; i++) {
        for (int t = 0; t < NTYPE; t++) {
            for (int pr = 0; pr < ospedale[i].coda[t].livello_pr; pr++) {
                ospedale[i].coda[t].dati[pr].area += ((time_next_event - tempo_attuale) * (ospedale[i].coda[t].dati[pr].accessi_normali +
                                                                                          ospedale[i].coda[t].dati[pr].accessi_altre_code +
                                                                                          ospedale[i].coda[t].dati[pr].accessi_altri_ospedali -
                                                                                          ospedale[i].coda[t].dati[pr].usciti_serviti -
                                                                                          ospedale[i].coda[t].dati[pr].usciti_morti -
                                                                                          ospedale[i].coda[t].dati[pr].usciti_aggravati));
                // varianza numero pazienti
                ospedale[i].coda[t].dati[pr].index_wel_numero_pazienti++;
                index = ospedale[i].coda[t].dati[pr].index_wel_numero_pazienti;
                diff = (ospedale[i].coda[t].dati[pr].accessi_normali +
                    ospedale[i].coda[t].dati[pr].accessi_altre_code +
                    ospedale[i].coda[t].dati[pr].accessi_altri_ospedali -
                    ospedale[i].coda[t].dati[pr].usciti_serviti -
                    ospedale[i].coda[t].dati[pr].usciti_morti -
                    ospedale[i].coda[t].dati[pr].usciti_aggravati) - ospedale[i].coda[t].dati[pr].area / (tempo_attuale - checkpoint_tempo);
                ospedale[i].coda[t].dati[pr].varianza_wel_numero_pazienti += diff * diff * (index - 1.0) / index;
            }
        }
    }

}

void* simulation_start(void* input) {
    int in = *((int*)input);
#ifdef AUDIT
    printf("Simulation: %d - STARTED\n", in);
#endif

    inizializza_variabili_per_simulazione(in);
#ifndef BATCH
    inizializza_csv_code(in,"_globali.csv");
    inizializza_csv_reparti(in, "_globali.csv");
#endif
#ifdef GEN_RT
    inizializza_csv_code(in,".csv");
    inizializza_csv_reparti(in, ".csv");
#elif BATCH
    inizializza_csv_code(in, "_batch.csv");
    inizializza_csv_reparti(in, "_batch.csv");
#endif

    descrittore_next_event* next_event = malloc(sizeof(descrittore_next_event));
    while (tempo_attuale < END) {

        ottieni_next_event(next_event);

        // se abilitato, ferma la simulazione, mostra lo stato
        // degli ospedali ed il prossimo evento. Mettiti in attesa
        // del carattere "invio" prima di processare il next event
#ifdef SIM_INTERATTIVA
        if (next_event->tempo_ne >= END)
            step_simulazione(ospedale, NOSPEDALI, tempo_attuale, next_event, testa_trasferiti, 0);
        else
            step_simulazione(ospedale, NOSPEDALI, tempo_attuale, next_event, testa_trasferiti, 1);
#endif

#ifndef GEN_RT
#ifdef BATCH
        if (tick_globale == TICK_END)
            goto end;
        if (tick_globale % k == 0 && tick_globale != 0) {

            genera_output_parziale();
            checkpoint_tempo = tempo_attuale;
            for(int i=0; i<NOSPEDALI; i++)
                azzera_statistiche_ospedale(&ospedale[i], tempo_attuale);
        }
#endif
#endif

        update_stats(next_event->tempo_ne);
        // gestisci eventi
        if (next_event->evento == ARRIVO) {
            processa_arrivo(next_event);
        }
        else if (next_event->evento == COMPLETAMENTO) {
            processa_completamento(next_event);
        }
        else if (next_event->evento == TIMEOUT) {
            processa_timeout(next_event);
        }
        else if (next_event->evento == AGGRAVAMENTO) {
            processa_aggravamento(next_event);
        }
        else if (next_event->evento == TRASFERIMENTO) {
            processa_trasferimento(next_event);
        }
        else if (next_event->evento == AGGIORNAMENTO) {
            processa_aggiorna_flussi_covid(next_event);
        }
        tempo_attuale = next_event->tempo_ne;  // manda avanti il tempo della simulazione
#ifdef GEN_RT
        genera_output_parziale();
#elif BATCH
        tick_globale++;
#endif
    }
    genera_output_globale();
end:
    distruttore();
    return NULL;
}

int inizializza_simulazioni() {
    int select, ret, running_thread = 0, index_checkpoint = 0, wait_checkpoint = 0;
#ifndef BATCH
    printf("\n------------------------------------------");
    printf("\n--- Progetto PMCSN -----------------------");
    printf("\n------------------------------------------\n\n");


    r_menu:
    printf("\n\n\nSelezionare il numero di simulazioni da svolgere: ");
    fflush(stdout);
    ret = scanf("%d", &select);
    getchar();
    if (select <= 0 || select > STREAMS || ret != 1) goto r_menu;
    nsimulation = select;
#ifdef WIN
    int* input = (int*)malloc(sizeof(int) * select);
    HANDLE* hThread = (HANDLE*)malloc(sizeof(HANDLE) * select);
    DWORD* tid = (DWORD*)malloc(sizeof(DWORD) * select);
#else
    int input[select];
    pthread_t tid[select];
#endif
    spawn_thread:
    for (int i = index_checkpoint; i < select; i++) {
        input[i] = i;
        if (running_thread < (MAXNSIMULATION / (NOSPEDALI * (2 + NCODECOVID + NCODENCOVID))*2) - 1) { // 2 sta per i due file corrispondenti alle due tipologie di reparti
#ifdef WIN
            hThread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)simulation_start, (LPVOID)&input[i], NORMAL_PRIORITY_CLASS, tid);
            if (hThread[i] == NULL) {
                printf("Impossibile creare il thread. Errore: %d\n", ret);
                fflush(stdout);
                exit(-1);
            }
#else
#ifdef AUDIT
            printf("%d < %d\n", i, select);
#endif
            if (pthread_create(&tid[i], NULL, simulation_start, &input[i]) != 0) {
                printf("Impossibile creare il thread. Errore: %d\n", ret);
                fflush(stdout);
                exit(-1);
            }
#endif
        }
        else {
            index_checkpoint = i;
            goto wait;
        }
        running_thread++;
    }
wait:

#ifdef WIN
    WaitForSingleObject(hThread[wait_checkpoint], INFINITE);
#else
    pthread_join(tid[wait_checkpoint], NULL);
#endif
    running_thread--;
    wait_checkpoint++;
    if (wait_checkpoint < nsimulation)    goto spawn_thread;
#ifdef ORGANIZZA_DIRECTORY
    organizzatore_directory(nsimulation);
#endif
#else
#ifndef GEN_RT
    printf("\n------------------------------------------");
    printf("\n--- Progetto PMCSN - Modalita' BATCH -----");
    printf("\n------------------------------------------\n\n");

    int input[1] = { 0 };
    b = sqrt(TICK_END / 8);
    k = sqrt(TICK_END * 8);

    nsimulation = 1;
    simulation_start(&input[0]);
#endif
#endif

    return nsimulation;
}


#ifdef TEST
int main() {

}
#else
int main() {

    inizializza_variabili();

    #ifdef SIM_INTERATTIVA
    int stream = 0;
    simulation_start(&stream);
    #else
    inizializza_simulazioni();
    #endif
    printf("\n\nDONE!\n");
    return 0;
}
#endif
