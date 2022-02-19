#define START 0					// inizio all'ora 0

#ifdef BATCH
#define TICK_END 200*100*100*100        // numero di eventi massimi per la simulazione batch
#define END 18446744073709551615LU 		// valore irraggiungibile
#define INF 18446744073709551615LU 		// valore irraggiungibile
#else
#define END 24*429				// termine simulazione al giorno x
#define INF 100*END			    // tempo irraggiungibile
#endif

#define NOSPEDALI 2             // numero di ospedali nella simulazione

#define COVID 0					// Tipo 1
#define NCOVID 1				// Tipo 2

#define TRASFERITO 0            // tipi di ingresso nelle code
#define DIRETTO 1

#define NTYPE 2					// Numero tipi
#define NRFIELD 0				// Numero del campo "ingressi_terapia_intensiva"
#define SAMPLINGRATE 24.0

#define NCOLONNECODE 15
#define NCOLONNEREPARTI 4

#define NCODECOVID 3
#define NCODENCOVID 2

#define MAXNSIMULATION STREAMS

#ifdef BATCH
#define MAX_BATCH 5100
#endif
#define BATCH_SCARTATI 1

char* colonne_dati_code[] = {
                "accessi_normali","accessi_altre_code","accessi_altri_ospedali",
                "usciti_serviti","usciti_morti", "usciti_aggravati",
                "permanenza_serviti", "permanenza_morti", "permanenza_aggravati",
                "tipo", "pazienti medi", "varianza num pazienti",
                "attesa media", "varianza attesa", "tempo simulazione"};
char* colonne_dati_reparti[] = {
                "tempo_occupazione","num_entrati",
                "num_usciti", "utilizzazione"};
