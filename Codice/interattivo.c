double tempo_stop = -1;

typedef struct {
    int analizzato;             // 0 se è stato già mostrato su console, 1 altrimenti
    int ospedale_partenza;
    int ospedale_destinazione;
    unsigned long id_paziente;
    double tempo_trasferimento;
} _ultimo_trasferimento;
_ultimo_trasferimento ultimo_trasferimento;

char* nome_evento(int codice_evento) {
    if(codice_evento == ARRIVO)
        return "ARRIVO";
    else if(codice_evento == COMPLETAMENTO)
        return "COMPLETAMENTO";
    else if(codice_evento == TIMEOUT)
        return "TIMEOUT";
    else if(codice_evento == AGGRAVAMENTO)
        return "AGGRAVAMENTO";
    else if(codice_evento == TRASFERIMENTO)
        return "TRASFERIMENTO";
    else if(codice_evento == AGGIORNAMENTO)
        return "AGGIORNAMENTO FLUSSO";
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

        // stampa su ogni coda e su ogni livello di priorità

        for(int tipo=0; tipo<NTYPE; tipo++) {

            printf("\tCoda %s (media inter: %.4f) - Prossimo arrivo: %.4f\n", nome_da_tipo(tipo), ospedale[i].coda[tipo].media_interarrivi, ospedale[i].coda[tipo].prossimo_arrivo);


            for(int pr=0; pr<ospedale[i].coda[tipo].livello_pr; pr++) { // per livello di priorità di una coda (COVID o NCOVID)
                printf("\t - numero pazienti %s: %d\n", nome_coda(tipo, pr), numero_elementi_in_coda(&ospedale[i].coda[tipo], pr) );

                // stampa statistiche output per livello priorità della coda

                printf("\t\t# accessi (normali: %lu", ospedale[i].coda[tipo].dati[pr].accessi_normali);
                printf(", altre code: %lu", ospedale[i].coda[tipo].dati[pr].accessi_altre_code);
                printf(", altri ospedali: %lu)\n", ospedale[i].coda[tipo].dati[pr].accessi_altri_ospedali);

                printf("\t\t# usciti (serviti: %lu", ospedale[i].coda[tipo].dati[pr].usciti_serviti);
                printf(", morti: %lu", ospedale[i].coda[tipo].dati[pr].usciti_morti);
                printf(", aggravati: %lu)\n", ospedale[i].coda[tipo].dati[pr].usciti_aggravati);

                printf("\t\t# permanenza (serviti: %.1f", ospedale[i].coda[tipo].dati[pr].permanenza_serviti);
                printf(", morti: %.1f", ospedale[i].coda[tipo].dati[pr].permanenza_morti);
                printf(", aggravati: %.1f)\n", ospedale[i].coda[tipo].dati[pr].permanenza_aggravati);

                // stampa tutti i pazienti in quel livello di priorità della coda

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

void stampa_trasferimenti(_trasferimento* testa_trasferiti) {

    printf("\n------------------------------------------");
    printf("\n--- Lista trasferimenti ------------------");
    printf("\n------------------------------------------\n\n");

    _trasferimento* t = testa_trasferiti;
    while(t != NULL) {
        printf("\tpaziente %lu da ospedale %d->%d (arrivo %.2f)\n", t->p->id, t->ospedale_partenza, t->ospedale_destinazione, t->p->ingresso);
        t = t->next;
    }
}

void stampa_simulazione_attuale(_ospedale* ospedale, int num_ospedali, double tempo_attuale, descrittore_next_event* next_event) {

    for(int i=0; i<num_ospedali; i++) { // per ogni ospedale

        // stampa info generali ospedale

        printf("\n------------------------------------------");
        printf("\n--- OSPEDALE %d: --------------------------", i);
        printf("\n------------------------------------------\n\n");

        printf("Occupazione covid: %.2f%%, Livello utilizzo: %.2f\n", ottieni_occupazione_reparto_covid(&ospedale[i]), ottieni_livello_utilizzo_zona_covid(&ospedale[i]));
        printf("Ampliamento in corso: %s, Riduzione in corso: %s\n\n", si_no_da_numero(ospedale[i].ampliamento_in_corso), si_no_da_numero(ospedale[i].riduzione_in_corso));

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
            
            if(ospedale[i].coda[tipo].media_interarrivi != INF)
                printf("\tCoda %s (media inter: %.4f) - Prossimo arrivo: %.4f\n", nome_da_tipo(tipo), ospedale[i].coda[tipo].media_interarrivi, ospedale[i].coda[tipo].prossimo_arrivo);
            else
                printf("\tCoda %s (media inter: 0) - Prossimo arrivo: %f\n", nome_da_tipo(tipo), ospedale[i].coda[tipo].prossimo_arrivo);
            
            for(int pr=0; pr<ospedale[i].coda[tipo].livello_pr; pr++) { // per livello di priorità di una coda (COVID o NCOVID)
                printf("\t - numero pazienti %s: %d\n", nome_coda(tipo, pr), numero_elementi_in_coda(&ospedale[i].coda[tipo], pr) );
            }
            printf("\n");
        }
    }

    // mostra se è stato avviato un trasferimento nella precedente iterazione

    if(ultimo_trasferimento.analizzato == 1) {
        ultimo_trasferimento.analizzato = 0;
        printf("\nTRASFERIMENTO IN EVENTO PRECEDENTE\n");
        printf("\tpaziente %lu da ospedale %d->%d", ultimo_trasferimento.id_paziente, ultimo_trasferimento.ospedale_partenza, ultimo_trasferimento.ospedale_destinazione);
        printf(" (arrivo %.2f)\n", ultimo_trasferimento.tempo_trasferimento);
    }

    // stampa informazioni sul prossimo evento

    printf("\nPROSSIMO EVENTO:\t%s - ", nome_evento(next_event->evento));
    if(next_event->evento == ARRIVO) {
        printf("ospedale %d coda %s\n", next_event->id_ospedale, nome_da_tipo(next_event->tipo));
    } else if(next_event->evento == COMPLETAMENTO) {
        printf("ospedale %d, reparto %s %d, letto %d\n", next_event->id_ospedale, nome_da_tipo(next_event->tipo), next_event->id_reparto, next_event->id_letto);
    } else if(next_event->evento == AGGRAVAMENTO || next_event->evento == TIMEOUT) {
        printf("ospedale %d, coda %s (id:%lu)\n", next_event->id_ospedale, nome_coda(next_event->tipo, next_event->id_priorita), next_event->id_paziente);
    } else if(next_event->evento == TRASFERIMENTO) {
        printf("ospedale %d->%d (paziente %lu)\n", next_event->id_ospedale_partenza, next_event->id_ospedale_destinazione, next_event->paziente_trasferito->id);
    } else if(next_event->evento == AGGIORNAMENTO) {
        printf("calcolo nuovo flusso covid\n");
    }
    printf("\t\t\ttempo attuale:    %f\n", tempo_attuale);
    printf("\t\t\ttempo next event: %f\n", next_event->tempo_ne);
}

void step_simulazione(_ospedale* ospedale, int num_ospedali, double tempo_attuale, descrittore_next_event* next_event, _trasferimento* testa_trasferiti, int step_forzato) {

    if(tempo_attuale < tempo_stop && step_forzato == 1) { // fintanto che il tempo attuale non raggiunge il tempo di stop si va avanti veloce
        return;
    }

    stampa_simulazione_attuale(ospedale, num_ospedali, tempo_attuale, next_event);

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
                else if(comando[0] == 'c') // stampa situazione attuale delle code
                    stampa_stato_code(ospedale, num_ospedali);
                else if(comando[0] == 's') // ristampa dati evento attuale
                    stampa_simulazione_attuale(ospedale, num_ospedali, tempo_attuale, next_event);
                else if(comando[0] == 't') // stampa trasferimenti in corso
                    stampa_trasferimenti(testa_trasferiti);

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