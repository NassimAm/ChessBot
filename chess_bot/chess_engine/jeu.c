/********************************************************************************
*********************************************************************************

Programme de démonstration de MinMax avec élagage alpha/bêta pour le jeu d'échecs
Hidouci / CRP - 2CS-SID / ESI 2023/2024

Implémentation du module : jeu.c

Les fonctions sont regroupées en 4 parties :              
      - Partie : Minmax avec Alpha/Beta                   
      - Partie : Evaluations et Estimations               
      - Partie : Génération des alternatives              
      - Partie : Fonctions utilitaires                    

*********************************************************************************
*********************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <time.h>
#include <limits.h>          // pour INT_MAX
#include "jeu.h"



/*******************************************************/
/*** Les variables globales définies dans "main.c"   ***/
/*******************************************************/

// Tableau de config pour garder la trace des conf déjà visitées
extern struct config Partie[MAXPARTIE];

// Fichier pour sauvegarder l'historique des parties
extern FILE *f;

// compteur de coups effectués
extern int num_coup;          

// profondeur initiale d'exploration préliminaire avant le tri des alternatives
extern int h0;

// tableau de pointeurs de fonctions (les fonctions d'estimations disponibles)
extern int (*Est[10])(struct config *);

// nb de fonctions d'estimation dans le tableau précédent
extern int nbEst;

// pour statistques sur le nombre de coupes effectuées
extern int nbAlpha;
extern int nbBeta;


/*********************************************************/
/*** Les variables internes (static) du module "jeu.c" ***/
/*********************************************************/

// vecteurs pour générer les différents déplacements par type de pièce ...
//    cavalier :
static int dC[8][2] = { {-2,+1} , {-1,+2} , {+1,+2} , {+2,+1} , {+2,-1} , {+1,-2} , {-1,-2} , {-2,-1} };
//    fou (indices impairs), tour (indices pairs), reine et roi (indices pairs et impairs):
static int D[8][2] = { {+1,0} , {+1,+1} , {0,+1} , {-1,+1} , {-1,0} , {-1,-1} , {0,-1} , {+1,-1} }; 



/***********************************************************************************/


// ******************************
// Partie:  MinMax avec AlphaBeta
// ******************************


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
{
   int n, i, score, score2, npc;
   struct config T[100];

   // npc: le nombre de pieces dans conf
   // npp: le nombre de pieces dans le parent de conf
   npc = npieces(conf);   

   if ( feuille(conf, &score) )
      return score;

   if ( niv == 0 ) {	// si le niveau souhaité est atteint
      			// traitement partiel (et simpliste) de l'effet horizon :
      if ( npp == npc  || npp == 0) {	// si conf est stable ou alors on a deja ajouté un niveau
         return Est[numFctEst]( conf );	// evaluer par estimation de conf
      }
      else {				// sinon
         niv = 1;			// ajouter un niveau supplementaire dans l'exploration
         npc = 0;			// et ne plus considerer la stabilité de la prochaine conf
      }

   }
                
   if ( mode == MAX ) {

      generer_succ( conf, MAX, T, &n );

      // Match nul (pas de coups possibles pour Blanc et le roi blanc n'est pas checké)
      if ( conf->xrB != -1 && n == 0 && !caseMenaceePar( MIN, conf->xrB, conf->yrB, conf ) )
      {
         return 0;
      }

      if ( largeur != +INFINI ) {
         for (i=0; i<n; i++) 
             T[i].val = Est[numFctEst]( &T[i] );                    

	 // trier en ordre decroissant les successeurs selon leurs estimations
         qsort(T, n, sizeof(struct config), confcmp321);
         if ( largeur < n ) n = largeur;   // pour limiter la largeur d'exploration
      }

      score = alpha;
      for ( i=0; i<n; i++ ) {
          score2 = minmax_ab( &T[i], MIN, niv-1, score, beta, largeur, numFctEst, npc);
          if (score2 > score) score = score2;
             if (score >= beta) {
                // Coupe Beta
                nbBeta++;       // compteur de coupes beta
                return score;   // evaluation tronquee ***
             }
          } 
   }
   
   else  { // mode == MIN 

      generer_succ( conf, MIN, T, &n );

      // Match nul (pas de coups possibles pour Noir et le roi noir n'est pas checké)
      if ( conf->xrN != -1 && n == 0 && !caseMenaceePar( MAX, conf->xrN, conf->yrN, conf ) )
      {
         return 0;
      }

      if ( largeur != +INFINI ) {
         for (i=0; i<n; i++) 
             T[i].val = Est[numFctEst]( &T[i] );                    

	 // trier en ordre croissant les successeurs selon leurs estimations
         qsort(T, n, sizeof(struct config), confcmp123);
         if ( largeur < n ) n = largeur;   // pour limiter la largeur d'exploration
      }

      score = beta;
      for ( i=0; i<n; i++ ) {
         score2 = minmax_ab( &T[i], MAX, niv-1, alpha, score, largeur, numFctEst, npc );
         if (score2 < score) score = score2;
            if (score <= alpha) {
               // Coupe Alpha
               nbAlpha++;      // compteur de coupes alpha
               return score;   // evaluation tronquee ***
            }
         }
   }

   if ( score == +INFINI ) score = +100;
   if ( score == -INFINI ) score = -100;

   return score;

} // fin de minmax_ab

/***********************************************************************************/



// ***********************************
// Partie:  Evaluations et Estimations 
// ***********************************


/* Teste si conf représente une fin de partie et retourne dans 'cout' la valeur associée */
int feuille( struct config *conf, int *cout )
{
        
   *cout = 0;

   // Si victoire pour les Noirs cout = -100
   if ( conf->xrB == -1 ) { 
      *cout = -100;
      return 1; 
   }

   // Si victoire pour les Blancs cout = +100
   if ( conf->xrN == -1 ) {
      *cout = +100;
      return 1;
   }

   // Sinon ce n'est pas une config feuille 
   return 0;

}  // fin de feuille


/* Quelques exemples de fonctions d'estimation simples (estim1, estim2, ...) */
/* Voir fonction estim plus bas pour le choix l'estimation à utiliser */

/* cette estimation est basée uniquement sur le nombre de pièces */
int estim1( struct config *conf )
{

        int i, j, ScrQte;
        int pionB = 0, pionN = 0, cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;
        
        // parties : nombre de pièces 
        for (i=0; i<8; i++)
           for (j=0; j<8; j++) {
                switch (conf->mat[i][j]) {
                   case 'p' : pionB++;   break;
                   case 'c' : 
                   case 'f' : cfB++;  break;
                   case 't' : tB++; break;
                   case 'n' : nB++;  break;

                   case -'p' : pionN++;  break;
                   case -'c' : 
                   case -'f' : cfN++;  break;
                   case -'t' : tN++; break;
                   case -'n' : nN++;  break;
                }
           }

        // Somme pondérée de pièces de chaque joueur. 
        // Les poids sont fixés comme suit: pion:2  cavalier/fou:6  tour:8  et  reine:20
        // Le facteur 100/76 pour ne pas sortir de l'intervalle ]-100 , +100[
        ScrQte = ( (pionB*2 + cfB*6 + tB*8 + nB*20) - (pionN*2 + cfN*6 + tN*8 + nN*20) ) * 100.0/76;

        if (ScrQte > 95) ScrQte = 95;                // pour rétrécir l'intervalle à
        if (ScrQte < -95) ScrQte = -95;                // ]-95 , +95[ car ce n'est qu'une estimation

        return ScrQte;

} // fin de estim1


