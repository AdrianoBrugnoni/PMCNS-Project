/*PMMLCG Lehmer Generator*/

#include <stdio.h>
#include <time.h>
#include <math.h>

#define MOD    2147483647       /*Numero primo di Lehmer.
                                  Periodo di modulo (MOD-1)*/
#define MULTIPLIER 48271        /*g(x):= MULTIPLIER*x mod (MOD)*/
#define CHECK      399268537
#define STREAMS    256
#define A256       22925
#define DEFAULT    123456789    /*Seme di default*/

static long seed[STREAMS] = { DEFAULT };
static int  stream = 0;
static int  initialized = 0;

double Random(void);
void   PlantSeeds(long x);
void   GetSeed(long* x);
void   PutSeed(long x);
void   SelectStream(int index);
void   TestRandom(void);

double Random(void) {
    const long Q = MOD / MULTIPLIER;
    const long R = MOD % MULTIPLIER;
    long t;

    t = MULTIPLIER * (seed[stream] % Q) - R * (seed[stream] / Q);
    if (t > 0)
        seed[stream] = t;
    else
        seed[stream] = t + MOD;
    return ((double)seed[stream] / MOD);
}

void PlantSeeds(long x) {
    const long Q = MOD / A256;
    const long R = MOD % A256;
    int  j;
    int  s;

    initialized = 1;
    s = stream;
    SelectStream(0);
    PutSeed(x);
    stream = s;
    /*Distribuzione a distanza fissa dei semi*/
    for (j = 1; j < STREAMS; j++) { 
        x = A256 * (seed[j - 1] % Q) - R * (seed[j - 1] / Q);
        if (x > 0)
            seed[j] = x;
        else
            seed[j] = x + MOD;
    }
}

void PutSeed(long x) {
    char ok = 0;

    if (x > 0)
        x = x % MOD;    /*Data la linearità abbiamo che g(a*x)=a*g(x); 
                        Correzione per x troppo grandi */
    if (x < 0)
        x = ((unsigned long)time((time_t*)NULL)) % MOD;
    if (x == 0)
        while (!ok) {
            printf("\nEnter a positive integer seed (9 digits or less) >> ");
            scanf("%ld", &x);
            ok = (0 < x) && (x < MOD);
            if (!ok)
                printf("\nInput out of range ... try again\n");
        }
    seed[stream] = x;
}


void GetSeed(long* x) {
    *x = seed[stream];
}


void SelectStream(int index) {
    stream = ((unsigned int)index) % STREAMS;
    if ((initialized == 0) && (stream != 0))
        PlantSeeds(DEFAULT);
}


double exponential(double m) {
    return (-m * log(1.0 - Random()));
}

double uniform(double a, double b) {
    return (a + (b - a) * Random());
}

#ifdef TEST
void main() {
    PlantSeeds(112233445);
    SelectStream(2);
    
    printf("Uniform: %f\n", uniform(2, 10));
    printf("Exponential: %f\n", exponential(2));

    SelectStream(3);
    printf("Exponential: %f\n", exponential(2));
    printf("Uniform: %f\n", uniform(2, 10));
}
#endif