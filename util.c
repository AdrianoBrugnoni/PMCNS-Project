/****Variabili globali csv manager*****/
enum { NOMEM = -2 };
static char* line = NULL;  /* caratteri input */
static char* sline = NULL;  /* copia linea per split */
static int  maxline = 0;
static char** field = NULL;
static int  maxfield = 0;
static int  nfield = 0;
static char fieldsep[] = ";"; // separatore dei campi
static char* nextsep(char*);
static int split(void);
/*************************************/

int min(int a, int b) {
    if (a > b)
        return b;
    else
        return a;
}

int tipo_opposto(int tipo) {
    if(tipo == COVID)
        return NCOVID;
    else
        return COVID;
}

int tipo_esiste(int tipo) {
    if(tipo == COVID || tipo == NCOVID)
        return 0;
    else
        return 1;
}

char int_to_char(int val) {
    return val + '0';
}

int genera_csv(char** colonne,int num_colonne) {
    int fd;
    int char_index;
	int ret;
    fd = open("output.csv", O_TRUNC|O_RDWR|O_CREAT, 0666);
#ifdef MSEXEL
	if ((ret = write(fd, "sep=,\n", 6)) == -1) {
		exit(0);
	}

#endif
    for (int i = 0; i < num_colonne; i++) {
        char_index = 0;
        while (colonne[i][char_index] != '\0') {
			if ((ret = write(fd, (char*)&colonne[i][char_index], 1)) == -1) {
				exit(0);
			}
            char_index++;
        }
		if ((ret = write(fd, ",", 1)) == -1) {
			exit(0);
		}
    }
	if ((ret = write(fd, "\n", 1)) == -1) {
		exit(0);
	}
    return 0;
}

int riempi_csv(char *** elemento, int num_righe, int num_colonne) {
    int fd;
    int char_index;
    fd = open("output.csv", O_RDWR | O_APPEND, 0666);

    for (int i = 0; i < num_colonne; i++) {
        for (int j = 0; j < num_righe; j++) {
            j = 0;
            while (elemento[i][j][char_index] != '\0') {
				if ((ret = write(fd, (char*)&elemento[i][j][char_index], 1)) == -1) {
					exit(0);
				}
                char_index++;
            }
			if ((ret = write(fd, ",", 1)) == -1) {
				exit(0);
			}
        }
		if ((ret = write(fd, "\n", 1)) == -1) {
			exit(0);
    }
    return 0;
}

static int endofline(FILE* fin, int c) {
	int eol;

	eol = (c == '\r' || c == '\n');
	if (c == '\r') {
		c = getc(fin);
		if (c != '\n' && c != EOF)
			ungetc(c, fin);
	}
	return eol;
}

static void reset(void) {
	free(line);
	free(sline);
	free(field);
	line = NULL;
	sline = NULL;
	field = NULL;
	maxline = maxfield = nfield = 0;
}

char* csvgetline(FILE* fin) {
	int i, c;
	char* newl, * news;

	if (line == NULL) {
		maxline = maxfield = 1;
		line = (char*)malloc(maxline);
		sline = (char*)malloc(maxline);
		field = (char**)malloc(maxfield * sizeof(field[0]));
		if (line == NULL || sline == NULL || field == NULL) {
			reset();
			return NULL;
		}
	}
	for (i = 0; (c = getc(fin)) != EOF && !endofline(fin, c); i++) {
		if (i >= maxline - 1) {
			maxline *= 2;
			newl = (char*)realloc(line, maxline);
			if (newl == NULL) {
				reset();
				return NULL;
			}
			line = newl;
			news = (char*)realloc(sline, maxline);
			if (news == NULL) {
				reset();
				return NULL;
			}
			sline = news;


		}
		line[i] = c;
	}
	line[i] = '\0';
	if (split() == NOMEM) {
		reset();
		return NULL;
	}
	return (c == EOF && i == 0) ? NULL : line;
}

// Divisione delle svariate colonne
static int split(void) {
	char* p, ** newf;
	char* sepp;
	int sepc;

	nfield = 0;
	if (line[0] == '\0')
		return 0;
	strcpy(sline, line);
	p = sline;

	do {
		if (nfield >= maxfield) {
			maxfield *= 2;
			newf = (char**)realloc(field,
				maxfield * sizeof(field[0]));
			if (newf == NULL)
				return NOMEM;
			field = newf;
		}
		if (*p == '"')
			sepp = nextsep(++p);
		else
			sepp = p + strcspn(p, fieldsep);
		sepc = sepp[0];
		sepp[0] = '\0';
		field[nfield++] = p;
		p = sepp + 1;
	} while (sepc == ';');

	return nfield;
}


static char* nextsep(char* p) {
	int i, j;

	for (i = j = 0; p[j] != '\0'; i++, j++) {
		if (p[j] == '"' && p[++j] != '"') {
			int k = strcspn(p + j, fieldsep);
			memmove(p + i, p + j, k);
			i += k;
			j += k;
			break;
		}
		p[i] = p[j];
	}
	p[i] = '\0';
	return p + j;
}

char* csvfield(int n) {
	if (n < 0 || n >= nfield)
		return NULL;
	return field[n];
}

int csvnfield(void) {
	return nfield;
}

int estrai_ricoveri_giornata(int giornata) {
	int index = 0;	
	FILE* csv = fopen("covid_lazio.csv", "r");
	csvgetline(csv);	// Skip intestazione
	while ((line = csvgetline(csv)) != NULL 
			&& index != giornata)
		index++;
	return atoi(csvfield(NRFIELD));
}
