#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

// inicijalizacija promenljivih
sem_t s_empty_pot;
sem_t s_full_pot;
int num_savages = 0;
int num_meals=0;
static pthread_mutex_t m_servings, m_cook; // lonac i kuvar se cuvaju muteksom
static int servings = 0;
static int cook_speed = 1;
static int total_consumed = 0;

// podaci o divljaku
typedef struct SavageInfo{
    int eat_meals;
    int id;
}Savage;

int getServingsFromPot(void) { // funkcija ce vracati ili 0 ili 1
    
	// Lonac se ''zakljucava'' i nema vise hrane
    pthread_mutex_lock(&m_servings);
    
	// Povratna vrednost funkcije
    int can_serve;
   
	// ako je lonac prazan
    if (servings == 0) {
        
		// upozorava da se ne moze servirati porcija
        can_serve = 0;
    } else {
        
		// uzima se porcija
        servings--;
        
		// upozorava da je porcija uzeta
        can_serve = 1;
    }
    
	// oslobadja posudu
    total_consumed++;
	// oslobadja posudu
    pthread_mutex_unlock (&m_servings);
    return can_serve;
}

void putServingsInPot(int num) {
	// Lonac se ''zakljucava'' i nema vise hrane
    pthread_mutex_lock(&m_servings);
    // Proverava je li lonac prazan
    if(servings == 0) {
		//Dodaje zadatu kolicinu obroka u porciju
        servings += num;
    }
    // oslobadja posudu
    pthread_mutex_unlock(&m_servings);
}

void *cook (void *id) {
    int i;
    // petlja ide dok se ''program'' ne zavrsi
    while(1)
    {
        // Sacekaj da se lonac isprazni
        sem_wait(&s_empty_pot);
        // Kuvar (to je prevod)
        putServingsInPot(cook_speed); // onoliko puta koliko obroka kuvar moze skuvati odjednom
        // Kuvar je skuvao obroke
        printf("\nCook: cooked %d meals\n\n", servings);
        fflush(stdout);
        // Govori se koliko je jela skuvao
        for(i = 0;i<cook_speed;i++)   //  koliko kuvar obroka skuva odjednom
            sem_post(&s_full_pot);
    }
}

void *eat (void *id) {
    // Prima vrednosti koje ce biti koriscene u niti
    Savage s = *(Savage *)id;
    int eat_meals = s.eat_meals;
    int savage_id = s.id;
    // Dok divljaci jedu
    while (eat_meals) {
        // Ako ne mogu jesti (prazna cinija)
        if (!getServingsFromPot()) {
            // Budi se kuvar. UPOZORENJE: Kuvara budi iskljucivo jedan divljak
            pthread_mutex_lock(&m_cook);
            sem_post(&s_empty_pot);
            printf("Savage [%d]: I need food\n", savage_id);
            fflush(stdout);
            pthread_mutex_unlock(&m_cook);

            // Ceka se da se napuni cinija
            sem_wait(&s_full_pot);
        }
        eat_meals--;

        // Obavestava se da divljak jede
        printf ("Savage [%d]:  is eating\n", savage_id);
        fflush(stdout);

        // simulacija vremena potrebnog da se pojede obrok
        sleep(1);

        // Obavestava se da je divljak zavrsio i koliko mu je jos ostalo
        printf ("Savage [%d]: is DONE eating, need %d more\n", savage_id, eat_meals);
        fflush(stdout);
    }
    // Obavestava se da je divljak potpuno zavrsio i  da je sit.
    printf("\nSavage [%i]: FINISHED\n",savage_id);
    return NULL;
}

int main(int argc, char *argv[]) {
    /*
        Program prima:
         num_savages: Broj divljaka
         num_meals: Koliko obroka divljak jede
         servings: Inicijalni broj obroka u ciniji
         cook_speed: Koliko obroka kuvar kuva odjednom
    */
	if (argc < 4) {
		printf("invalid parametars");
		exit(0);
	}
   
    // Koristice se kod funkcije strtol
    char **p_end = NULL;
    // Broj porcija koji je inicijalno bio u ciniji
    servings = strtol(argv[3], p_end, 10);
    // Broj divljaka
    num_savages = strtol(argv[1], p_end, 10);

    printf("There are %d savages\n", num_savages);
    printf("That need to eat %d meals\n",(int)strtol(argv[2], p_end, 10));
    printf("The pot have %d initial meals\n", servings);
    if(argc > 4)
    {
        cook_speed = strtol(argv[4], p_end, 10);
        printf("The cook will cook %d meal per time\n", cook_speed);
    }

    num_meals=strtol(argv[2], p_end, 10);
    sleep(5);
    // Indeks koji se koristi za pravljenje divljaka (i onih koji kuvaju i onih koji jedu)
    int i;

    // Struktura koja cuva ID svakog divljaka
    // Broj obroka koje ce skuvati
    // Broj obroka koje ce pojesti
    Savage s[num_savages+1];

    // Niz niti sa brojem divljaka
    pthread_t tid[num_savages+1];

    // Inicijalizuje se mutex da osigura medjusobno iskljucenje u grupi divljaka.
    // U ''eat'' funkciji samo jedan
    // divljak moze uzeti hranu iz cinije u isto vreme
    pthread_mutex_init(&m_servings, NULL);

    // Inicijalizacija semafora, za sve divljake se smatra da spavaju
    sem_init(&s_empty_pot, 0, 0);
    sem_init(&s_full_pot,  0, 0);

    // Salje se broj divljaka koji bi da jedu
    for (i=0; i<num_savages; i++)
    {
        // Inicijalizuju se strukture i nit za divljake
        s[i].eat_meals = num_meals;
        s[i].id = i;
        pthread_create(&tid[i], NULL, eat, (void *)&s[i]);
    }
    // Kuvanje
	// nit za kuvara
    s[i].eat_meals = num_meals;
    s[i].id = i;
    pthread_create (&tid[i], NULL, cook, (void *)&s[i]);
    // Ceka se da svi zavrse sa jelom
    for (i=0; i<num_savages; i++) {
        pthread_join(tid[i], NULL);
    }
}
