double tempo_stop = -1;

char* nome_da_tipo(int tipo) {
    if(tipo == COVID)
        return "covid";
    else
        return "generale";
}

char* nome_coda(int tipo, int pr) {
    if(tipo == COVID) {
        if(pr == 0)
            return "giovani";
        else if(pr == 1) 
            return "adulti";
        else
            return "anziani";
    } else {
        if(pr == 0)
            return "urgenti";
        else
            return "sostenibili";
    }
}

char* nome_evento(int codice_evento) {
    if(codice_evento == ARRIVO)
        return "ARRIVO";
    else if(codice_evento == COMPLETAMENTO)
        return "COMPLETAMENTO";
    else if(codice_evento == TIMEOUT)
        return "TIMEOUT";
    else if(codice_evento == AGGRAVAMENTO)
        return "AGGRAVAMENTO";
    else
        return "";
}

char* stringa_bloccato(int codice) {
    if(codice == 0)
        return "";
    else
        return "(bloccato)";
}

char* si_no_da_numero(int num) {
    if(num == 0)
        return "no";
    else if(num == 1)
        return "si";
    else
        return "";
}

int carattere_e_numero(char c) {
    if(c == '0' || c == '1'  || c == '2'  || c == '3'  || c == '4'  ||
       c == '5' || c == '6'  || c == '7'  || c == '8'  || c == '9') 
        return 0;
    else
        return 1;
}

void stampa_stato_code(_ospedale* ospedale, int num_ospedali) {

    for(int i=0; i<num_ospedali; i++) { // per ogni ospedale

        // stampa info generali ospedale

        printf("\n------------------------------------------");
        printf("\n--- CODA OSPEDALE %d: ---------------------", i);
        printf("\n------------------------------------------\n\n");

        // stampa pazienti per ogni coda

        for(int tipo=0; tipo<NTYPE; tipo++) {

            printf("\tCoda %s (media inter: %.0f) - Prossimo arrivo: %f\n", nome_da_tipo(tipo), ospedale[i].coda[tipo].tasso_arrivo, ospedale[i].coda[tipo].prossimo_arrivo);

            for(int pr=0; pr<ospedale[i].coda[tipo].livello_pr; pr++) { // per livello di priorità di una coda (COVID o NCOVID)
                printf("\t - numero pazienti %s: %d\n", nome_coda(tipo, pr), numero_elementi_in_coda(&ospedale[i].coda[tipo], pr) );

                paziente* c = ospedale[i].coda[tipo].testa[pr];
                while(c != NULL) {

                    printf("\t\tpaziente %lu (in: %.1f, out: %.1f", c->id, c->ingresso, c->timeout);
                    if(tipo == NCOVID && pr == 1)
                        printf(", sw: %.1f)\n", c->aggravamento);
                    else
                        printf(")\n");

                    c = c->next;
                }
            }
            printf("\n");
        }
    
    }
}

