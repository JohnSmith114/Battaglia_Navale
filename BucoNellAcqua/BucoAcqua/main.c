#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
Battaglia Navale by Kien Tuon Truong, Started rules at 18:15 08/12/17, Started coding at 18:49 08/12/17

REGOLE E RICHIESTE
1.	Niente variabili globali
2.	Dimensione della plancia di gioco 8x8, con N=8 in define
3.	Controllare per collisioni delle navi e che esse rientrino nella plancia di gioco
4.	Poter stampare la plancia di gioco con le navi, con '.' come acqua e '@' come nave. Inoltre si stampino anche le coordinate ai lati della plancia.
5.	Poter stampare la plancia di gioco in modalità attacco, con '.' come acqua, 'x' come punto colpito, 'o' come colpo mancato.
6.	Queste dovranno essere variabili in modo da poter essere modificate facilmente
7.	Le navi possono essere posizionate in diagonale
8.	Poter generare casualmente una mappa con delle navi già incluse. A quel punto si chieda se voler giocare in modalità "alleato" e mostrare come nel punto 4 o in modalità "guerra"
9.	Nella modalità guerra si stampi la mappa con le navi nascoste e si chiedano le coordinate per colpire. Ristampare di nuovo la mappa aggiornata. Il gioco termina quando tutte le navi sono affondate
10.	Alla fine stampare il numero di colpi sparati.
11.	Esistono più tipi di nave, ora descritti secondo lo schema <numero>*<nome>(<lunghezza>).
		2xMotovedetta(1)
		2xFregata(2)
		1xIncrociatore(3)
		1xPortaerei(5)
		Dare un nome a ogni nave
12.	Anche le coordinate richieste per colpire (Punto 9) vanno verificate e devono essere nei confini.
12b.	Quando si scelgono coordinate già colpite, si richiedano nuove coordinate.
13.	Alla fine della partita mostrare le statistiche di gioco
14. 	Modalità "I'm Feeling Lucky", dare tanti colpi quanti sono i punti "colpibili" delle navi (Somma di tutte le lunghezze). Il gioco finisce quando finiscono i colpi.
15.	Alla fine del gioco chiedere all'utente se vuole ricominciare il gioco
16.	Modalità in cui il giocatore posiziona le navi e il computer cerca di affondarle. Il gioco funziona come se il computer fosse il giocatore, con statistiche e tutto.
17.	Definire il concetto di "Giocatore" e fare in modo che sia un gioco completo VS AI
*/


/*
IDEA SPACE:
- Usare i piani come int, in modo da avere molti valori facilmente settabili. Mediante un dizionario poi si convertono i numeri in caratteri
- Draft per gli status: 0 acqua (o anche un pezzo di nave offuscata), 1 nave, 2 nave affondata, -1 colpo mancato
- I piani con le effettive navi non andranno toccate dopo l'inizializzazione/piazzamento delle navi




*/

#define N 8
#define MAXPLACEMENTATTEMPTS 512

typedef struct xy{
	unsigned int x;
	unsigned int y;
}coords_t;

typedef struct deltaxy{
    int deltax;
    int deltay;
}displacement_t;

typedef struct ship{
	char name[20];
	int length;
	int number;
	char desc[500];
}ship_t;

typedef struct player{
    char nickname[20];
    int nPlayer;
    int shipPlane[N][N];
    int visionPlane[N][N];
    char playerType;    /* 'a' = AI, 'h' = Human */
}player_t;

