flag_fissi=-DGEN_RT
flag_normali=-DTERAPIE_VARIABILI -DFLUSSO_COVID_VARIABILE -DABILITA_AGGRAVAMENTO -DABILITA_TIMEOUT -DORGANIZZA_DIRECTORY
nome_eseguibile=prog

ifeq ($(OS),Windows_NT)
	CFLAGS += -DWIN
	CC = cl
else ifeq ($(shell uname),Darwin)
	CFLAGS += -DMAC_OS
	CC = gcc -o $(nome_eseguibile)
	CCFLAGS += -lpthread -lm
else ifeq ($(shell uname),Linux)
	CC = gcc -o $(nome_eseguibile)
	CCFLAGS += -lpthread -lm
endif

compile:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) $(flag_normali) -DCOOPERAZIONE_OSPEDALI -DSIM_INTERATTIVA
semplice:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) -DSIM_INTERATTIVA
semplice_n:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi)
semplice_batch:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) -DBATCH
normale:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) $(flag_normali) -DSIM_INTERATTIVA
normale_n:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) $(flag_normali)
global_only:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) -DTERAPIE_VARIABILI -DABILITA_AGGRAVAMENTO -DABILITA_TIMEOUT
migliorativo:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) $(flag_normali) -DCOOPERAZIONE_OSPEDALI -DSIM_INTERATTIVA
migliorativo_n:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) $(flag_normali) -DCOOPERAZIONE_OSPEDALI
stat:
	$(CC) teststat stat.c -DTESTSTAT
batch:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) -DTERAPIE_VARIABILI -DABILITA_AGGRAVAMENTO -DABILITA_TIMEOUT -DBATCH