void stampa_simluazione_attuale(_ospedale* ospedale, int num_ospedali, double tempo_attuale, descrittore_next_event* next_event) {

    for(int i=0; i<num_ospedali; i++) { // per ogni ospedale

        // stampa info generali ospedale

        printf("\n------------------------------------------");
        printf("\n--- OSPEDALE %d: --------------------------", i);
        printf("\n------------------------------------------\n\n");

        printf("Occupazione covid: %.2f%%\n", ottieni_occupazione_reparto_covid(&ospedale[i]));
        printf("Ampliamento in corso: %s\nRiduzione in corso: %s\n\n", si_no_da_numero(ospedale[i].ampliamento_in_corso), si_no_da_numero(ospedale[i].riduzione_in_corso));

        // stampa dati storici;

        for(int tipo=0; tipo<NTYPE; tipo++) {
            unsigned long pazienti_entrati = ospedale->storico[tipo].pazienti_entrati;
            unsigned long pazienti_usciti = ospedale->storico[tipo].pazienti_usciti;
            double tempo_vita_letti = ospedale->storico[tipo].tempo_vita_letti;
            double tempo_occupazione_letti = ospedale->storico[tipo].tempo_occupazione_letti;
            printf("Storico %s: (in: %lu, out: %lu, usati %.1f su %.1f) \n", nome_da_tipo(tipo), pazienti_entrati, pazienti_usciti, tempo_occupazione_letti, tempo_vita_letti);
        }
        printf("\n");

        // stampa letti per reparto

        for(int tipo=0; tipo<NTYPE; tipo++) {

            for(int j=0; j<ospedale[i].num_reparti[tipo]; j++) { // per ogni reparto COVID o NCOVID
                
                printf("\treparto t.i. %s %d %s\n", nome_da_tipo(tipo), j, stringa_bloccato(ospedale[i].reparto[tipo][j].bloccato));

                for(int k=0; k<ospedale[i].reparto[tipo][j].num_letti; k++) { // per ogni letto del reparto

                    double servizio_letto;
                    double tempo_occupazione = ospedale[i].reparto[tipo][j].letto[k].tempo_occupazione;
                    double tempo_attivo = tempo_attuale - ospedale[i].reparto[tipo][j].letto[k].tempo_nascita;
                    unsigned long num_entrati = ospedale[i].reparto[tipo][j].letto[k].num_entrati;
                    unsigned long num_usciti = ospedale[i].reparto[tipo][j].letto[k].num_usciti;

                    if(ospedale[i].reparto[tipo][j].letto[k].occupato == 1)
                        servizio_letto = ospedale[i].reparto[tipo][j].letto[k].servizio;
                    else 
                        servizio_letto = 0;

                    printf("\t\tletto %d: %f ", k, servizio_letto);
                    printf("(in: %lu, out: %lu, usato %.1f su %.1f)\n", num_entrati, num_usciti, tempo_occupazione, tempo_attivo);
                }
            }
            printf("\t------------------------------------------\n");
        }

        // stampa situazione generale code

        for(int tipo=0; tipo<NTYPE; tipo++) {

            printf("\tCoda %s (media inter: %.0f) - Prossimo arrivo: %f\n", nome_da_tipo(tipo), ospedale[i].coda[tipo].tasso_arrivo, ospedale[i].coda[tipo].prossimo_arrivo);

            for(int pr=0; pr<ospedale[i].coda[tipo].livello_pr; pr++) { // per livello di priorità di una coda (COVID o NCOVID)
                printf("\t - numero pazienti %s: %d\n", nome_coda(tipo, pr), numero_elementi_in_coda(&ospedale[i].coda[tipo], pr) );
            }
            printf("\n");
        }
    }

    // stampa informazioni sul prossimo evento

    printf("\nPROSSIMO EVENTO:\t%s - ", nome_evento(next_event->evento));
    if(next_event->evento == ARRIVO) {
        printf("ospedale %d coda %s\n", next_event->id_ospedale, nome_da_tipo(next_event->tipo));
    } else if(next_event->evento == COMPLETAMENTO) {
        printf("ospedale %d, reparto %s %d, letto %d\n", next_event->id_ospedale, nome_da_tipo(next_event->tipo), next_event->id_reparto, next_event->id_letto);
    }
    printf("\t\t\ttempo attuale:    %f\n", tempo_attuale);
    printf("\t\t\ttempo next event: %f\n", next_event->tempo_ne);
}

void step_simulazione(_ospedale* ospedale, int num_ospedali, double tempo_attuale, descrittore_next_event* next_event, int step_forzato) {

    if(tempo_attuale < tempo_stop && step_forzato == 1) { // fintanto che il tempo attuale non raggiunge il tempo di stop si va avanti veloce
        return;
    }

    stampa_simluazione_attuale(ospedale, num_ospedali, tempo_attuale, next_event);

    char comando[20];
    char indice = 0;
    char c;

    preleva_input:

    printf("\n>");
    while(1) { // leggi tutti i caratteri che sono stati inseriti da terminale

        c = getchar(); // preleva il singolo carattere

        if(c == '\n') { // se il carattere è '\n' allora la stringa è finita

            // se solo '\n' è stato inserito allora si va direttamente al prossimo evento
            if(indice == 0) { // vai avanti al prossimo evento
                return;
            }
            // se ho inserito un singolo carattere non numero vado ad analizzare i comandi da un carattere
            else if(indice == 1 && carattere_e_numero(comando[0]) == 1) {
                
                if(comando[0] == 'f') { // finisci simulazione
                    tempo_stop = END;
                    return;
                }
                else if(comando[0] == 'c')
                    stampa_stato_code(ospedale, num_ospedali);
                else if(comando[0] == 's') // ristampa dati evento attuale
                    stampa_simluazione_attuale(ospedale, num_ospedali, tempo_attuale, next_event);
                    
                indice = 0;
                goto preleva_input;
            }
            // altrimenti, leggi la stringa inserita e prendi una decisione
            else {
                comando[indice + 1] = '\0';
                int tempo_da_saltare = atoi(comando);
                tempo_stop = tempo_attuale + tempo_da_saltare;
                return;
            }
    
        } else {
            comando[indice] = c;
            indice++;
        } 
    }
}