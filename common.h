#define START 0					// inizio all'ora 0
#define END 24*90				// termine simulazione al giorno 90
#define INF 100*END			    // tempo irraggiungibile

#define COVID 0					// Tipo 1
#define NCOVID 1				// Tipo 2

#define NTYPE 2					// Numero tipi
#define NRFIELD 7				// Numero del campo "Nuovi Ricoveri"
#define SAMPLINGRATE 24.0

#define NCOLONNECODE 10
#define NCOLONNEREPARTI 3

#define MAXNSIMULATION STREAMS


thread_local char* colonne_dati_code[] = {
                "accessi_normali","accessi_altre_code","accessi_altri_ospedali",
                "usciti_serviti","usciti_morti", "usciti_aggravati",
                "permanenza_serviti", "permanenza_morti", "permanenza_aggravati", "tipo"};
thread_local char* colonne_dati_reparti[] = {
                "tempo_occupazione","num_entrati",
                "num_usciti"};