// estimation basée sur le nb de pieces, l'occupation, la défense du roi et les roques
int estim2(  struct config *conf )
{
        int i, j, a, b, stop, bns, ScrQte, ScrDisp, ScrDfs, ScrDivers, Score;
        int pionB = 0, pionN = 0, cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;
        int occCentreB = 0, occCentreN = 0, protectRB = 0, protectRN = 0, divB = 0, divN = 0;

        // parties : nombre de pièces et occupation du centre
        for (i=0; i<8; i++)
           for (j=0; j<8; j++) {
                bns = 0;  // bonus pour l'occupation du centre de l'échiquier
                if (i>1 && i<6 && j>=0 && j<=7 ) bns = 1; 
                if (i>2 && i<5 && j>=2 && j<=5 ) bns = 2; 
                switch (conf->mat[i][j]) {
                   case 'p' : pionB++; occCentreB += bns;  break;
                   case 'c' : 
                   case 'f' : cfB++; occCentreB += 4*bns; break;
                   case 't' : tB++; break;
                   case 'n' : nB++; occCentreB += 4*bns; break;

                   case -'p' : pionN++; occCentreN += bns; break;
                   case -'c' : 
                   case -'f' : cfN++; occCentreN += 4*bns; break;
                   case -'t' : tN++; break;
                   case -'n' : nN++; occCentreN += 4*bns; break;
                }
           }

        ScrQte = ( (pionB*2 + cfB*6 + tB*8 + nB*20) - (pionN*2 + cfN*6 + tN*8 + nN*20) );
        // donc ScrQteMax ==> 76

        ScrDisp = occCentreB - occCentreN;
        // donc ScrDispMax ==> 42
        
        // partie : défense du roi B ...
        for (i=0; i<8; i += 1) {
           // traitement des 8 directions paires et impaires
           stop = 0;
           a = conf->xrB + D[i][0];
           b = conf->yrB + D[i][1];                   
           while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
                if ( conf->mat[a][b] != 0 )  stop = 1;
                else {
                    a = a + D[i][0];
                    b = b + D[i][1];
                }
           if ( stop ) 
                if ( conf->mat[a][b] > 0 ) protectRB++; 
        } // for

        // partie : défense du roi N ...
        for (i=0; i<8; i += 1) {
           // traitement des 8 directions paires et impaires
           stop = 0;
           a = conf->xrN + D[i][0];
           b = conf->yrN + D[i][1];                   
           while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
                if ( conf->mat[a][b] != 0 )  stop = 1;
                else {
                    a = a + D[i][0];
                    b = b + D[i][1];
                }
           if ( stop ) 
                if ( conf->mat[a][b] < 0 ) protectRN++; 
        } // for        
         
        ScrDfs = protectRB - protectRN;
        // donc ScrDfsMax ==> 8

        // Partie : autres considérations ...
        if ( conf->roqueB == 'e' ) divB = 24;        // favoriser les roques B
        if ( conf->roqueB == 'r' ) divB = 12;
        if ( conf->roqueB == 'p' || conf->roqueB == 'g' ) divB = 10;

        if ( conf->roqueN == 'e' ) divN = 24;        // favoriser les roques N
        if ( conf->roqueN == 'r' ) divN = 12;
        if ( conf->roqueN == 'p' || conf->roqueN == 'g' ) divN = 10;

        ScrDivers = divB - divN;
        // donc ScrDiversMax ==> 24

        Score = (4*ScrQte + ScrDisp + ScrDfs + ScrDivers) * 100.0/(4*76+42+8+24);
        // pour les poids des pièces et le facteur multiplicatif voir commentaire dans estim1

        if (Score > 98 ) Score = 98;
        if (Score < -98 ) Score = -98;

        return Score;

} // fin de estim2


/* prise en compte du nombre de pièces avec petite perturbation aléatoire */
int estim3( struct config *conf )
{

        int i, j, ScrQte, Score;
        int pionB = 0, pionN = 0, cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;

        // parties : nombre de pièces
        for (i=0; i<8; i++)
           for (j=0; j<8; j++) {
                switch (conf->mat[i][j]) {
                   case 'p' : pionB++; break;
                   case 'c' : 
                   case 'f' : cfB++; break;
                   case 't' : tB++; break;
                   case 'n' : nB++; break;

                   case -'p' : pionN++; break;
                   case -'c' : 
                   case -'f' : cfN++; break;
                   case -'t' : tN++; break;
                   case -'n' : nN++; break;
                }
           }

        ScrQte = ( (pionB*2 + cfB*6 + tB*8 + nB*20) - (pionN*2 + cfN*6 + tN*8 + nN*20) );
        // donc ScrQteMax ==> 76

        Score = (10*ScrQte + rand()%10) * 100.0 / (10*76+10);
        // pour les poids des pièces et le facteur multiplicatif voir commentaire dans estim1

        if (Score > 98 ) Score = 98;
        if (Score < -98 ) Score = -98;

        return Score;

} // fin de estim3


// estimation basée sur le nb de pieces et les menaces
int estim4( struct config *conf )
{

        int i, j, Score;
        int pionB = 0, pionN = 0, cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;
        int npmB = 0, npmN = 0;

        // parties : nombre de pièces 
        for (i=0; i<8; i++)
           for (j=0; j<8; j++) {
                switch (conf->mat[i][j]) {
                   case 'p' : pionB++;   break;
                   case 'c' : 
                   case 'f' : cfB++;  break;
                   case 't' : tB++; break;
                   case 'n' : nB++;  break;

                   case -'p' : pionN++;  break;
                   case -'c' : 
                   case -'f' : cfN++;  break;
                   case -'t' : tN++; break;
                   case -'n' : nN++;  break;
                }
           }

        for (i=0; i<8; i++)
           for (j=0; j<8; j++) {
                if ( conf->mat[i][j] < 0 && caseMenaceePar(MAX, i, j, conf) ) {
                   npmB++;
                   if ( conf->mat[i][j] == -'c' || conf->mat[i][j] == -'f' )
                           npmB++;
                   if ( conf->mat[i][j] == -'t' || conf->mat[i][j] == -'n' )
                           npmB += 2;
                   if ( conf->mat[i][j] == -'r' )
                           npmB += 5;
                }
                if ( conf->mat[i][j] > 0 && caseMenaceePar(MIN, i, j, conf) ) {
                   npmN++;
                   if ( conf->mat[i][j] == 'c' || conf->mat[i][j] == 'f' )
                           npmN++;
                   if ( conf->mat[i][j] == 't' || conf->mat[i][j] == 'n' )
                           npmN += 2;
                   if ( conf->mat[i][j] == 'r' )
                           npmN += 5;
                }
           }

        Score = ( 4*((pionB*2 + cfB*6 + tB*8 + nB*20) - (pionN*2 + cfN*6 + tN*8 + nN*20)) + \
                  (npmB - npmN) ) * 100.0/(4*76+31);

        // pour les poids des pièces et le facteur multiplicatif voir commentaire dans estim1

        if (Score > 95) Score = 95;                // pour rétrécir l'intervalle à
        if (Score < -95) Score = -95;                // ]-95 , +95[ car ce n'est qu'une estimation

        return Score;

} // fin de estim4


