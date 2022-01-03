flag_fissi=-DGEN_RT -lpthread -lm
flag_normali=-DTERAPIE_VARIABILI -DFLUSSO_COVID_VARIABILE -DABILITA_AGGRAVAMENTO -DABILITA_TIMEOUT
nome_eseguibile=prog

compile:
	gcc -o $(nome_eseguibile) prog.c $(CFLAGS) $(flag_fissi) $(flag_normali) -DCOOPERAZIONE_OSPEDALI -DSIM_INTERATTIVA
semplice:
	gcc -o $(nome_eseguibile) prog.c $(CFLAGS) $(flag_fissi) -DSIM_INTERATTIVA
semplice_n:
	gcc -o $(nome_eseguibile) prog.c $(CFLAGS) $(flag_fissi)
normale:
	gcc -o $(nome_eseguibile) prog.c $(CFLAGS) $(flag_fissi) $(flag_normali) -DSIM_INTERATTIVA
normale_n:
	gcc -o $(nome_eseguibile) prog.c $(CFLAGS) $(flag_fissi) $(flag_normali)
migliorativo:
	gcc -o $(nome_eseguibile) prog.c $(CFLAGS) $(flag_fissi) $(flag_normali) -DCOOPERAZIONE_OSPEDALI -DSIM_INTERATTIVA
migliorativo_n:
	gcc -o $(nome_eseguibile) prog.c $(CFLAGS) $(flag_fissi) $(flag_normali) -DCOOPERAZIONE_OSPEDALI
stat:
	gcc -o teststat stat.c -DTESTSTAT