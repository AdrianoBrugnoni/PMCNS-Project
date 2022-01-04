flag_fissi=-DGEN_RT -DPROTOTIPO1
flag_normali=-DTERAPIE_VARIABILI -DFLUSSO_COVID_VARIABILE -DABILITA_AGGRAVAMENTO -DABILITA_TIMEOUT
nome_eseguibile=prog

ifeq ($(OS),Windows_NT)
	CFLAGS += -DWIN
	CC = cl
else ifeq ($(OS),Darwin)
	CFLAGS += -DMAC_OS
	CC = gcc -o $(nome_eseguibile)
	CCFLAGS += -lpthread -lm
else
	CC = gcc -o $(nome_eseguibile)
	CCFLAGS += -lpthread -lm
endif

compile:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) $(flag_normali) -DCOOPERAZIONE_OSPEDALI -DSIM_INTERATTIVA
semplice:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) -DSIM_INTERATTIVA
semplice_n:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi)
normale:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) $(flag_normali) -DSIM_INTERATTIVA
normale_n:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) $(flag_normali)
migliorativo:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) $(flag_normali) -DCOOPERAZIONE_OSPEDALI -DSIM_INTERATTIVA
migliorativo_n:
	$(CC) prog.c $(CCFLAGS) $(CFLAGS) $(flag_fissi) $(flag_normali) -DCOOPERAZIONE_OSPEDALI
stat:
	$(CC) teststat stat.c -DTESTSTAT