// estimation basée sur le nb de pieces et l'occupation
int estim5(  struct config *conf )
{
        int i, j, a, b, stop, bns, ScrQte, ScrDisp, ScrDfs, ScrDivers, Score;
        int pionB = 0, pionN = 0, cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;
        int occCentreB = 0, occCentreN = 0, protectRB = 0, protectRN = 0, divB = 0, divN = 0;

        // parties : nombre de pièces et occupation du centre
        for (i=0; i<8; i++)
           for (j=0; j<8; j++) {
                bns = 0;  // bonus pour l'occupation du centre de l'échiquier
                if (i>1 && i<6 && j>=0 && j<=7 ) bns = 1; 
                if (i>2 && i<5 && j>=2 && j<=5 ) bns = 2; 
                switch (conf->mat[i][j]) {
                   case 'p' : pionB++; occCentreB += bns;  break;
                   case 'c' : 
                   case 'f' : cfB++; occCentreB += 4*bns; break;
                   case 't' : tB++; break;
                   case 'n' : nB++; occCentreB += 4*bns; break;

                   case -'p' : pionN++; occCentreN += bns; break;
                   case -'c' : 
                   case -'f' : cfN++; occCentreN += 4*bns; break;
                   case -'t' : tN++; break;
                   case -'n' : nN++; occCentreN += 4*bns; break;
                }
           }

        ScrQte = ( (pionB*2 + cfB*6 + tB*8 + nB*20) - (pionN*2 + cfN*6 + tN*8 + nN*20) );

        ScrDisp = occCentreB - occCentreN;

        Score = (4*ScrQte + ScrDisp) * 100.0/(4*76+42);
        // pour les poids des pièces et le facteur multiplicatif voir commentaire dans estim1

        if (Score > 98 ) Score = 98;
        if (Score < -98 ) Score = -98;

        return Score;

} // fin de estim5


/* on peut aussi combiner entre plusieurs estimation comme cet exemple: */
int estim6( struct config *conf )
{
        // Combinaison de 3 fonctions d'estimation:
        // -au début on utilise estim2 pour favoriser l'occupation du centre et la defense du roi
        // -au milieu on utilise estim5 pour continuer à bien se positionner
        // -dans la dernière phase on utilise estim4 pour favoriser l'attaque
        if ( num_coup < 25 )                        // 1ere phase
           return estim2(conf);          
        if ( num_coup >= 25 && num_coup < 35 )        // 2e phase
           return estim5(conf);         
        if ( num_coup >= 35 )                        // 3e phase
           return estim4(conf);        

} // fin de estim6


/* Une fonction d'estimation aléatoire */
int estim7( struct config *conf )
{
   return (rand() % 200) - 100;
} // fin de estim7

/* Estimation basée sur le nb de pièces et le nb de cases occupées par chaque joueur (mobilité)*/
int estim8( struct config *conf )
{
   int mode = MAX;
   int casesScore = 0;
   int a, b, stop;
   int pionNb = 0, chNb = 0, cfNb = 0, tNb = 0, nNb = 0;
   const int poidsPion = 1, poidsRoi = 1;
   const int poidsCh = 2;
   const int poidsF = 3;
   const int poidsTr = 4;
   const int poidsRn = 5;

   const double poidsNb = 3;
   const double poidsMobilite = 2;

   int score = 0;
    
   for (int i=0; i<8; i++)
   {
      for (int j=0; j<8; j++) {
         if(conf->mat[i][j] != 0) { // Si la case n'est pas vide
            mode = conf->mat[i][j] / abs(conf->mat[i][j]);
            switch (abs(conf->mat[i][j])) {
               case 'p' :
                  pionNb += mode;  
                  if ( i < 7 && conf->mat[i+1][j] == 0 ) casesScore += mode * poidsPion; // avance d'une case
                  if ( i == 1 && conf->mat[2][j] == 0 && conf->mat[3][j] == 0) casesScore += mode * 2 * poidsPion; // avance de 2 cases
                  if ( i < 7 && j > 0 && conf->mat[i+1][j-1] < 0 ) casesScore += mode * poidsPion;
                  if ( i < 7 && j < 7 && conf->mat[i+1][j+1] < 0 ) casesScore += mode * poidsPion;
                  break;
               // mvmt CAVALIER ...
               case 'c' :
                  chNb += mode;
                  for (int k=0; k<8; k++)
                     if ( i+dC[k][0] <= 7 && i+dC[k][0] >= 0 && j+dC[k][1] <= 7 && j+dC[k][1] >= 0 )
                        if ( conf->mat[ i+dC[k][0] ] [ j+dC[k][1] ] <= 0 )  {
                           casesScore += mode * poidsCh;
                        }
                  break;
               // mvmt FOU ...
               case 'f' :
                  cfNb += mode;
                  for (int k=1; k<8; k += 2) {
                     // traitement des directions impaires (1, 3, 5 et 7)
                     stop = 0;
                     a = i + D[k][0];
                     b = j + D[k][1];                   
                     while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
                           if ( mode * conf->mat[ a ] [ b ] > 0 )  stop = 1;
                           else {
                              if ( mode * conf->mat[a][b] < 0 ) stop = 1;
                              casesScore += mode * poidsF;
                              a = a + D[k][0];
                              b = b + D[k][1];
                           }
                     } // while
                  } // for
                  break;
               // mvmt TOUR ...
               case 't' :
                  tNb += mode;
                  for (int k=0; k<8; k+=2) {
                     // traitement des directions paires (0, 2, 4 et 6)
                     stop = 0;
                     a = i + D[k][0];
                     b = j + D[k][1];                 
                     while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
                           if ( mode * conf->mat[ a ] [ b ] > 0 )  stop = 1;
                           else {
                              if ( mode * conf->mat[a][b] < 0 ) stop = 1;
                              casesScore += mode * poidsTr;
                              a = a + D[k][0];
                              b = b + D[k][1];
                           }
                     } // while
                  } // for
                  break;
               // mvmt REINE ...
               case 'n' :
                  nNb += mode; 
                  for (int k=0; k<8; k+= 1) {
                     // traitement des 8 directions paires et impaires
                     stop = 0;
                     a = i + D[k][0];
                     b = j + D[k][1];                   
                     while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
                        if ( mode * conf->mat[ a ] [ b ] > 0 )  stop = 1;
                        else {
                           if ( mode * conf->mat[a][b] < 0 ) stop = 1;
                           casesScore += mode * poidsRn;
                           a = a + D[k][0];
                           b = b + D[k][1];
                        }
                     } // while
                  } // for
                  break;
               // mvmt ROI ...
               case 'r' : 
                  for (int k=0; k<8; k+=1) {
                     // traitement des 8 directions paires et impaires
                     a = i + D[k][0];
                     b = j + D[k][1];                   
                     if ( a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
                           if ( mode * conf->mat[a][b] <= 0 ) {
                              casesScore += mode * poidsRoi;
                           }
                  } // for
                  break;
            }
         }
      }
   }

   score = (poidsMobilite * casesScore * 100.0 / 64.0 + poidsNb * (pionNb*2 + cfNb*4 + chNb*6 + tNb*8 + nNb*20) * 100.0 / 72.0) / (poidsMobilite + poidsNb);

   if (score > 95) score = 95;                // pour rétrécir l'intervalle à
   if (score < -95) score = -95;                // ]-95 , +95[ car ce n'est qu'une estimation

   return score;
}

