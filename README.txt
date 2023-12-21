Programme de démonstration de MinMax avec élagage alpha/bêta pour le jeu d'échecs
A utiliser dans le module CRP/2CS-SID (Hidouci W.K. - ESI - 2023/2024)


Dans la version actuelle, le programme comporte une implémentation de "MinMax avec élagage alpha/bêta", les règles (presque complètes) du jeu d'échecs et quelques fonctions d'estimations utilisées pour l'évaluation des configurations situées à la frontière de l'exploration.

"MinMax" est un algorithme de type 'parcours en profondeur' utilisé dans l'exploration des espaces de recherche.
Des coupes de branches sont effectuées afin de rendre l'exploration plus rapide (c'est "l'élagage alpha/bêta").
Cette procédure est paramétrée principalement par: 
(i)  la profondeur max de chaque parcours, 
(ii) la largeur max à considérer à chaque coup et la fonction d'estimation utilisée pour l'évaluation des configurations de la frontière d'exploration.
Pour diminuer "l'effet horizon", la stabilité des configurations de la frontière d'exploration est prise en considération pour décider s'il faut continuer l'exploration de quelques niveaux supplémentaires ou non.
Dans cette version, une configuration est considérée comme étant "stable" si le nombre de pièces présentes est égal à celui de la configuration parante (c-a-d pas de captures effectuées).

Afin d'augmenter les opportunités de coupes de branches, un tri des différents successeurs à évaluer est effectué:
Pour chaque coup généré (dans la fonction main), les successeurs sont d'abord évalués avec une petite exploration de hauteur faible h0 (par exemple 2) afin d'obtenir une première estimation de chacune d'elles. Ces estimations sont ensuite utilisées pour ordonner les configurations filles, avant d'être évaluées avec la profondeur (hauteur) voulue. Cet ordre d'évaluation des alternatives devrait augmenter les coupes de branches (si les estimations utilisées sont de bonnes qualités). 
Au niveau de la fonction minmax_ab, il y a aussi un tri des successeurs avant leurs exploration, mais ce tri se base uniquement sur les estimations (c'est equivalent a une exploration avec une hauteur 0).

Les principales fonctions sont:
- minmax_ab(...)  	// MinMax avec élagage Alpha-Bêta.
- generer_succ(...)  	// Génère les successeurs d'une configuration donnée.
- estim*(...)  		// Les fonctions d'estimations pouvant être utilisées dans minmax_ab(...)

------------------------------------------------------------------------


Explication de quelques variables globales dans le source :

int h0 = 2;
Représente la profondeur de la pré-exploration (par exemple 2)
Avant d'explorer les successeurs de la configuration courante en utilisant MinMax avec la profondeur souhaité, on lance d'abors MinMax avec une petite profondeur (h0) afin d'obtenir une première estimation permettant de trier les successeurs du plus prometteur au moins prometteur. L'objectif est de maximiser le nombre coupes alpha et beta lors de l'exploration normale de ces successeurs avec la profondeur souhaitée.

struct config Partie[MAXPARTIE];
Un tableau stockant le déroulement d'une partie (les MAXPARTIE derniers coups).
Il est utilisé pour détecter si une configuration a déjà été générée, pour ne pas la considérer une 2e fois.

int num_coup = 0;
Le numéro du coup actuel.
Il est incrémenté à chaque tour.

int (*Est[10])(struct config *);
Un tableau de pointeurs de fonctions retournant un entier (int) et ayant comme paramètre une configuration.
Ce tableau est utilisé dans la fonction main() pour stocker les différentes fonctions d'estimations disponibles et permettre à l'utilisateur de choisir quelle fonction associer à chaque adversaire (Noir et Blanc)
Le tableau 'Est' est aussi utilisé dans la fonction minmax_ab() pour appeler la bonne fonction d'estimation quand la profondeur souhaitée est atteinte (c-a-d au niveau des configurations de la frontière d'exploration, quand 'niv' atteint 0)

int nbEst;
C'est le nombre de fonctions d'estmations dans le tableau 'Est' (normalement < 10 sinon s'il y a plus de 10 fonctions d'estimations augmenter la taille du tableau 'Est' en conséquence)

------------------------------------------------------------------------


Toutes les fonctions du programme :

