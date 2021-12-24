int min(int a, int b) {
    if (a > b)
        return b;
    else
        return a;
}

char int_to_char(int val) {
    return val + '0';
}

int genera_csv(char** colonne,int num_colonne) {
    int fd;
    int char_index;
    fd = open("output.csv", O_TRUNC|O_RDWR|O_CREAT, 0666);
#ifdef MSEXEL
    write(fd,"sep=,\n",6);
#endif
    for (int i = 0; i < num_colonne; i++) {
        char_index = 0;
        while (colonne[i][char_index] != '\0') {
            write(fd,(char *)&colonne[i][char_index], 1);
            char_index++;
        }
        write(fd, ",", 1);
    }
    write(fd, "\n", 1);
}

int riempi_csv(char *** elemento, int num_righe, int num_colonne) {
    int fd;
    int char_index;
    fd = open("output.csv", O_RDWR | O_APPEND, 0666);

    for (int i = 0; i < num_colonne; i++) {
        for (int j = 0; j < num_righe; j++) {
            j = 0;
            while (elemento[i][j][char_index] != '\0') {
                write(fd, (char*)&elemento[i][j][char_index], 1);
                char_index++;
            }
            write(fd, ",", 1);
        }
        write(fd, "\n", 1);
    }

}