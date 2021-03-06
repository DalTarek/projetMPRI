/*
	Canvas pour algorithmes de jeux à 2 joueurs

	joueur 0 : humain
	joueur 1 : ordinateur

*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

// Paramètres du jeu
#define LARGEUR_MAX 7 		// nb max de fils pour un noeud (= nb max de coups possibles)
#define TEMPS 10		// temps de calcul pour un coup avec MCTS (en secondes)
#define LIGNE 6
#define COL 7
#define C sqrt(2)

// macros
//0 : humain, 1 : ordinateur
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

// Critères de fin de partie
typedef enum {NON, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {

	int joueur; // à qui de jouer ?

    char plateau[6][7];

} Etat;

// Definition du type Coup
typedef struct {

	int colonne;

} Coup;

// Copier un état
Etat * copieEtat( Etat * src ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	etat->joueur = src->joueur;

	int i,j;
	for (i=0; i< LIGNE; i++)
		for ( j=0; j<COL; j++)
			etat->plateau[i][j] = src->plateau[i][j];

	return etat;
}

// Etat initial
Etat * etat_initial( void ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	int i,j;
	for (i=0; i< LIGNE; i++)
		for ( j=0; j<COL; j++)
			etat->plateau[i][j] = ' ';

	return etat;
}


void afficheJeu(Etat * etat) {

	int i,j;
	printf("   |");
	for ( j = 0; j < COL; j++)
		printf(" %d |", j);
	printf("\n");
	printf("--------------------------------");
	printf("\n");

	for(i=0; i < LIGNE; i++) {
		printf(" %d |", i);
		for ( j = 0; j < COL; j++)
			printf(" %c |", etat->plateau[i][j]);
		printf("\n");
		printf("--------------------------------");
		printf("\n");
	}
}


// Nouveau coup
Coup * nouveauCoup( int j ) {
	Coup * coup = (Coup *)malloc(sizeof(Coup));

	coup->colonne = j;

	return coup;
}

// Demander à l'humain quel coup jouer
Coup * demanderCoup () {

	int j;
	printf(" quelle colonne ? ") ;
	scanf("%d",&j);

	return nouveauCoup(j);
}

// Modifier l'état en jouant un coup
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {

	int i = LIGNE - 1;
	while(i >= 0){
        if ( etat->plateau[i][coup->colonne] == ' ' ){
            etat->plateau[i][coup->colonne] = etat->joueur ? 'O' : 'X';

            // à l'autre joueur de jouer
            etat->joueur = AUTRE_JOUEUR(etat->joueur);
            return 1;
        }
        i--;
	}
    return 0;
}

// Retourne une liste de coups possibles à partir d'un etat
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {

	Coup ** coups = (Coup **) malloc((1+LARGEUR_MAX) * sizeof(Coup *) );

	int k = 0;

	int j;
	for(j = 0; j < COL; j++){
        if ( etat->plateau[0][j] == ' ' ) {
            coups[k] = nouveauCoup(j);
            k++;
        }
	}

	coups[k] = NULL;

	return coups;
}


// Definition du type Noeud
typedef struct NoeudSt {

	int joueur; // joueur qui a joué pour arriver ici
	Coup * coup;   // coup joué par ce joueur pour arriver ici

	Etat * etat; // etat du jeu

	struct NoeudSt * parent;
	struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
	int nb_enfants;	// nb d'enfants présents dans la liste

	// POUR MCTS:
	int nb_victoires;
	int nb_simus;

} Noeud;


// Créer un nouveau noeud en jouant un coup à partir d'un parent
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {
	Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));

	if ( parent != NULL && coup != NULL ) {
		noeud->etat = copieEtat ( parent->etat );
		jouerCoup ( noeud->etat, coup );
		noeud->coup = coup;
		noeud->joueur = AUTRE_JOUEUR(parent->joueur);
	}
	else {
		noeud->etat = NULL;
		noeud->coup = NULL;
		noeud->joueur = 0;
	}
	noeud->parent = parent;
	noeud->nb_enfants = 0;

	// POUR MCTS:
	noeud->nb_victoires = 0;
	noeud->nb_simus = 0;


	return noeud;
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
	Noeud * enfant = nouveauNoeud (parent, coup ) ;
	parent->enfants[parent->nb_enfants] = enfant;
	parent->nb_enfants++;
	return enfant;
}

void freeNoeud ( Noeud * noeud) {
	if ( noeud->etat != NULL)
		free (noeud->etat);

	while ( noeud->nb_enfants > 0 ) {
		freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
		noeud->nb_enfants --;
	}
	if ( noeud->coup != NULL)
		free(noeud->coup);

	free(noeud);
}


// Test si l'état est un état terminal
// et retourne NON, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {

	// tester si un joueur a gagné
	int i,j,k,n = 0;
	for ( i=0;i < LIGNE; i++) {
		for(j=0; j < COL; j++) {
			if ( etat->plateau[i][j] != ' ') {
				n++;	// nb coups joués

				// lignes
				k=0;
				while ( k < 4 && i+k < LIGNE && etat->plateau[i+k][j] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// colonnes
				k=0;
				while ( k < 4 && j+k < COL && etat->plateau[i][j+k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// diagonales
				k=0;
				while ( k < 4 && i+k < LIGNE && j+k < COL && etat->plateau[i+k][j+k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				k=0;
				while ( k < 4 && i+k < LIGNE && j-k >= 0 && etat->plateau[i+k][j-k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;
			}
		}
	}

	// et si match nul alors l'ordi gagne
	if ( n == COL*LIGNE )
		return ORDI_GAGNE;

	return NON;
}



// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat * etat, int tempsmax) {

	clock_t tic, toc;
	tic = clock();
	int temps;

	Coup ** coups;
	Coup * meilleur_coup ;

	// Créer l'arbre de recherche
	Noeud * racine = nouveauNoeud(NULL, NULL);
	racine->etat = copieEtat(etat);

	// créer les premiers noeuds:
	coups = coups_possibles(racine->etat);
	int k = 0;
	Noeud * enfant;
	while ( coups[k] != NULL) {
		enfant = ajouterEnfant(racine, coups[k]);
		k++;
	}

    /* début de l'algorithme  */

	int iter = 0;
    int nb_vict = 0;

    int i, bMaxIndice, vJoueur;
    float bMax, b;
    bool trouve;

	do {

        // Récursion jusqu'à trouver un état terminal ou un noeud avec aucun fils développé
        do {
            // Sélection du plus grand B-valeur si il existe
            bMax = -8000;
            vJoueur = 1;
            if(racine->joueur == 0){
                // humain
                vJoueur = -1;
            }
            trouve = false;
            for(i =0; i < racine->nb_enfants; i++){
                if(racine->enfants[i]->nb_simus > 0){
                    b = vJoueur * (((float)racine->enfants[i]->nb_victoires)/((float)racine->enfants[i]->nb_simus)) + C * sqrt(log((float)racine->nb_simus)/((float)racine->enfants[i]->nb_simus));
                    if(b > bMax){
                        bMax = b;
                        bMaxIndice = i;
                    }
                }else{
                    trouve = true;
                }
            }
            if(!trouve){
                racine = racine->enfants[bMaxIndice];
            }
        } while(testFin(racine->etat) == NON && !trouve);
        if(trouve && testFin(racine->etat) == NON){
            // Cas noeud où il existe fils non développé(s)
            // On choisit un fils aléatoirement jusqu'à arriver à un état terminal
            while(testFin(racine->etat) == NON){
                // créer les fils
                i = rand()%racine->nb_enfants;
                while(racine->enfants[i]->nb_simus > 0){
                    i = rand()%racine->nb_enfants;
                }
                racine = racine->enfants[i];
                coups = coups_possibles(racine->etat);
                k = 0;
                while ( coups[k] != NULL && testFin(racine->etat) == NON) {
                    ajouterEnfant(racine, coups[k]);
                    k++;
                }
            }
            // On répercute le résultat en fonction de qui a gagné
            if(testFin(racine->etat) == ORDI_GAGNE){
                while(racine->parent != NULL){
                    racine->nb_simus = racine->nb_simus + 1;
                    racine->nb_victoires = racine->nb_victoires + 1;
                    racine = racine->parent;
                }
                racine->nb_simus = racine->nb_simus + 1;
                racine->nb_victoires = racine->nb_victoires + 1;

                nb_vict++;
            }else{
                while(racine->parent != NULL){
                    racine->nb_simus = racine->nb_simus + 1;
                    racine = racine->parent;
                }
                racine->nb_simus = racine->nb_simus + 1;
            }
        }else{
            // Cas noeud terminal
            // On répercute le résultat en fonction de qui a gagné
            if(testFin(racine->etat) == ORDI_GAGNE){
                while(racine->parent != NULL){
                    racine->nb_simus = racine->nb_simus + 1;
                    racine->nb_victoires = racine->nb_victoires + 1;
                    racine = racine->parent;
                }
                racine->nb_simus = racine->nb_simus + 1;
                racine->nb_victoires = racine->nb_victoires + 1;

                nb_vict++;
            }else{
                while(racine->parent != NULL){
                    racine->nb_simus = racine->nb_simus + 1;
                    racine = racine->parent;
                }
                racine->nb_simus = racine->nb_simus + 1;
            }
        }
        // Sélection du plus grand B-valeur si il existe pour l'affectation du meilleur coup
        bMaxIndice = rand() % racine->nb_enfants;
        bMax = 0;
        for(i =0; i < racine->nb_enfants; i++){
            if(racine->enfants[i]->nb_simus > 0){
                b = (float)(racine->enfants[i]->nb_victoires/racine->enfants[i]->nb_simus) + C * sqrt(log(racine->nb_simus)/racine->enfants[i]->nb_simus);
                if(b > bMax){
                    bMax = b;
                    bMaxIndice = i;
                }
            }
        }
        //Affectation du meilleur coup
        meilleur_coup = racine->enfants[bMaxIndice]->coup;

		toc = clock();
		temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
		iter ++;
	} while ( temps < tempsmax );


	/* Regle Max */
    /*float recompense;
	float recompenseMax = 0.;
	int indiceRecompense;
	for (i = 0; i < racine->nb_enfants; i++) {
        recompense = (float)(racine->enfants[i]->nb_victoires) / (float)(racine->enfants[i]->nb_simus);
        if (recompense > recompenseMax) {
            recompenseMax = recompense;
            indiceRecompense = i;
        }
	}
    meilleur_coup = racine->enfants[indiceRecompense]->coup;*/


    /* Regle robuste */
    /*int nbSimus;
    int nbSimusMax = 0;
    int indiceSimus;
    for (i = 0; i < racine->nb_enfants; i++) {
        nbSimus = racine->enfants[i]->nb_simus;
        if (nbSimus > nbSimusMax) {
            nbSimusMax = nbSimus;
            indiceSimus = i;
        }
    }
    meilleur_coup = racine->enfants[indiceSimus]->coup;*/


	/* fin de l'algorithme  */

	// Jouer le meilleur premier coup
	jouerCoup(etat, meilleur_coup );

	printf("Nombre de simulations realisees = %i\n", iter);
	printf("Nombre de victoires = %i\n", nb_vict);
	printf("Probabilite de gagner = %f\n", ((float)nb_vict)/((float)iter));


	// Penser à libérer la mémoire :
	freeNoeud(racine);
	free (coups);
}

int main(void) {

	Coup * coup;
	FinDePartie fin;

	// initialisation
	Etat * etat = etat_initial();

	// Choisir qui commence :
	printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
	scanf("%d", &(etat->joueur) );

	// boucle de jeu
	do {
		printf("\n");
		afficheJeu(etat);

		if ( etat->joueur == 0 ) {
			// tour de l'humain

			do {
				coup = demanderCoup();
			} while ( !jouerCoup(etat, coup) );

		}
		else {
			// tour de l'Ordinateur

			ordijoue_mcts( etat, TEMPS );

		}

		fin = testFin( etat );
	}	while ( fin == NON ) ;

	printf("\n");
	afficheJeu(etat);

	if ( fin == ORDI_GAGNE )
		printf( "** L'ordinateur a gagne **\n");
	else
		printf( "** BRAVO, l'ordinateur a perdu  **\n");
	return 0;
}
