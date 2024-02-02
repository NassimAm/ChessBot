#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <time.h>
#include <limits.h>          // pour INT_MAX

#include "chess_bot/chess_engine/jeu.h"
#include "chess_bot/gui/gui.h"

/************************/
/* Variables Globales : */
/************************/

// Tableau de config pour garder la trace des conf déjà visitées
struct config Partie[MAXPARTIE];

// Fichier pour sauvegarder l'historique des parties
FILE *f;

// compteur de coups effectués
int num_coup = 0;          

// profondeur initiale d'exploration préliminaire avant le tri des alternatives
int h0 = 0;

// tableau de pointeurs de fonctions (les fonctions d'estimations disponibles)
int (*Est[10])(struct config *);

// nb de fonctions d'estimation dans le tableau précédent
int nbEst;

// pour statistques sur le nombre de coupes effectuées
int nbAlpha = 0;
int nbBeta = 0;

// indices des cases sélectionnées
int selectedX = -1;
int selectedY = -1;

// indices des cases sélectionnées
int moveToX = -1;
int moveToY = -1;

int main(int argc, char *argv[])
{
    int n, i, j, score, stop, cout, hauteur, largeur, tour, estMin, estMax, nbp;
    
    // Variables pour affichage
    int hauteurb, hauteurn, nb, nn, jb, jn, scoreb, scoren;
    hauteurb = 0;
    hauteurn = 0;
    nb = 0;
    nn = 0;
    jb = -1;
    jn = -1;
    scoreb = -INFINI;
    scoren = +INFINI;

    int cout2, legal;
    int cmin, cmax;
    int typeExec, refaire;

    char coup[20] = "";
    char pathf[100] = "records/";
    char nomf[50];  // nom du fichier de sauvegarde
    char ch[100];

    struct config T[100], conf, conf1;

    // initialiser le tableau des fonctions d'estimation
    Est[0] = estim1;
    Est[1] = estim2;
    Est[2] = estim3;
    Est[3] = estim4;
    Est[4] = estim5;
    Est[5] = estim6;
    Est[6] = estim7;
    // Nombre de fonctions d'estimation disponibles
    nbEst = 7;        

    // Choix du type d'exécution (pc-contre-pc ou user-contre-pc) ...
    printf("Type de parties (B:Blancs  N:Noirs) :\n");
    printf("1- PC(B)   contre PC(N)\n");
    printf("2- USER(N) contre PC(B)\n");
    printf("3- USER(B) contre PC(N)\n");
    printf("\tChoix : ");
    scanf(" %d", &typeExec);
    if (typeExec != 2 && typeExec != 3) typeExec = 1;

    // Choix des fonctions d'estimation à utiliser ...
    do {
        printf("\nLes fonctions d'estimations disponibles sont:\n");
        printf("1- basée uniquement sur le nombre de pièces\n");
        printf("2- basée sur le nb de pieces, l'occupation, la défense du roi et les roques\n");
        printf("3- basée sur le nb de pièces et une perturbation aléatoire\n");
        printf("4- basée sur le nb de pieces et les menaces\n");
        printf("5- basée sur le nb de pieces et l'occupation\n");
        printf("6- basée sur une combinaisant de 3 estimations: (2 -> 5 -> 4)\n");
        printf("7- une fonction d'estimation aléatoire (ou à définir) \n\n");
        if (typeExec != 3) {
            printf("Donnez la fonction d'estimation utilisée par le PC pour le joueur B : ");
            scanf(" %d", &estMax);
        }
        else
            estMax = 7;
        if (typeExec != 2) {
            printf("Donnez la fonction d'estimation utilisée par le PC pour le joueur N : ");
            scanf(" %d", &estMin);
        }
        else
            estMin = 7;
    }
    while ( estMax < 1 || estMax > nbEst || estMin < 1 || estMin > nbEst );

    estMax--; 
    estMin--;

    printf("\n--- Estimation_pour_Blancs = %d \t Estimation_pour_Noirs = %d ---\n", \
            (typeExec != 3 ? estMax+1 : 0), (typeExec != 2 ? estMin+1 : 0) );
        
    printf("\n'B' : joueur maximisant et 'N' : joueur minimisant\n");
    if (typeExec == 1)
        printf("\nPC 'B' contre PC 'N'\n\n");
    if (typeExec == 2)
        printf("\nUSER 'N' contre PC 'B'\n\n");
    if (typeExec == 3)
        printf("\nUSER 'B' contre PC 'N'\n\n");

    printf("Paramètres de MinMax\n");

    printf("Profondeur maximale d'exploration (par exemple 5) : ");
    scanf(" %d", &hauteur);

    printf("Nombre d'alternatives maximum à considérer dans chaque coup (0 pour +infini) : ");
    scanf(" %d", &largeur);
    if ( largeur == 0 ) largeur = +INFINI;

    // Initialise la configuration de départ
    init( &conf );
    for (i=0; i<MAXPARTIE; i++)
                copier( &conf, &Partie[i] );

    num_coup = 0;
    
    // initialise le générateur de nombre aléatoire pour la fonction estim3(...) si elle est utilisée
    srand( time(NULL) ); 

    // sauter la fin de ligne des lectures précédentes
    while ((i = getchar()) != '\n' && i != EOF) { } 

    printf("\nNom du fichier texte où sera sauvegarder la partie : ");
    fgets(ch, 20, stdin);
    sscanf(ch," %s", nomf);
    strcat(pathf, nomf);
    f = fopen(pathf, "w");
    
    fprintf(f, "--- Estimation_pour_Blancs = %d \t Estimation_pour_Noirs = %d ---\n", \
            (typeExec != 3 ? estMax+1 : 0), (typeExec != 2 ? estMin+1 : 0) );

    printf("\n---------------------------------------------\n\n");

    // Boucle principale du déroulement d'une partie ...
    stop = 0;
    tour = MAX;            // le joueur MAX commence en premier
    nbAlpha = nbBeta = 0;  // les compteurs de coupes alpha et beta

    // ======================================== GUI ================================================

    App *app = malloc(sizeof(App));

    initSDL(app);

    Board board = initBoard(app, conf);

    while(1)
    {
        prepareScene(app);

        doInput(&selectedX, &selectedY, &moveToX, &moveToY);

        setBoardConf(app, &board, conf);
        drawBoard(app, board, conf, &selectedX, &selectedY);

        // Draw texts
        char msgb1[60];
        char msgb2[60];
        char msgn1[60];
        char msgn2[60];

        sprintf(msgb1, "hauteur = %d    nb alternatives = %d", hauteurb, nb);
        sprintf(msgb2, "choix=%d (score:%d)", jb, scoreb);
        sprintf(msgn1, "hauteur = %d    nb alternatives = %d", hauteurn, nn);
        sprintf(msgn2, "choix=%d (score:%d)", jn, scoren);

        switch (typeExec)
        {
            case 1:
                drawText(app, "PC (N)", BOARD_OFFSET_X, 2);
                drawText(app, msgn1, BOARD_OFFSET_X, 20);
                drawText(app, msgn2, BOARD_OFFSET_X, 40);
                drawText(app, "PC (B)", BOARD_OFFSET_X, BOARD_POS_Y + BOARD_SIZE - BOARD_OFFSET_Y);
                drawText(app, msgb1, BOARD_OFFSET_X, BOARD_POS_Y + BOARD_SIZE - BOARD_OFFSET_Y + 20);
                drawText(app, msgb2, BOARD_OFFSET_X, BOARD_POS_Y + BOARD_SIZE - BOARD_OFFSET_Y + 40);
                break;

            case 2:
                drawText(app, "USER (N)", BOARD_OFFSET_X, 2);
                drawText(app, "PC (B)", BOARD_OFFSET_X, BOARD_POS_Y + BOARD_SIZE - BOARD_OFFSET_Y);
                drawText(app, msgb1, BOARD_OFFSET_X, BOARD_POS_Y + BOARD_SIZE - BOARD_OFFSET_Y + 20);
                drawText(app, msgb2, BOARD_OFFSET_X, BOARD_POS_Y + BOARD_SIZE - BOARD_OFFSET_Y + 40);
                break;
            
            default:
                drawText(app, "PC (N)", BOARD_OFFSET_X, 2);
                drawText(app, "USER (B)", BOARD_OFFSET_X, BOARD_POS_Y + BOARD_SIZE - BOARD_OFFSET_Y);
                drawText(app, msgn1, BOARD_OFFSET_X, 20);
                drawText(app, msgn2, BOARD_OFFSET_X, 40);
                break;
        }

        // ========================== Chess Logic =============================================
        if(!stop)
        {
            // affich( &conf, coup, num_coup );
            copier( &conf, &Partie[num_coup % MAXPARTIE] );        // rajouter conf au tableau 'Partie'
            sauvConf( &conf );
            refaire = 0;  // indicateur de coup illegal, pour refaire le mouvement

            if ( tour == MAX ) {   // au tour de MAX ...

                if (typeExec == 3) {   // c-a-d MAX ===> USER
                     
                    // récupérer le coup du joueur ...
                    if(moveToX != -1 && moveToY != -1 && selectedX != -1 && selectedY != -1)
                    {
                        copier(&conf, &conf1);

                        // Traitement du coup du joueur ...

                        if (selectedY == conf.xrB && selectedX == conf.yrB && moveToX == selectedX + 2) { // petit roque ...
                            conf1.mat[0][4] = 0;
                            conf1.mat[0][7] = 0;
                            conf1.mat[0][6] = 'r'; 
                            conf1.xrB = 0; conf1.yrB = 6;
                            conf1.mat[0][5] = 't';
                            conf1.roqueB = 'e';
                        }
                        else
                        if (selectedY == conf.xrB && selectedX == conf.yrB && moveToX == selectedX - 2) { // grand roque ...
                            conf1.mat[0][4] = 0;
                            conf1.mat[0][0] = 0;
                            conf1.mat[0][2] = 'r'; 
                            conf1.xrB = 0; conf1.yrB = 2;
                            conf1.mat[0][3] = 't';
                            conf1.roqueB = 'e';
                        } 
                        else  {  // deplacement normal (les autres coups) ...
                            conf1.mat[moveToY][moveToX] = conf1.mat[selectedY][selectedX];
                            conf1.mat[selectedY][selectedX] = 0;
                            // vérifier possibilité de transformation d'un pion arrivé en fin d'échiquier ...
                            if (moveToY == (8-1) && conf1.mat[moveToY][moveToX] == 'p') {
                                printf("Pion arrivé en ligne 8, transformer en (p/c/f/t/n) : ");
                                scanf(" %s", ch);
                                switch (ch[0]) {
                                    case 'c' : conf1.mat[moveToY][moveToX] = 'c'; break;
                                    case 'f' : conf1.mat[moveToY][moveToX] = 'f'; break;
                                    case 't' : conf1.mat[moveToY][moveToX] = 't'; break;
                                    case 'p' : conf1.mat[moveToY][moveToX] = 'p'; break;
                                    default  : conf1.mat[moveToY][moveToX] = 'n';
                                }
                            }
                        }

                        // vérifier si victoire (le roi N n'existe plus) ...
                        if ( conf1.xrN == moveToY && conf1.yrN == moveToX) {
                            conf1.xrN = -1;
                            conf1.yrN = -1;
                        }

                        // vérification de la légalité du coup effectué par le joueur ...
                        generer_succ(  &conf, MAX, T, &n );

                        legal = 0;
                        for (i=0; i<n && !legal; i++)
                            if ( egal(T[i].mat, conf1.mat) )  legal = 1;

                        if ( legal && !feuille(&conf1,&cout) ) {
                            printf("OK\n\n");

                            // Déselectionner la pièce
                            selectedX = -1;
                            selectedY = -1;

                            i--;
                            formuler_coup( &conf, &T[i], coup );
                            copier( &T[i], &conf );
                        }
                        else
                        if ( !legal && n > 0 ) { 
                            printf("Coup illégal (%c%d -> %c%d) -- réessayer\n", selectedX + 'A', selectedY + 1, moveToX + 'A', moveToY + 1);
                            selectedX = moveToX;
                            selectedY = moveToY;
                            refaire = 1; // pour forcer la prochaine itération à rester MAX
                        }
                        else
                            stop = 1;

                        moveToX = -1;
                        moveToY = -1;
                    }
                    else
                    {
                        refaire = 1;
                    }
                } // if (typeExec == 3)
                else { // MAX ===> PC
                    
                    generer_succ(  &conf, MAX, T, &n );
                    nbp = npieces( &conf );
                
                    // On effectue un tri sur les alternatives selon l'estimation de leur qualité 
                    // Le but est d'explorer les alternatives les plus prometteuses d'abord 
                    // pour maximiser les coupes lors des évaluation minmax avec alpha-bêta

                    // 1- on commence donc par une petite exploration de profondeur h0 
                    //    pour récupérer des estimations plus précises sur chaque coups:
                    for (i=0; i<n; i++) 
                        T[i].val = minmax_ab( &T[i], MIN, h0, -INFINI, +INFINI, largeur, estMax, nbp );

                    // 2- on réalise le tri des alternatives T suivant les estimations récupérées:                
                    qsort(T, n, sizeof(struct config), confcmp321);   // en ordre décroissant des évaluations
                    if ( largeur < n ) n = largeur;
                    
                    // 3- on lance l'exploration des alternatives triées avec la profondeur voulue:
                    score = -INFINI;
                    j = -1;

                    printf("Tour du joueur maximisant 'B' (Essai des alternatives):\n");
                    for (i=0; i<n; i++) {
                        cout = minmax_ab( &T[i], MIN, hauteur, score, +INFINI, largeur, estMax, nbp );

                        printf("."); fflush(stdout);
                        
                        if ( cout > score ) {  // Choisir le meilleur coup (c-a-d le plus grand score)
                            score = cout;
                            j = i;
                        }

                        if ( cout == 100 ) {
                            printf("v\n"); fflush(stdout);
                            break;
                        }
                    }
                    printf("\n");

                    if ( j != -1 ) { // jouer le coup et aller à la prochaine itération ...
                        formuler_coup( &conf, &T[j], coup );
                        copier( &T[j], &conf );
                        conf.val = score;
                    }
                    else   // S'il n'y a pas de successeur possible, le joueur MAX à perdu
                        stop = 1;

                    // Sauvegarder pour affichage
                    hauteurb = hauteur;
                    nb = n;
                    jb = j;
                    scoreb = score;
                } // fin else // MAX ===> PC
        
                if (stop) {
                    if(scoreb == 0)
                    {
                        printf("\n *** Match Nul (Stalemate) ! ***\n");
                    }
                    else
                    {
                        printf("\n *** le joueur maximisant 'B' a perdu ***\n");
                        fprintf(f, "Victoire de 'N'\n");
                    }
                }

            }  // if ( tour == MAX )

            else {  // donc tour == MIN 

                if (typeExec == 2) {   // c-a-d MIN ===> USER

                    if(moveToX != -1 && moveToY != -1 && selectedX != -1 && selectedY != -1)
                    {
                        copier(&conf, &conf1);

                        // Traitement du coup du joueur ...

                        if (selectedY == conf.xrN && selectedX == conf.yrN && moveToX == moveToX + 2) { // petit roque ...
                            conf1.mat[7][4] = 0;
                            conf1.mat[7][7] = 0;
                            conf1.mat[7][6] = -'r'; 
                            conf1.xrN = 7; conf1.yrN = 6;
                            conf1.mat[7][5] = -'t';
                            conf1.roqueN = 'e';
                        }
                        else
                        if (selectedY == conf.xrN && selectedX == conf.yrN && moveToX == moveToX - 2) { // grand roque ...
                            conf1.mat[7][4] = 0;
                            conf1.mat[7][0] = 0;
                            conf1.mat[7][2] = -'r'; 
                            conf1.xrN = 7; conf1.yrN = 2;
                            conf1.mat[7][3] = -'t';
                            conf1.roqueN = 'e';
                        } 
                        else  {  // deplacement normal (les autres coups) ...
                            conf1.mat[moveToY][moveToX] = conf1.mat[selectedY][selectedX];
                            conf1.mat[selectedY][selectedX] = 0;
                            // vérifier possibilité de transformation d'un pion arrivé en fin d'échiquier ...
                            if (moveToY == 0 && conf1.mat[selectedY][selectedX] == -'p') {
                                printf("Pion arrivé en ligne 8, transformer en (p/c/f/t/n) : ");
                                scanf(" %s", ch);
                                switch (ch[0]) {
                                case 'c' : conf1.mat[moveToY][moveToX] = -'c'; break;
                                case 'f' : conf1.mat[moveToY][moveToX] = -'f'; break;
                                case 't' : conf1.mat[moveToY][moveToX] = -'t'; break;
                                case 'p' : conf1.mat[moveToY][moveToX] = -'p'; break;
                                default  : conf1.mat[moveToY][moveToX] = -'n';
                                }
                            }
                        }

                        // vérifier si victoire (le roi B n'existe plus) ...
                        if ( conf1.xrB == moveToY && conf1.yrB == moveToX) {
                            conf1.xrB = -1;
                            conf1.yrB = -1;
                        }

                        // vérification de la légalité du coup effectué par le joueur ...
                        generer_succ(  &conf, MIN, T, &n );

                        legal = 0;
                        for (i=0; i<n && !legal; i++)
                            if ( egal(T[i].mat, conf1.mat) )  legal = 1;

                        if ( legal && !feuille(&conf1,&cout) ) {
                            printf("OK\n\n");

                            // Déselectionner la pièce
                            selectedX = -1;
                            selectedY = -1;

                            i--;
                            formuler_coup( &conf, &T[i], coup );
                            copier( &T[i], &conf );
                        }
                        else
                        if ( !legal && n > 0 ) {
                            printf("Coup illégal (%c%d -> %c%d) -- réessayer\n", selectedX + 'A', selectedY + 1, moveToX + 'A', moveToY + 1);
                            selectedX = moveToX;
                            selectedY = moveToY;
                            refaire = 1; // pour forcer la prochaine itération à rester MIN
                        }
                        else
                            stop = 1;

                        moveToX = -1;
                        moveToY = -1;
                    }
                    else
                    {
                        refaire = 1;
                    }
                } // if (typeExec == 2)

                else { // MIN ===> PC
                    
                    // Générer tous les coups possibles pour le joueur N dans le tableau T
                    generer_succ(  &conf, MIN, T, &n );
                    nbp = npieces( &conf);
                
                    // On effectue un tri sur les alternatives selon l'estimation de leur qualité 
                    // Le but est d'explorer les alternatives les plus prometteuses d'abord 
                    // pour maximiser les coupes lors des évaluation minmax avec alpha-bêta

                    // 1- on commence donc par une petite exploration de profondeur h0 
                    //    pour récupérer des estimations plus précises sur chaque coups:
                    for (i=0; i<n; i++) 
                        T[i].val = minmax_ab( &T[i], MAX, h0, -INFINI, +INFINI, largeur, estMin, nbp );

                    // 2- on réalise le tri des alternatives T suivant les estimations récupérées:
                    qsort(T, n, sizeof(struct config), confcmp123);   // en ordre croissant des évaluations
                    if ( largeur < n ) n = largeur;

                    // 3- on lance l'exploration des alternatives triées avec la profondeur voulue:
                    score = +INFINI;
                    j = -1;

                    printf("Tour du joueur minimisant 'N' (Essai des alternatives):\n");               
                    for (i=0; i<n; i++) {
                        cout = minmax_ab( &T[i], MAX, hauteur, -INFINI, score, largeur, estMin, nbp );
                        //cout = minmax_ab( &T[i], MAX, hauteur, -INFINI, +INFINI, largeur, estMin );
                        printf("."); fflush(stdout);
                        if ( cout < score ) {  // Choisir le meilleur coup (c-a-d le plus petit score)
                            score = cout;
                            j = i;
                        }
                        if ( cout == -100 ) {
                            printf("v"); fflush(stdout);
                            break;
                        }
                    }
                    printf("\n");

                    if ( j != -1 ) { // jouer le coup et aller à la prochaine itération ...
                        formuler_coup( &conf, &T[j], coup );
                        copier( &T[j], &conf );
                        conf.val = score;
                    }
                    else  // S'il n'y a pas de successeur possible, le joueur MIN à perdu
                        stop = 1;

                    // Sauvegarder pour affichage
                    hauteurn = hauteur;
                    nn = n;
                    jn = j;
                    scoren = score; 
                } // fin else // MIN ===> PC

                if (stop) {
                    if(scoren == 0)
                    {
                        printf("\n *** Match Nul (Stalemate) ! ***\n");
                    }
                    else
                    {
                        printf("\n *** le joueur minimisant 'N' a perdu ***\n");
                        fprintf(f, "Victoire de 'B'\n");
                    }
                }

            } // fin else // tour == MIN

            if ( !refaire ) {
                num_coup++;
                tour = ( tour == MIN ? MAX : MIN );
            }
        } // fin if ( !stop )
        else
        {
            selectedX = -1;
            selectedY = -1;
            moveToX = -1;
            moveToY = -1;
        }

        presentScene(app);

        SDL_Delay(16);
    }

    printf("\nNb de coupes (alpha:%d + beta:%d)  = %d", nbAlpha, nbBeta,nbAlpha+nbBeta); 
    printf("\nFin de partie\n");

    return 0;
}