/* Estimation basée sur le nb de pièces les captures et la protection des pièces*/
int estim9( struct config *conf )
{
   int i, j;
   int pionB = 0, pionN = 0, chB = 0, chN = 0, cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;

   const int poidsCapture = 2;
   const int poidsProtection = 1;
   const double poidsNbPieces = 3;
   const double poidsStrategie = 2;

   int score = 0, scoreNb = 0, scoreS = 0;

   for (i=0; i<8; i++)
   {
      for (j=0; j<8; j++) {
         if ( conf->mat[i][j] < 0) {
            // Attaque du joueur blanc
            if(caseMenaceePar(MAX, i, j, conf))
            {
               scoreS += poidsCapture;
               if ( conf->mat[i][j] == -'c' || conf->mat[i][j] == -'f' )
                  scoreS += poidsCapture;
               if ( conf->mat[i][j] == -'t' || conf->mat[i][j] == -'n' )
                  scoreS += 2 * poidsCapture;
               if ( conf->mat[i][j] == -'r' )
                  scoreS += 5 * poidsCapture;
            }
            // Le joueur noir protège la case (i,j) grâce à une de ses pièces
            if(caseMenaceePar(MIN, i, j, conf))
            {
               scoreS -= poidsProtection; // Pour éviter de trop sacrifier les reines et les tours
               if ( conf->mat[i][j] == -'c' || conf->mat[i][j] == -'f' )
                     scoreS -= poidsProtection;
               if ( conf->mat[i][j] == -'p' ) // Pour favoriser l'attaque avec les pions tout en les protégeant
                     scoreS -= 2 * poidsProtection;
            }
         }
         if ( conf->mat[i][j] > 0 ) {
            // Attaque du joueur noir
            if(caseMenaceePar(MIN, i, j, conf))
            {
               scoreS -= poidsCapture;
               if ( conf->mat[i][j] == 'c' || conf->mat[i][j] == 'f' )
                  scoreS -= poidsCapture;
               if ( conf->mat[i][j] == 't' || conf->mat[i][j] == 'n' )
                  scoreS -= 2 * poidsCapture;
               if ( conf->mat[i][j] == 'r' )
                  scoreS -= 5 * poidsCapture;
            }
            // Le joueur blanc protège la case (i,j) grâce à une de ses pièces
            if(caseMenaceePar(MAX, i, j, conf))
            {
               scoreS += poidsProtection; // Pour éviter de trop sacrifier les reines et les tours
               if ( conf->mat[i][j] == 'c' || conf->mat[i][j] == 'f' )
                     scoreS += poidsProtection;
               if ( conf->mat[i][j] == 'p' ) // Pour favoriser l'attaque avec les pions tout en les protégeant
                     scoreS += 2 * poidsProtection;
            }
         }

         // Nombre de pièces
         switch (conf->mat[i][j]) {
            case 'p' : pionB++;   break;
            case 'c' : chB++; break;
            case 'f' : cfB++;  break;
            case 't' : tB++; break;
            case 'n' : nB++;  break;

            case -'p' : pionN++;  break;
            case -'c' : chN++; break;
            case -'f' : cfN++;  break;
            case -'t' : tN++; break;
            case -'n' : nN++;  break;
         }
      }
   }

   scoreNb = (pionB*2 + cfB*4 + chB*6 + tB*8 + nB*20) - (pionN*2 + cfN*4 + chN*6 + tN*8 + nN*20);
   score = (poidsNbPieces * (scoreNb * 100.0 / 72.0) + poidsStrategie * (scoreS * 100.0 / 58.0)) / (poidsNbPieces + poidsStrategie);

   if (score > 95) score = 95;                // pour rétrécir l'intervalle à
   if (score < -95) score = -95;                // ]-95 , +95[ car ce n'est qu'une estimation

   return score;
}

/***********************************************************************************/


// ************************************
// Partie:  Génération des alternatives
// ************************************


/* Génère, pour le joueur 'mode', les successeurs de la configuration 'conf' dans le tableau 'T', 
   retourne aussi dans 'n' le nombre de configurations filles générées */
void generer_succ( struct config *conf, int mode, struct config T[], int *n )
{
        int i, j, k, stop;

        *n = 0;

        if ( mode == MAX ) {                // mode == MAX
           for (i=0; i<8; i++)
              for (j=0; j<8; j++)
                 if ( conf->mat[i][j] > 0 )
                    deplacementsB(conf, i, j, T, n );

           // vérifier si le roi est en echec, auquel cas on ne garde que les succ évitants l'échec
           // ou alors si une conf est déjà visitée dans Partie, auquel cas on l'enlève aussi ...
           for (k=0; k < *n; k++) {
               i = T[k].xrB; j = T[k].yrB;  // pos du roi B dans T[k]
               // vérifier si roi menacé dans la config T[k] ou alors T[k] est dejà visitée ...
               if ( caseMenaceePar( MIN, i, j, &T[k] ) || dejaVisitee( &T[k] ) ) {
                   T[k] = T[(*n)-1];        // alors supprimer T[k] de la liste des succ...
                   (*n)--;
                   k--;
               }
           } // for k
        }

        else {                                 // mode == MIN
           for (i=0; i<8; i++)
              for (j=0; j<8; j++)
                 if ( conf->mat[i][j] < 0 )
                    deplacementsN(conf, i, j, T, n );

           // vérifier si le roi est en echec, auquel cas on ne garde que les succ évitants l'échec
           // ou alors si une conf est déjà visitée dans Partie, auquel cas on l'enlève aussi ...
           for (k=0; k < *n; k++) {
               i = T[k].xrN; j = T[k].yrN;
               // vérifier si roi menacé dans la config T[k] ou alors T[k] est dejà visitée ...
               if ( caseMenaceePar( MAX, i, j, &T[k] ) || dejaVisitee( &T[k] ) ) {
                  T[k] = T[(*n)-1];        // alors supprimer T[k] de la liste des succ...
                  (*n)--;
                  k--;
               }
           } // for k        
        } // if (mode == MAX) ... else ...

} // fin de generer_succ


/* Génère dans T les configurations obtenues à partir de conf lorsqu'un pion (a,b) va atteindre 
   la limite de l'échiquier: pos (x,y)  */
void transformPion( struct config *conf, int a, int b, int x, int y, struct config T[], int *n )
{
        int signe = +1;
        if (conf->mat[a][b] < 0 ) signe = -1;
        copier(conf, &T[*n]);
        T[*n].mat[a][b] = 0;
        T[*n].mat[x][y] = signe * 'n';        // transformation en Reine
        (*n)++;
        copier(conf, &T[*n]);
        T[*n].mat[a][b] = 0;
        T[*n].mat[x][y] = signe * 'c';        // transformation en Cavalier
        (*n)++;
        copier(conf, &T[*n]);
        T[*n].mat[a][b] = 0;
        T[*n].mat[x][y] = signe * 'f';        // transformation en Fou
        (*n)++;
        copier(conf, &T[*n]);
        T[*n].mat[a][b] = 0;
        T[*n].mat[x][y] = signe * 't';        // transformation en Tour
        (*n)++;

} // fin de transformPion


