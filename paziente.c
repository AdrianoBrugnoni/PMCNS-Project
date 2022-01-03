#define PERC_GIOVANI 5          // percentuale pazienti giovani che accedono alla terapia intensiva covid
#define PERC_MEZZA_ETA 15       // percentuale pazienti di mezza età che accedono alla terapia intensiva covid

#define SOGLIA_GRAVITA 3        // valore soglia di timeout secondo il quale il paziente deve accedere
                                // urgentemente alla terapia intensiva

#define GIOVANE 0               // tipi di paziente.classe_eta
#define MEZZA_ETA 1
#define ANZIANO 2

#define URGENTE 0               // tipi di paziente.gravita
#define SOSTENIBILE 1

typedef struct paziente {
    unsigned long id;           // id del paziente (per la cancellazione)
    int classe_eta;             // classe di età del paziente (solo per pazienti covid)
    int gravita;                // livello di gravita (solo per pazienti normali)

    double timeout;             /* tempo nel quale il paziente muore nella coda.
                                   La rapidità con cui un paziente si aggrava
                                   è funzione di gravità e timeout */
    double ingresso;            // tempo nel quale il paziente è entrato nella coda
    double aggravamento;        /* tempo nel quale il paziente si aggrava e deve cambiare coda
                                   se il paziente è già grave allora questo tempo vale INF */
    struct paziente* next;      // prossimo paziente nella coda
} paziente;

#ifdef MAC_OS
__thread unsigned long next_id = 1;      // id univoco da assegnare al prossimo paziente
__thread double timeout_paziente[NTYPE]; // tempo medio di timeout in coda per i tipi di pazienti
#elif WIN
__declspec(thread) unsigned long next_id = 1;      // id univoco da assegnare al prossimo paziente
__declspec(thread) double timeout_paziente[NTYPE]; // tempo medio di timeout in coda per i tipi di pazienti
#else
thread_local unsigned long next_id = 1;      // id univoco da assegnare al prossimo paziente
thread_local double timeout_paziente[NTYPE]; // tempo medio di timeout in coda per i tipi di pazienti
#endif

double ottieni_timeout_paziente(int tipo) {
    return exponential(timeout_paziente[tipo]);
}

int ottieni_classe_eta_paziente() {

    double var = uniform(0, 100); //Da rivedere
    
    if(var < PERC_GIOVANI)
        return GIOVANE;
    else if(var < PERC_GIOVANI + PERC_MEZZA_ETA)
        return MEZZA_ETA;
    else
        return ANZIANO;
}

double ottieni_gravita_paziente(double ttl) {

    if(ttl <= SOGLIA_GRAVITA)
        return URGENTE;
    else
        return SOSTENIBILE;
}

paziente* genera_paziente(double tempo_attuale, int tipo) {

    // genera il paziente
    paziente* p = malloc(sizeof(paziente));

    p->id = next_id;
    p->classe_eta = ottieni_classe_eta_paziente();
    p->ingresso = tempo_attuale;
    p->timeout = tempo_attuale + ottieni_timeout_paziente(tipo);
    p->gravita = ottieni_gravita_paziente(p->timeout - p->ingresso);
    p->next = NULL;

    if(p->gravita == URGENTE || tipo == COVID)
        p->aggravamento = INF; // non esiste il momento in cui il paziente si aggrava poichè è già urgente (oppure è covid)
    else if(p->gravita == SOSTENIBILE) {
        p->aggravamento = p->timeout - SOGLIA_GRAVITA;
    }

    next_id++;

    return p;
}