const char alphabetRef[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

const displacement_t cardinalSet[8] = {
                                        /*
                                            Controintuitivamente la direzione positiva è verso il basso,
                                            è un piano cartesiano ma ribaltato rispetto all'asse delle x
                                            Quindi queste direzioni sono riferite al tipico piano cartesiano,
                                            ma le coordinate saranno calcolate sottraendo le delta y, non sommandole.
                                        */

                                        {.deltax = 1,   .deltay = 0     },  /*  W   */
                                        {.deltax = 1,   .deltay = 1    },  /*  N-W */
                                        {.deltax = 0,   .deltay = 1    },  /*  N   */
                                        {.deltax = -1,  .deltay = 1    },  /*  N-E */
                                        {.deltax = -1,  .deltay = 0     },  /*  E   */
                                        {.deltax = -1,  .deltay = -1     },  /*  S-E */
                                        {.deltax = 0,   .deltay = -1     },  /*  S   */
                                        {.deltax = 1,   .deltay = -1     }   /*  S-W */
                                      };

const ship_t defaultNavy[4] = {
                                    {.name="Motovedetta", .length=1, .number=2, .desc="Sentinella dei mari, vitale per qualsiasi flotta per tenere il vantaggio strategico scoprendo le tattiche avversarie"},
                                    {.name="Fregata", .length=2, .number=2, .desc="Veloce e manovrabile, le fregate sono spesso utilizzate come scorta per navi più grosse. Questo però non deve farle sembrare deboli, una flotta di molte fregate è in grado di assaltare il nemico prima che se lo aspetti"},
                                    {.name="Incrociatore", .length=3, .number=1, .desc="L'incrociatore è l'arma pesante della marina militare, in grado di colpire più bersagli contemporaneamente, è in grado di cambiare le sorti di una battaglia rapidamente"},
                                    {.name="Portaerei", .length=5, .number=1, .desc="Seppur meno armata delle sue navi compagne, è di importanza strategica massima: una singola portaerei ben equipaggiata può sfoderare un bombardamento aereo in grado di concludere vittoriosamente il conflitto senza supporto ulteriore."}
                              };

void returnError(int type){
    system("clear");    /*  LINUX   */
    system("cls");      /*  WIN     */
    switch(type){
        case 0:
            printf("Il generatore casuale ha effettuato troppi tentativi\n\n");
            printf("Questo puo' essere causato dall'utilizzo di una flotta con troppe navi o navi troppo lunghe\n");
            printf("Oppure questo errore può essere capitato per pura sfortuna.\n\n");
            break;
        case 1:
            printf("La stringa inserita supera la dimensione limite\n\n");
            break;
        default:
            printf("In qualche maniera è uscito questo errore, non dovrebbe mai uscire. Kien, se leggi questo ricontrolla il tuo codice, nabbo.");
    };
}

int getStringInput(char *outputString, size_t length){
    /*  Prende una stringa di lunghezza length, la mette in outputString e cancella il \n finale. Inoltre ne verifica la lunghezza    */
    fgets(outputString, length, stdin);
    char *pos;
    if((pos=strchr(outputString, '\n'))!=NULL){
        *pos='\0';
        return 1;
    }else{
        returnError(1);
        return 0;
    }
}

void printFleet(ship_t fleet[], int fleetSize){ /*DEBUG: Stampa una flotta, cioè la lista di modelli di navi*/
    int i;
    for(i=0; i<fleetSize; i++){
        printf("Name: %s\nDescription: %s\nLength: %d\nQuantity in Fleet: %d\n\n", fleet[i].name, fleet[i].desc,fleet[i].length,fleet[i].number);
    }
}

int toInt(char c){
	return c-'0';
}

void printPlane(int plane[N][N]){   /*Self-explanatory*/
    int i,j;
    printf("   ");
    for(i=0;i<N;i++){
        printf("[%d]", i);
    }
    printf("\n");
    for(i=0;i<N;i++){
        printf("[%c]", i+65);   /*Dalla tabella ASCII: A=65, B=66 ecc...*/
        for(j=0;j<N;j++){
            if(plane[i][j]!=0){
                printf("(%d)", plane[i][j]);
            }else{
                printf(" %d ", plane[i][j]);
            }
        }
        printf("\n");
    }
}

void initEmptyPlane(int plane[N][N]){   /*Riempie il piano di 0*/
    int i,j;
    for(i=0;i<N;i++){
        for(j=0;j<N;j++){
            plane[i][j]=0;
        }
    }
}

coords_t getDisplacedPoint(coords_t startPoint, displacement_t displacement){
    /*Dato un punto e un vettore spostamento (Definito dalle sue componenti) restituisce la posizione finale    */
    coords_t newPoint = startPoint;
    newPoint.x += displacement.deltax;
    newPoint.y -= displacement.deltay;
    return newPoint;
}

int isValidCellForPlacement(coords_t cell, int plane[N][N]){
    /*Returna 0 se non è valida, 1 se lo è*/
    if(cell.x < 0 || cell.y<0){
        return 0;
    }
    if(cell.x > N-1 || cell.y > N-1){
        return 0;
    }
    if(plane[cell.y][cell.x]!=0){
        return 0;
    }
    return 1;
}

displacement_t getScalarMultVector(displacement_t displacement, int scalar){
    /*Prodotto di un vettore per uno scalare*/
    displacement_t newDisplacement;
    newDisplacement.deltax = displacement.deltax * scalar;
    newDisplacement.deltay = displacement.deltay * scalar;
    return newDisplacement;
}

int collisionCheck(coords_t startPoint, displacement_t propagationDir, int length, int plane[N][N]){
    /*Dato un piano, a partire dal punto di inizio (startPoint) controlla se tutte le "length" celle sono valide nella direzione della propagazione    */
    /*1 se il collision check non ha trovato collisioni, 0 se invece ci sono collisioni */
    int i;
    for(i=0 ;i<length; i++){
        if(isValidCellForPlacement(getDisplacedPoint(startPoint, getScalarMultVector(propagationDir, i)), plane)==0){
            return 0;
        }
    }
    return 1;
}

int insertShip(coords_t startPoint, displacement_t propagationDir, int length, int plane[N][N], int fillNumber){
    /* Returna 1 se l'inserimento è riuscito, 0 altrimenti  */
    if(collisionCheck(startPoint, propagationDir, length, plane)==0){
        return 0;
    }
    int i;
    for(i=0; i<length; i++){
        coords_t currCell = getDisplacedPoint(startPoint, getScalarMultVector(propagationDir, i));
        plane[currCell.y][currCell.x] = fillNumber;
    }
    return 1;
}

void fillRandomShips(int typeNumber, ship_t fleet[], int plane[N][N]){
    int shipsForType = 0, currType = 0, attempts = 0, shipNumber=1;
    while(currType<typeNumber){
        while(shipsForType<fleet[currType].number){
            coords_t startPoint;
            startPoint.x = rand()%N;
            startPoint.y = rand()%N;
            displacement_t direction;
            direction = cardinalSet[rand()%8];
            if(insertShip(startPoint, direction, fleet[currType].length, plane, shipNumber)==1){
                /*printf("VALID SHIP INSERTED start: (%d; %d), direction: (%d, %d), length: %d\n", startPoint.x, startPoint.y, direction.deltax, direction.deltay, fleet[currType].length); */
                shipsForType++;
                shipNumber++;
            }
            attempts++;
            if(attempts>MAXPLACEMENTATTEMPTS){
                /*returnError(0);*/
                return;
            }
        }
        shipsForType = 0;
        currType++;
    }
}

void initializeGame(player_t *p1, player_t *p2){
    printf("INIZIALIZZAZIONE\n\n________________________\n\n");
    do{
        printf("Inserisci il nome del primo giocatore: ");
    }while(getStringInput(&(p1->nickname), 20)!=1);

    printf("\nInserisci il nome del secondo giocatore: ");
    getStringInput(&(p2->nickname), 20);
    srand(time(NULL));
}

int isValidHit(coords_t location, int planeToHit[N][N], int planeVision[N][N]){
    /*  1 se ha colpito, 0 se non ha colpito, -1 se ha già colpito precedentemente              */
        if(planeVision[location.y][location.x]==2||planeVision[location.y][location.x]==-1){
            return -1;
        }else if(planeToHit[location.y][location.x]==0){
            return 0;
        }else{
            return 1;
        }
}

int hitAttempt(coords_t hitLocation, player_t attackPlayer, player_t hitPlayer){
    /*  0 se non è un bersaglio valido (già colpito), 1 se ha colpito qualcosa (acqua o nave)    */
    if(isValidHit(hitLocation, hitPlayer.shipPlane, attackPlayer.visionPlane)==-1){
        return 0;
    }else{
        return 1;
    }
}

void update(){
	/*Update for hit*/
}

int main(){
    player_t player1;
    player_t player2;

	/*printFleet(defaultNavy, 4);*/

    initializeGame(&player1, &player2);
    printf("Nome1: %s, Nome2: %s\n\n", player1.nickname, player2.nickname);

	initEmptyPlane(player1.shipPlane);
	initEmptyPlane(player1.visionPlane);
	initEmptyPlane(player2.shipPlane);
	initEmptyPlane(player2.visionPlane);

	fillRandomShips(4, defaultNavy, player2.shipPlane);
	printPlane(player2.shipPlane);

	return 0;
}