/* Génere dans T tous les coups possibles de la pièce (de couleur N) se trouvant à la pos x,y */
void deplacementsN(struct config *conf, int x, int y, struct config T[], int *n )
{
        int i, j, a, b, stop;

        switch(conf->mat[x][y]) {
        // mvmt PION ...
        case -'p' : 
                if ( x > 0 && conf->mat[x-1][y] == 0 ) {                                
                        // avance d'une case
                        copier(conf, &T[*n]);
                        T[*n].mat[x][y] = 0;
                        T[*n].mat[x-1][y] = -'p';
                        (*n)++;
                        if ( x == 1 ) transformPion( conf, x, y, x-1, y, T, n );
                }
                if ( x == 6 && conf->mat[5][y] == 0 && conf->mat[4][y] == 0) {        
                        // avance de 2 cases
                        copier(conf, &T[*n]);
                        T[*n].mat[6][y] = 0;
                        T[*n].mat[4][y] = -'p';
                        (*n)++;
                }
                if ( x > 0 && y >0 && conf->mat[x-1][y-1] > 0 ) {                
                        // attaque à droite (en descendant)
                        copier(conf, &T[*n]);
                        T[*n].mat[x][y] = 0;
                        T[*n].mat[x-1][y-1] = -'p';
                        // cas où le roi adverse est pris...
                        if (T[*n].xrB == x-1 && T[*n].yrB == y-1) { 
                                T[*n].xrB = -1; T[*n].yrB = -1; 
                        }

                        (*n)++;
                        if ( x == 1 ) transformPion( conf, x, y, x-1, y-1, T, n ); 
                }
                if ( x > 0 && y < 7 && conf->mat[x-1][y+1] > 0 ) {                
                        // attaque à gauche (en descendant)
                        copier(conf, &T[*n]);
                        T[*n].mat[x][y] = 0;
                        T[*n].mat[x-1][y+1] = -'p';
                        // cas où le roi adverse est pris...
                        if (T[*n].xrB == x-1 && T[*n].yrB == y+1) { 
                                T[*n].xrB = -1; T[*n].yrB = -1; 
                        }

                        (*n)++;
                        if ( x == 1 ) transformPion( conf, x, y, x-1, y+1, T, n );
                }
                break;

        // mvmt CAVALIER ...
        case -'c' : 
                for (i=0; i<8; i++)
                   if ( x+dC[i][0] <= 7 && x+dC[i][0] >= 0 && y+dC[i][1] <= 7 && y+dC[i][1] >= 0 )
                        if ( conf->mat[ x+dC[i][0] ] [ y+dC[i][1] ] >= 0 )  {
                           copier(conf, &T[*n]);
                           T[*n].mat[x][y] = 0;
                           T[*n].mat[ x+dC[i][0] ][ y+dC[i][1] ] = -'c';
                           // cas où le roi adverse est pris...
                           if (T[*n].xrB == x+dC[i][0] && T[*n].yrB == y+dC[i][1]) { 
                                T[*n].xrB = -1; T[*n].yrB = -1; 
                           }

                           (*n)++;
                        }
                break;

        // mvmt FOU ...
        case -'f' : 
                for (i=1; i<8; i += 2) {
                   // traitement des directions impaires (1, 3, 5 et 7)
                   stop = 0;
                   a = x + D[i][0];
                   b = y + D[i][1];                   
                   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
                        if ( conf->mat[ a ] [ b ] < 0 )  stop = 1;
                        else {
                           copier(conf, &T[*n]);
                           T[*n].mat[x][y] = 0;
                           if ( T[*n].mat[a][b] > 0 ) stop = 1;
                           T[*n].mat[a][b] = -'f';
                           // cas où le roi adverse est pris...
                           if (T[*n].xrB == a && T[*n].yrB == b) { T[*n].xrB = -1; T[*n].yrB = -1; }

                           (*n)++;
                              a = a + D[i][0];
                              b = b + D[i][1];
                        }
                   } // while
                } // for
                break;

        // mvmt TOUR ...
        case -'t' : 
                for (i=0; i<8; i += 2) {
                   // traitement des directions paires (0, 2, 4 et 6)
                   stop = 0;
                   a = x + D[i][0];
                   b = y + D[i][1];                   
                   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
                        if ( conf->mat[ a ] [ b ] < 0 )  stop = 1;
                        else {
                           copier(conf, &T[*n]);
                           T[*n].mat[x][y] = 0;
                           if ( T[*n].mat[a][b] > 0 ) stop = 1;
                           T[*n].mat[a][b] = -'t';
                           // cas où le roi adverse est pris...
                           if (T[*n].xrB == a && T[*n].yrB == b) { T[*n].xrB = -1; T[*n].yrB = -1; }

                           if ( conf->roqueN != 'e' && conf->roqueN != 'n' ) {
                              if ( x == 7 && y == 0 && conf->roqueN != 'p')
                                 // le grand roque ne sera plus possible
                                    T[*n].roqueN = 'g'; 
                              else if ( x == 7 && y == 0 )
                                      // ni le grand roque ni le petit roque ne seront possibles
                                         T[*n].roqueN = 'n'; 

                              if ( x == 7 && y == 7 && conf->roqueN != 'g' )
                                 // le petit roque ne sera plus possible
                                    T[*n].roqueN = 'p'; 
                              else if ( x == 7 && y == 7 )
                                      // ni le grand roque ni le petit roque ne seront possibles
                                         T[*n].roqueN = 'n'; 
                           }

                           (*n)++;
                              a = a + D[i][0];
                              b = b + D[i][1];
                        }
                   } // while
                } // for
                break;

        // mvmt REINE ...
        case -'n' : 
                for (i=0; i<8; i += 1) {
                   // traitement des 8 directions paires et impaires
                   stop = 0;
                   a = x + D[i][0];
                   b = y + D[i][1];                   
                   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
                        if ( conf->mat[ a ] [ b ] < 0 )  stop = 1;
                        else {
                           copier(conf, &T[*n]);
                           T[*n].mat[x][y] = 0;
                           if ( T[*n].mat[a][b] > 0 ) stop = 1;
                           T[*n].mat[a][b] = -'n';
                           // cas où le roi adverse est pris...
                           if (T[*n].xrB == a && T[*n].yrB == b) { T[*n].xrB = -1; T[*n].yrB = -1; }

                           (*n)++;
                              a = a + D[i][0];
                              b = b + D[i][1];
                        }
                   } // while
                } // for
                break;

        // mvmt ROI ...
        case -'r' : 
                // vérifier possibilité de faire un roque ...
                if ( conf->roqueN != 'n' && conf->roqueN != 'e' ) {
                   if ( conf->roqueN != 'g' && conf->mat[7][1] == 0 \
                        && conf->mat[7][2] == 0 && conf->mat[7][3] == 0 )
                      if ( !caseMenaceePar( MAX, 7, 1, conf ) && !caseMenaceePar( MAX, 7, 2, conf ) && \
                           !caseMenaceePar( MAX, 7, 3, conf ) && !caseMenaceePar( MAX, 7, 4, conf ) )  {
                        // Faire un grand roque ...
                        copier(conf, &T[*n]);
                        T[*n].mat[7][4] = 0;
                        T[*n].mat[7][0] = 0;
                        T[*n].mat[7][2] = -'r'; T[*n].xrN = 7; T[*n].yrN = 2;
                        T[*n].mat[7][3] = -'t';
                        // aucun roque ne sera plus possible à partir de cette config
                        T[*n].roqueN = 'e'; 
                        (*n)++;
                      }
                   if ( conf->roqueN != 'p' && conf->mat[7][5] == 0 && conf->mat[7][6] == 0 )
                      if ( !caseMenaceePar( MAX, 7, 4, conf ) && !caseMenaceePar( MAX, 7, 5, conf ) && \
                           !caseMenaceePar( MAX, 7, 6, conf ) )  {
                        // Faire un petit roque ...
                        copier(conf, &T[*n]);
                        T[*n].mat[7][4] = 0;
                        T[*n].mat[7][7] = 0;
                        T[*n].mat[7][6] = -'r'; T[*n].xrN = 7; T[*n].yrN = 6;
                        T[*n].mat[7][5] = -'t';
                        // aucun roque ne sera plus possible à partir de cette config
                        T[*n].roqueN = 'e'; 
                        (*n)++;
                      }
                }
                        
                // vérifier les autres mouvements du roi ...
                for (i=0; i<8; i += 1) {
                   // traitement des 8 directions paires et impaires
                   a = x + D[i][0];
                   b = y + D[i][1];                   
                   if ( a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
                        if ( conf->mat[a][b] >= 0 ) {
                           copier(conf, &T[*n]);
                           T[*n].mat[x][y] = 0;
                           T[*n].mat[a][b] = -'r'; T[*n].xrN = a; T[*n].yrN = b;
                           // cas où le roi adverse est pris...
                           if (T[*n].xrB == a && T[*n].yrB == b) { 
                              T[*n].xrB = -1; 
                              T[*n].yrB = -1; 
                           }
                           // aucun roque ne sera plus possible à partir de cette config
                           T[*n].roqueN = 'n'; 
                           (*n)++;
                        }
                } // for
                break;

        }
} // fin de deplacementsN


