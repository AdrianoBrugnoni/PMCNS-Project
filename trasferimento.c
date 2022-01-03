typedef struct _trasferimento {
    paziente* p;
    int ospedale_partenza;
    int ospedale_destinazione;
    struct _trasferimento* next;
} _trasferimento;

void aggiungi_trasferimento(_trasferimento** testa, _trasferimento* t) {
    if(*testa == NULL) {
        *testa = t;
    } else {
        _trasferimento* tmp = *testa;
        *testa = t;
        (*testa)->next = tmp;
    }
}

void rimuovi_da_pazienti_in_trasferimento(_trasferimento** testa, int id) {
    
    _trasferimento* t = *testa;
    _trasferimento* q = NULL;

    if((*testa)->p->id == id) { // la testa ha l'id che cerchiamo
        (*testa) = (*testa)->next;
        free(t);
    } else {
        // asserisco che l'id è presente nella lista, quindi
        // la lista deve avere almeno due elementi
        do {
            q = t;
            t = t->next;
            if(t->p->id == id) {
                q->next = t->next;
                free(t);
                return;
            }
        }
        while(t->next != NULL);
    }
}