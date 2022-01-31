typedef struct {
    unsigned long pazienti_entrati;
    unsigned long pazienti_usciti;
    double somma_utilizzazione_letti;   // è la sommatoria di "tempo occupazione letto" / "tempo di vita letto"
                                        // per ogni letto che è andato nello storico
    double numero_letti_dismessi;       // numero totali di letti che sono stati portati nello storico
    double tempo_occupazione_letti;     // tempo totale per cui i letti sono stati occupati
    double tempo_vita_letti;            // tempo totale per cui i letti hanno vissuto
} _storico_reparti;

typedef struct {
    int num_letti;  // letti totali per la terapia intensiva
    _letto* letto;  // array dei letti nella sala
    int bloccato;   // 0 == reparto utilizzabile
                    // 1 == reparto bloccato
} _reparto;

int letti_occupati_reparto(_reparto* r) {
    int letti_occupati = 0;
    for(int i=0; i<r->num_letti; i++)
        letti_occupati += r->letto[i].occupato;
    return letti_occupati;
}

void blocca_reparto(_reparto* r) {
    r->bloccato = 1;
}

void sblocca_reparto(_reparto* r) {
    r->bloccato = 0;
}

void prepara_letti_reparto(_reparto* r, int letti_nel_reparto, double tempo_attuale) {

    r->num_letti = letti_nel_reparto;
    r->bloccato = 0;

    r->letto = malloc(sizeof(_letto) * r->num_letti);
    for(int i=0; i<r->num_letti; i++)
        prepara_letto(&r->letto[i], tempo_attuale);
}

double ottieni_tempo_liberazione_reparto(_reparto* r) {

    double max_tempo_liberazione_letto = 0;

    for(int i=0; i<r->num_letti; i++) {    // per ogni letto del reparto

        if(r->letto[i].occupato == 1) { // se il letto è occupato
            if(r->letto[i].servizio > max_tempo_liberazione_letto)
                max_tempo_liberazione_letto = r->letto[i].servizio;
        }      
    }

    // se almeno un letto è occupato ritorna il massimo tra i tempi di liberazione dei letti
    // se nessun letto è occupato ritorna 0 
    return max_tempo_liberazione_letto;
}

void copia_reparto(_reparto* r_src, _reparto* r_dst) {
    // asserisco che hanno lo stesso numero di letti

    for(int i=0; i<r_src->num_letti; i++) // per ogni letto del reparto
        copia_letto(&r_src->letto[i], &r_dst->letto[i]); // sorgente -> destinazione
}

unsigned long pazienti_entrati_reparto(_reparto* r) {
    unsigned long pazienti_entrati = 0;

    for(int i=0; i<r->num_letti; i++) // per ogni letto del reparto
        pazienti_entrati += r->letto[i].num_entrati;

    return pazienti_entrati;
}

unsigned long pazienti_usciti_reparto(_reparto* r) {
    unsigned long pazienti_usciti = 0;

    for(int i=0; i<r->num_letti; i++) // per ogni letto del reparto
        pazienti_usciti += r->letto[i].num_usciti;

    return pazienti_usciti;
}

double tempo_vita_letti_reparto(_reparto* r, double tempo_attuale) {
    double tempo_vita = 0;

    for(int i=0; i<r->num_letti; i++) // per ogni letto del reparto
        tempo_vita += (tempo_attuale - r->letto[i].tempo_nascita);

    return tempo_vita;
}

double tempo_occupazione_letti_reparto(_reparto* r) {
    double tempo_occupazione = 0;

    for(int i=0; i<r->num_letti; i++) // per ogni letto del reparto
        tempo_occupazione += r->letto[i].tempo_occupazione;

    return tempo_occupazione;

}

double somma_utilizzazione_letti_reparto(_reparto* r, double tempo_attuale) {

    double utilizzazione_letti = 0;

    for(int i=0; i<r->num_letti; i++) { // per ogni letto del reparto
       
        // un reparto può essere dimesso nello stesso istante di tempo in cui viene creato
        // in quel caso non si deve portare niente all'interno dello storico
        if(tempo_attuale - r->letto[i].tempo_nascita == 0)
            return -1;
        
        utilizzazione_letti += r->letto[i].tempo_occupazione / (tempo_attuale - r->letto[i].tempo_nascita);
    }

    return utilizzazione_letti;
}

void aggiungi_statistiche_reparto_in_storico(_reparto* r, _storico_reparti* s, double tempo_attuale) {

    s->pazienti_entrati += pazienti_entrati_reparto(r);
    s->pazienti_usciti += pazienti_usciti_reparto(r);
    s->tempo_vita_letti += tempo_vita_letti_reparto(r, tempo_attuale);
    s->tempo_occupazione_letti += tempo_occupazione_letti_reparto(r);

    double somma_utilizzazione = somma_utilizzazione_letti_reparto(r, tempo_attuale);
    if(somma_utilizzazione != -1) {
        s->somma_utilizzazione_letti += somma_utilizzazione;
        s->numero_letti_dismessi += r->num_letti;
    }
}