/* Génere dans T tous les coups possibles de la pièce (de couleur B) se trouvant à la pos x,y */
void deplacementsB(struct config *conf, int x, int y, struct config T[], int *n )
{
        int i, j, a, b, stop;

        switch(conf->mat[x][y]) {
        // mvmt PION ...
        case 'p' :  
                if ( x <7 && conf->mat[x+1][y] == 0 ) {        
                        // avance d'une case
                        copier(conf, &T[*n]);
                        T[*n].mat[x][y] = 0;
                        T[*n].mat[x+1][y] = 'p';
                        (*n)++;
                        if ( x == 6 ) transformPion( conf, x, y, x+1, y, T, n );
                }
                if ( x == 1 && conf->mat[2][y] == 0 && conf->mat[3][y] == 0) {        
                        // avance de 2 cases
                        copier(conf, &T[*n]);
                        T[*n].mat[1][y] = 0;
                        T[*n].mat[3][y] = 'p';
                        (*n)++;
                }
                if ( x < 7 && y > 0 && conf->mat[x+1][y-1] < 0 ) {        
                        // attaque à gauche (en montant)
                        copier(conf, &T[*n]);
                        T[*n].mat[x][y] = 0;
                        T[*n].mat[x+1][y-1] = 'p';
                        // cas où le roi adverse est pris...
                        if (T[*n].xrN == x+1 && T[*n].yrN == y-1) { 
                                T[*n].xrN = -1; T[*n].yrN = -1; 
                        }

                        (*n)++;
                        if ( x == 6 ) transformPion( conf, x, y, x+1, y-1, T, n );
                }
                if ( x < 7 && y < 7 && conf->mat[x+1][y+1] < 0 ) {
                        // attaque à droite (en montant)
                        copier(conf, &T[*n]);
                        T[*n].mat[x][y] = 0;
                        T[*n].mat[x+1][y+1] = 'p';
                        // cas où le roi adverse est pris...
                        if (T[*n].xrN == x+1 && T[*n].yrN == y+1) { 
                                T[*n].xrN = -1; T[*n].yrN = -1; 
                        }

                        (*n)++;
                        if ( x == 6 ) transformPion( conf, x, y, x+1, y+1, T, n );
                }
                break;

        // mvmt CAVALIER ...
        case 'c' : 
                for (i=0; i<8; i++)
                   if ( x+dC[i][0] <= 7 && x+dC[i][0] >= 0 && y+dC[i][1] <= 7 && y+dC[i][1] >= 0 )
                        if ( conf->mat[ x+dC[i][0] ] [ y+dC[i][1] ] <= 0 )  {
                           copier(conf, &T[*n]);
                           T[*n].mat[x][y] = 0;
                           T[*n].mat[ x+dC[i][0] ][ y+dC[i][1] ] = 'c';
                           // cas où le roi adverse est pris...
                           if (T[*n].xrN == x+dC[i][0] && T[*n].yrN == y+dC[i][1]) { 
                                T[*n].xrN = -1; 
                                T[*n].yrN = -1; 
                           }

                           (*n)++;
                        }
                break;

        // mvmt FOU ...
        case 'f' : 
                for (i=1; i<8; i += 2) {
                   // traitement des directions impaires (1, 3, 5 et 7)
                   stop = 0;
                   a = x + D[i][0];
                   b = y + D[i][1];                   
                   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
                        if ( conf->mat[ a ] [ b ] > 0 )  stop = 1;
                        else {
                           copier(conf, &T[*n]);
                           T[*n].mat[x][y] = 0;
                           if ( T[*n].mat[a][b] < 0 ) stop = 1;
                           T[*n].mat[a][b] = 'f';
                           // cas où le roi adverse est pris...
                           if (T[*n].xrN == a && T[*n].yrN == b) { 
                                T[*n].xrN = -1; 
                                T[*n].yrN = -1; 
                           }
                           (*n)++;
                              a = a + D[i][0];
                              b = b + D[i][1];
                        }
                   } // while
                } // for
                break;

        // mvmt TOUR ...
        case 't' : 
                for (i=0; i<8; i += 2) {
                   // traitement des directions paires (0, 2, 4 et 6)
                   stop = 0;
                   a = x + D[i][0];
                   b = y + D[i][1];                   
                   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
                        if ( conf->mat[ a ] [ b ] > 0 )  stop = 1;
                        else {
                           copier(conf, &T[*n]);
                           T[*n].mat[x][y] = 0;
                           if ( T[*n].mat[a][b] < 0 ) stop = 1;
                           T[*n].mat[a][b] = 't';
                           // cas où le roi adverse est pris...
                           if (T[*n].xrN == a && T[*n].yrN == b) { 
                                T[*n].xrN = -1; 
                                T[*n].yrN = -1; 
                           }
                           if ( conf->roqueB != 'e' && conf->roqueB != 'n' ) {
                             if ( x == 0 && y == 0 && conf->roqueB != 'p')
                                // le grand roque ne sera plus possible
                                   T[*n].roqueB = 'g'; 
                             else if ( x == 0 && y == 0 )
                                     // ni le grand roque ni le petit roque ne seront possibles
                                        T[*n].roqueB = 'n'; 
                             if ( x == 0 && y == 7 && conf->roqueB != 'g' )
                                // le petit roque ne sera plus possible
                                   T[*n].roqueB = 'p'; 
                             else if ( x == 0 && y == 7 )
                                     // ni le grand roque ni le petit roque ne seront possibles
                                        T[*n].roqueB = 'n'; 
                           }

                           (*n)++;
                              a = a + D[i][0];
                              b = b + D[i][1];
                        }
                   } // while
                } // for
                break;

        // mvmt REINE ...
        case 'n' : 
                for (i=0; i<8; i += 1) {
                   // traitement des 8 directions paires et impaires
                   stop = 0;
                   a = x + D[i][0];
                   b = y + D[i][1];                   
                   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
                        if ( conf->mat[ a ] [ b ] > 0 )  stop = 1;
                        else {
                           copier(conf, &T[*n]);
                           T[*n].mat[x][y] = 0;
                           if ( T[*n].mat[a][b] < 0 ) stop = 1;
                           T[*n].mat[a][b] = 'n';
                           // cas où le roi adverse est pris...
                           if (T[*n].xrN == a && T[*n].yrN == b) { 
                              T[*n].xrN = -1; 
                              T[*n].yrN = -1; 
                           }
                           (*n)++;
                              a = a + D[i][0];
                              b = b + D[i][1];
                        }
                   } // while
                } // for
                break;

        // mvmt ROI ...
        case 'r' : 
                // vérifier possibilité de faire un roque ...
                if ( conf->roqueB != 'n' && conf->roqueB != 'e' ) {
                   if ( conf->roqueB != 'g' && conf->mat[0][1] == 0 && \
                        conf->mat[0][2] == 0 && conf->mat[0][3] == 0 )
                      if ( !caseMenaceePar( MIN, 0, 1, conf ) && !caseMenaceePar( MIN, 0, 2, conf ) && \
                           !caseMenaceePar( MIN, 0, 3, conf ) && !caseMenaceePar( MIN, 0, 4, conf ) )  {
                        // Faire un grand roque ...
                        copier(conf, &T[*n]);
                        T[*n].mat[0][4] = 0;
                        T[*n].mat[0][0] = 0;
                        T[*n].mat[0][2] = 'r'; T[*n].xrB = 0; T[*n].yrB = 2;
                        T[*n].mat[0][3] = 't';
                        // aucun roque ne sera plus possible à partir de cette config
                        T[*n].roqueB = 'e'; 
                        (*n)++;
                      }
                   if ( conf->roqueB != 'p' && conf->mat[0][5] == 0 && conf->mat[0][6] == 0 )
                      if ( !caseMenaceePar( MIN, 0, 4, conf ) && !caseMenaceePar( MIN, 0, 5, conf ) && \
                           !caseMenaceePar( MIN, 0, 6, conf ) )  {
                        // Faire un petit roque ...
                        copier(conf, &T[*n]);
                        T[*n].mat[0][4] = 0;
                        T[*n].mat[0][7] = 0;
                        T[*n].mat[0][6] = 'r'; T[*n].xrB = 0; T[*n].yrB = 6;
                        T[*n].mat[0][5] = 't';
                        // aucun roque ne sera plus possible à partir de cette config
                        T[*n].roqueB = 'e'; 
                        (*n)++;
                      }
                }
                        
                // vérifier les autres mouvements du roi ...
                for (i=0; i<8; i += 1) {
                   // traitement des 8 directions paires et impaires
                   a = x + D[i][0];
                   b = y + D[i][1];                   
                   if ( a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
                        if ( conf->mat[a][b] <= 0 ) {
                           copier(conf, &T[*n]);
                           T[*n].mat[x][y] = 0;
                           T[*n].mat[a][b] = 'r'; T[*n].xrB = a; T[*n].yrB = b;
                           // cas où le roi adverse est pris...
                           if (T[*n].xrN == a && T[*n].yrN == b) { 
                              T[*n].xrN = -1; 
                              T[*n].yrN = -1; 
                           }
                           // aucun roque ne sera plus possible à partir de cette config
                           T[*n].roqueB = 'n'; 
                           (*n)++;
                        }
                } // for
                break;

        }

} // fin de deplacementsB