/* MinMax avec élagage alpha-beta :
 Evalue la configuration 'conf' du joueur 'mode' en descendant de 'niv' niveaux.
 Le paramètre 'niv' est decrémenté à chaque niveau (appel récursif).
 'alpha' et 'beta' représentent les bornes initiales de l'intervalle d'intérêt 
 (pour pouvoir effectuer les coupes alpha et bêta).
 'largeur' représente le nombre max d'alternatives à explorer en profondeur à chaque niveau.
 Si 'largeur == +INFINI' toutes les alternatives seront prises en compte 
 (c'est le comportement par défaut).
 'numFctEst' est le numéro de la fonction d'estimation à utiliser lorsqu'on arrive à la
 frontière d'exploration (c-a-d 'niv' atteint 0)
 npp : le nombre de pieces dans la conf parente (pour verifier si conf est stable ou non) 
*/
int minmax_ab( struct config *conf, int mode, int niv, int alpha, int beta, int largeur, int numFctEst, int npp )


/* Teste si conf représente une fin de partie et retourne dans 'cout' la valeur associée */
int feuille( struct config *conf, int *cout )

/* Quelques exemples de fonctions d'estimation simples (estim1, estim2, ...) */
int estim1( struct config *conf )
int estim2( struct config *conf )
...


/* Génère, pour le joueur 'mode', les successeurs de la configuration 'conf' dans le tableau 'T', 
   retourne aussi dans 'n' le nombre de configurations filles générées */
void generer_succ( struct config *conf, int mode, struct config T[], int *n )

/* Génère dans T les configurations obtenues à partir de conf lorsqu'un pion (a,b) va atteindre 
   la limite de l'échiquier: pos (x,y)  */
void transformPion( struct config *conf, int a, int b, int x, int y, struct config T[], int *n )

/* Génere dans T tous les coups possibles de la pièce (de couleur N ou B) se trouvant à la pos x,y */
void deplacementsN(struct config *conf, int x, int y, struct config T[], int *n )
void deplacementsB(struct config *conf, int x, int y, struct config T[], int *n )


// Vérifie si la case (x,y) est menacée par une des pièces du joueur 'mode'
int caseMenaceePar( int mode, int x, int y, struct config *conf )

/* Fonctions de comparaison utilisées avec qsort     */
// 'a' et 'b' sont des configurations
int confcmp123(const void *a, const void *b)	// pour l'ordre croissant
int confcmp321(const void *a, const void *b)	// pour l'ordre decroissant

/* Le nombre de pièces de N et B dans l'echiquier 'conf' */
int npieces( struct config *conf )

/* Sauvegarder conf dans le fichier f (pour l'historique) */
void sauvConf( struct config *conf )

/* Intialise la disposition des pieces dans la configuration initiale conf */
void init( struct config *conf )

/* génère un texte décrivant le dernier coup effectué (pour l'affichage) */
void formuler_coup( struct config *oldconf, struct config *newconf, char *coup )

/* Affiche la configuration conf */
void affich( struct config *conf, char *coup, int num )

/* Copie la configuration c1 dans c2  */
void copier( struct config *c1, struct config *c2 ) 

/* Teste si les échiquiers c1 et c2 sont égaux */
int egal(char c1[8][8], char c2[8][8] )

/* Teste si conf a déjà été jouée (dans le tableau partie) */
int dejaVisitee( struct config *conf )

/* Teste s'il n'y a aucun coup possible dans la configuration conf (non encore implémentée) */
int AucunCoupPossible( struct config *conf )

------------------------------------------------------------------------


La boucle principale dans la fonction main

// initialisations....
h0 , h , Largeur
tour = MAX
while ( non fin de partie ) {
    Afficher la config courante : conf
    if ( tour == MAX )
	if ( joueur_humain )
	   Saisir le coup à jouer ==> newConf
	else
	   Generer les n successeurs de conf dans le tableau T
	   Explorer les n suuccesseurs T[i] avec une petite profondeur h0
	   Trier les n successeurs T[i] en ordre décroissant des estimations retournées
	   n = min( n , Largeur d'exploration)
	   Explorer les successeurs T[i] (dans l'ordre décroissant) avec une profondeur h
	   ==> soit newConf le meilleur successeur (ayant la plus grande estimation)
    else // c-a-d tour == MIN
	if ( joueur_humain )
	   Saisir le coup à jouer ==> newConf
	else
	   Generer les n successeurs de conf dans le tableau T
	   Explorer les n suuccesseurs T[i] avec une petite profondeur h0
	   Trier les n successeurs T[i] en ordre croissant des estimations retournées
	   n = min( n , Largeur d'exploration)
	   Explorer les successeurs T[i] (dans l'ordre croissant) avec une profondeur h
	   ==> soit newConf le meilleur successeur (ayant la plus petite estimation)
	
    conf = newConf
    tour = (tour == MAX ? MIN : MAX )
}


