compile:
	gcc -o prog prog.c -DSIM_INTERATTIVA -DTERAPIE_VARIABILI -DFLUSSO_COVID_VARIABILE -DCOPERAZIONE_OSPEDALI
stat:
	gcc -o teststat stat.c -DTESTSTAT