/***********************************************************************************/


// *****************************
// Partie: Fonctions utilitaires
// ***************************** 


// Vérifie si la case (x,y) est menacée par une des pièces du joueur 'mode'
int caseMenaceePar( int mode, int x, int y, struct config *conf )
{
        int i, j, a, b, stop;

        // menace par le roi ...
        for (i=0; i<8; i += 1) {
           // traitement des 8 directions paires et impaires
           a = x + D[i][0];
           b = y + D[i][1];                   
           if ( a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
                if ( conf->mat[a][b]*mode == 'r' ) return 1;
        } // for

        // menace par cavalier ...
        for (i=0; i<8; i++)
           if ( x+dC[i][0] <= 7 && x+dC[i][0] >= 0 && y+dC[i][1] <= 7 && y+dC[i][1] >= 0 )
                if ( conf->mat[ x+dC[i][0] ] [ y+dC[i][1] ] * mode == 'c' )  
                   return 1;

        // menace par pion ...
        if ( (x-mode) >= 0 && (x-mode) <= 7 && y > 0 && conf->mat[x-mode][y-1]*mode == 'p' )
           return 1;
        if ( (x-mode) >= 0 && (x-mode) <= 7 && y < 7 && conf->mat[x-mode][y+1]*mode == 'p' )
           return 1;

        // menace par fou, tour ou reine ...
        for (i=0; i<8; i += 1) {
           // traitement des 8 directions paires et impaires
           stop = 0;
           a = x + D[i][0];
           b = y + D[i][1];                   
           while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
                if ( conf->mat[a][b] != 0 )  stop = 1;
                else {
                    a = a + D[i][0];
                    b = b + D[i][1];
                }
           if ( stop )  {
                if ( conf->mat[a][b]*mode == 'f' && i % 2 != 0 ) return 1; 
                if ( conf->mat[a][b]*mode == 't' && i % 2 == 0 ) return 1;
                if ( conf->mat[a][b]*mode == 'n' ) return 1;
           }
        } // for

        // sinon, aucune menace ...
        return 0;

} // fin de caseMenaceePar


/* Fonctions de comparaison utilisées avec qsort     */
// 'a' et 'b' sont des configurations
int confcmp123(const void *a, const void *b)        // pour l'ordre croissant
{
    int x = ((struct config *)a)->val, y = ((struct config *)b)->val;
    if ( x < y )
        return -1;
    if ( x == y )
        return 0;
    return 1;
}  // fin confcmp123

int confcmp321(const void *a, const void *b)        // pour l'ordre decroissant
{
    int x = ((struct config *)a)->val, y = ((struct config *)b)->val;
    if ( x < y )
        return 1;
    if ( x == y )
        return 0;
    return -1;
}  // fin confcmp321


/* Le nombre de pièces de N et B dans l'echiquier 'conf' */
int npieces( struct config *conf )
{
    int i,j;
    int nb = 0;
    for (i=0; i<8; i++)
        for (j=0; j<8; j++)
            if ( conf->mat[i][j] != 0 ) nb++;
    return nb; 
}


/* Sauvegarder conf dans le fichier f (pour l'historique) */
void sauvConf( struct config *conf )
{

   char buf[72] = "";
   int i, j;
   for (i=7; i>=0; i--) {
        for (j=0; j<8; j++)
           if (conf->mat[i][j] == 0)  strcat(buf," ");
           else if (conf->mat[i][j] < 0) {
                   strcat(buf, "-");
                   buf[strlen(buf)-1] = -conf->mat[i][j];
                }
                else {
                   strcat(buf, "+");
                   buf[strlen(buf)-1] = conf->mat[i][j] - 32;
                }
        strcat(buf, "\n");
   }
   fprintf(f, "--- coup N° %d ---\n%s\n",num_coup, buf);
   fflush(f);

} // fin sauvConf


/* Intialise la disposition des pieces dans la configuration initiale conf */
void init( struct config *conf )
{
           int i, j;

            for (i=0; i<8; i++)
                for (j=0; j<8; j++)
                        conf->mat[i][j] = 0;        // Les cases vides sont initialisées avec 0

        conf->mat[0][0]='t'; conf->mat[0][1]='c'; conf->mat[0][2]='f'; conf->mat[0][3]='n';
        conf->mat[0][4]='r'; conf->mat[0][5]='f'; conf->mat[0][6]='c'; conf->mat[0][7]='t';

        for (j=0; j<8; j++) {
                conf->mat[1][j] = 'p';
                 conf->mat[6][j] = -'p'; 
                conf->mat[7][j] = -conf->mat[0][j];
        }

        conf->xrB = 0; conf->yrB = 4;
        conf->xrN = 7; conf->yrN = 4;

        conf->roqueB = 'r';
        conf->roqueN = 'r';

        conf->val = 0;

} // fin de init


/* génère un texte décrivant le dernier coup effectué (pour l'affichage) */
void formuler_coup( struct config *oldconf, struct config *newconf, char *coup )
{
        int i,j;
        char piece[20];

        // verifier si roqueB effectué ...
        if ( newconf->roqueB == 'e' && oldconf->roqueB != 'e' ) {
           if ( newconf->yrB == 2 ) sprintf(coup, "g_roqueB" ); 
           else  sprintf(coup, "p_roqueB" ); 
           return;
        }

        // verifier si roqueN effectué ...
        if ( newconf->roqueN == 'e' && oldconf->roqueN != 'e' ) {
           if ( newconf->yrN == 2 ) sprintf(coup, "g_roqueN" ); 
           else  sprintf(coup, "p_roqueN" ); 
           return;
        }

        // Autres mouvements de pièces
        for(i=0; i<8; i++)  
           for (j=0; j<8; j++)
                if ( oldconf->mat[i][j] != newconf->mat[i][j] ) 
                   if ( newconf->mat[i][j] != 0 ) {
                        switch (newconf->mat[i][j]) {
                           case -'p' : sprintf(piece,"pionN"); break;
                           case 'p' : sprintf(piece,"pionB"); break;
                           case -'c' : sprintf(piece,"cavalierN"); break;
                           case 'c' : sprintf(piece,"cavalierB"); break;
                           case -'f' : sprintf(piece,"fouN"); break;
                           case 'f' : sprintf(piece,"fouB"); break;
                           case -'t' : sprintf(piece,"tourN"); break;
                           case 't' : sprintf(piece,"tourB"); break;
                           case -'n' : sprintf(piece,"reineN"); break;
                           case 'n' : sprintf(piece,"reineB"); break;
                           case -'r' : sprintf(piece,"roiN"); break;
                           case 'r' : sprintf(piece,"roiB"); break;
                        }
                        sprintf(coup, "%s en %c%d", piece, 'a'+j, i+1);
                        return;
                   }
} // fin de formuler_coup


/* Affiche la configuration conf */
void affich( struct config *conf, char *coup, int num )
{
        int i, j, k;
        int pB = 0, pN = 0, cB = 0, cN = 0, fB = 0, fN = 0, tB = 0, tN = 0, nB = 0, nN = 0; 

             printf("Coup num:%3d : %s\n", num, coup);
             printf("\n");
        for (i=0;  i<8; i++)
                printf("\t   %c", i+'a');
           printf("\n");

        for (i=0;  i<8; i++)
                printf("\t-------");
           printf("\n");

        for(i=8; i>0; i--)  {
                printf("    %d", i);
                for (j=0; j<8; j++) {
                        if ( conf->mat[i-1][j] < 0 ) printf("\t  %cN", -conf->mat[i-1][j]);
                        else if ( conf->mat[i-1][j] > 0 ) printf("\t  %cB", conf->mat[i-1][j]);
                                  else printf("\t   ");
                        switch (conf->mat[i-1][j]) {
                            case -'p' : pN++; break;
                            case 'p'  : pB++; break;
                            case -'c' : cN++; break;
                            case 'c'  : cB++; break;
                            case -'f' : fN++; break;
                            case 'f'  : fB++; break;
                            case -'t' : tN++; break;
                            case 't'  : tB++; break;
                            case -'n' : nN++; break;
                            case 'n'  : nB++; break;
                        }
                }
                printf("\n");

                for (k=0;  k<8; k++)
                        printf("\t-------");
                   printf("\n");

        }
        printf("\n\tB : p(%d) c(%d) f(%d) t(%d) n(%d) \t N : p(%d) c(%d) f(%d) t(%d) n(%d)\n\n",
                pB, cB, fB, tB, nB, pN, cN, fN, tN, nN);
        printf("\n");

} // fin de  affich


/* Copie la configuration c1 dans c2  */
void copier( struct config *c1, struct config *c2 ) 
{
        int i, j;

        for (i=0; i<8; i++)
                for (j=0; j<8; j++)
                        c2->mat[i][j] = c1->mat[i][j];

        c2->val = c1->val;
        c2->xrB = c1->xrB;
        c2->yrB = c1->yrB;
        c2->xrN = c1->xrN;
        c2->yrN = c1->yrN;

        c2->roqueB = c1->roqueB;
        c2->roqueN = c1->roqueN;
} // fin de copier


/* Teste si les échiquiers c1 et c2 sont égaux */
int egal(char c1[8][8], char c2[8][8] )
{
        int i, j;

        for (i=0; i<8; i++)
                for (j=0; j<8; j++)
                        if (c1[i][j] != c2[i][j]) return 0;
        return 1;
} // fin de egal


/* Teste si conf a déjà été jouée (dans le tableau partie) */
int dejaVisitee( struct config *conf )
{
   int i = 0;
   int trouv = 0;
   while ( i < MAXPARTIE && trouv == 0 ) 
        if ( egal( conf->mat, Partie[i].mat ) ) trouv = i+1;
        else i++;
   return trouv;
}


/* Teste s'il n'y a aucun coup possible dans la configuration conf */
int AucunCoupPossible( struct config *conf )
{
   // ... A completer pour les matchs nuls
   // ... vérifier que generer_succ retourne 0 configurations filles ...
   struct config T[100];
   int n_max = 0, n_min = 0;

   generer_succ( conf, MAX, T, &n_max );
   generer_succ( conf, MIN, T, &n_min );
   // ... ou qu'une même configuration a été générée plusieurs fois ...
   if(n_max == 0 || n_min == 0)
      return 1;
   else
      return 0;
} // fin de AucunCoupPossible

/***********************************************************************************/

