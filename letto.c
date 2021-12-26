typedef struct {
    int occupato;               // 0 == letto libero, 1 == letto occupato

    double ingresso;            // tempo nel quale il paziente è entrato nel letto
    double servizio;            // tempo nel quale il paziente lascia il letto

    // output simulazione
    double tempo_nascita;       /* tempo nel quale il letto è stato attivato
                                   Necessario poichè i letti possono cambiare funzione
                                   durante la loro vita. A me interessa solo sapere da quanto
                                   tempo un certo letto è attivo per la funzione che svolge adesso */

    double tempo_occupazione;   // tempo per cui il letto è stato occupato
    unsigned long num_entrati;  // numero di pazienti entrati su questo letto
    unsigned long num_usciti;   // numero di pazienti usciti da questo letto
} _letto;

double servizio_paziente[NTYPE];    // tempo medio di servizio nel letto per i tipi di pazienti

double ottieni_tempo_servizio_letto(int tipo) {
    return exponential(servizio_paziente[tipo]);
}

void libera_letto(_letto* l) {

    l->tempo_occupazione += l->servizio - l->ingresso;

    l->occupato = 0;
    l->ingresso = 0;
    l->servizio = INF;
    l->num_usciti += 1;
}

void prepara_letto(_letto* l, double tempo_attuale) {

    l->occupato = 0;
    l->ingresso = 0;
    l->tempo_nascita = tempo_attuale;
    l->servizio = INF;
    l->tempo_occupazione = 0;
    l->num_entrati = 0;
    l->num_usciti = 0;
}

void occupa_letto(_letto* l, double tempo_attuale, int tipo) {

    l->occupato = 1;
    l->ingresso = tempo_attuale;
    l->servizio = tempo_attuale + ottieni_tempo_servizio_letto(tipo);
    l->num_entrati += 1;
}

void copia_letto(_letto* l_src, _letto* l_dst) {

    l_dst->occupato = l_src->occupato;
    l_dst->ingresso = l_src->ingresso;
    l_dst->servizio = l_src->servizio;

    // output simulazione
    l_dst->tempo_nascita = l_src->tempo_nascita;
    l_dst->tempo_occupazione = l_src->tempo_occupazione;
    l_dst->num_entrati = l_src->num_entrati;
    l_dst->num_usciti = l_src->num_usciti;
}

void aggiungi_statistiche_letto(_letto* l_src, _letto* l_dst) {

    l_dst->tempo_occupazione += l_src->tempo_occupazione;
    l_dst->num_entrati += l_src->num_entrati;
    l_dst->num_usciti += l_src->num_usciti;
}