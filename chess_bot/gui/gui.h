#include "../../SDL2/include/SDL.h"
#include "../../SDL2/include/SDL_image.h"
#include "../chess_engine/jeu.h"
#include "../../SDL2/include/SDL_ttf.h"
#include <stdio.h>

#ifndef GUI_H
#define GUI_H

#define SCREEN_WIDTH   650
#define SCREEN_HEIGHT  770

#define BOARD_POS_X 0
#define BOARD_POS_Y 61
#define BOARD_SIZE 648
#define BOARD_OFFSET_X 8
#define BOARD_OFFSET_Y 8

#define NB_PIECES_TEXTURES 12
#define GRID_SIZE 79
#define GRID_OFFSET_X 3
#define GRID_OFFSET_Y 3

#define MOVE_INDICATOR_SIZE 26

// Structure générale pour tout affichage graphique
typedef struct {
	SDL_Renderer *renderer;
	SDL_Window *window;
} App;

// Structure pour représenter une entitée visuelle
typedef struct {
	int x;
	int y;
	SDL_Texture *texture;
} Entity;

// Strucutre pour représenter l'échiquier graphiquement
typedef struct {
	Entity pieces[8][8]; // Matrice de pièces
	Entity entity; // échiqier
} Board;

// Fonction qui initialise SDL
void initSDL(App *app);

// Capture tout entrée de l'utilisateur
void doInput(int *selectedX, int *selectedY, int *moveToX, int *moveToY);

// Prépare la scène sur le fenêtre
void prepareScene(App *app);

// Affiche la fenêtre
void presentScene(App *app);

// Charger une image depuis in fichier
SDL_Texture *loadTexture(App *app ,char *filename);

// Charger un font depuis un fichier
TTF_Font *loadFont(App *app, char *filename, int fontSize);

// Afficher une entitée chargée dans une position (x,y)
void blit(App *app, Entity e);

// Charge toutes les ressources (images, ...)
void initAssets(App *app);

// Affiche un texte sur l'écran dans la position voulue
void drawText(App *app, char *text, int x, int y);

// Retourne la texture qui correspond au type et la couleur de la pièce
SDL_Texture *getTextureFromPieceType(int t);

// Afficher une conf sur l'équichier
void setBoardConf(App *app, Board *board, struct config conf);

// Charge l'échiquier sur une configuration initiale
Board initBoard(App *app, struct config conf);

// Afficher l'échiquier et ses pièces
void drawBoard(App *app, Board board, struct config conf, int *selectedX, int *selectedY);

#endif