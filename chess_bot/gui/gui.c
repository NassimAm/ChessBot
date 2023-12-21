#include "../../SDL2/include/SDL.h"
#include "../../SDL2/include/SDL_image.h"
#include "gui.h"
#include "../chess_engine/jeu.h"
#include <stdio.h>

SDL_Texture* piecesTextures[NB_PIECES_TEXTURES];
SDL_Texture* activeSquareTexture;
SDL_Texture* moveIndicatorTexture;

// Fonction qui initialise SDL
void initSDL(App *app)
{
	int rendererFlags, windowFlags;

	rendererFlags = SDL_RENDERER_ACCELERATED;

	windowFlags = 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

    if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
        printf("Error SDL2_image Initialization");
        exit(1);
    }

	app->window = SDL_CreateWindow("Chess 1.0", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);

	if (!app->window)
	{
		printf("Failed to open %d x %d window: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());
		exit(1);
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	app->renderer = SDL_CreateRenderer(app->window, -1, rendererFlags);

	if (!app->renderer)
	{
		printf("Failed to create renderer: %s\n", SDL_GetError());
		exit(1);
	}
}

// Capture tout entrée de l'utilisateur
void doInput(int *selectedX, int *selectedY)
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				exit(0);
				break;

			case SDL_MOUSEBUTTONDOWN:
				// printf("%d %d\n", event.button.x, event.button.y);
				if(event.button.x >= BOARD_POS_X + BOARD_OFFSET_X && event.button.x <= BOARD_POS_X + BOARD_OFFSET_X + 8*GRID_SIZE
				&& event.button.y >= BOARD_POS_Y + BOARD_OFFSET_Y && event.button.y <= BOARD_POS_Y + BOARD_OFFSET_Y + 8*GRID_SIZE)
				{
					*selectedX = (int)((event.button.x - (BOARD_POS_X + BOARD_OFFSET_X)) / GRID_SIZE);
					*selectedY = 8 - ((int)((event.button.y - (BOARD_POS_Y + BOARD_OFFSET_Y)) / GRID_SIZE) + 1);
				}
				else
				{
					*selectedX = -1;
					*selectedY = -1;
				}
				break;

			default:
				break;
		}
	}
}

// Prépare la scène sur le fenêtre
void prepareScene(App *app)
{
	SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);
	SDL_RenderClear(app->renderer);
}

// Affiche la fenêtre
void presentScene(App *app)
{
	SDL_RenderPresent(app->renderer);
}

// Charger une image depuis in fichier
SDL_Texture *loadTexture(App *app, char *filename)
{
	SDL_Texture *texture;

	// SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Loading %s", filename);

	texture = IMG_LoadTexture(app->renderer, filename);

	if (!texture)
    {
        printf("Error loading texture from %s: %s\n", filename, IMG_GetError());
    }

	return texture;
}

// Afficher une entitée chargée dans une position (x,y)
void blit(App *app, Entity e)
{
	SDL_Rect dest;

	dest.x = e.x;
	dest.y = e.y;
	SDL_QueryTexture(e.texture, NULL, NULL, &dest.w, &dest.h);

	SDL_RenderCopy(app->renderer, e.texture, NULL, &dest);
}

// Charge toutes les ressources (images, ...)
void initAssets(App *app){
	piecesTextures[0] = loadTexture(app, "assets/pieces/pawn_white.png");
	piecesTextures[1] = loadTexture(app, "assets/pieces/pawn_black.png");
	piecesTextures[2] = loadTexture(app, "assets/pieces/bishop_white.png");
	piecesTextures[3] = loadTexture(app, "assets/pieces/bishop_black.png");
	piecesTextures[4] = loadTexture(app, "assets/pieces/knight_white.png");
	piecesTextures[5] = loadTexture(app, "assets/pieces/knight_black.png");
	piecesTextures[6] = loadTexture(app, "assets/pieces/rook_white.png");
	piecesTextures[7] = loadTexture(app, "assets/pieces/rook_black.png");
	piecesTextures[8] = loadTexture(app, "assets/pieces/queen_white.png");
	piecesTextures[9] = loadTexture(app, "assets/pieces/queen_black.png");
	piecesTextures[10] = loadTexture(app, "assets/pieces/king_white.png");
	piecesTextures[11] = loadTexture(app, "assets/pieces/king_black.png");
	activeSquareTexture = loadTexture(app, "assets/active_square.png");
	moveIndicatorTexture = loadTexture(app, "assets/move_indicator.png");
}

// Retourne la texture qui correspond au type et la couleur de la pièce
SDL_Texture *getTextureFromPieceType(int t)
{
	switch (abs(t))
	{
		case 'p':
			if(t > 0)
				return piecesTextures[0];
			else
				return piecesTextures[1];
		case 'f':
			if(t > 0)
				return piecesTextures[2];
			else
				return piecesTextures[3];
		case 'c':
			if(t > 0)
				return piecesTextures[4];
			else
				return piecesTextures[5];
		case 't':
			if(t > 0)
				return piecesTextures[6];
			else
				return piecesTextures[7];
		case 'n':
			if(t > 0)
				return piecesTextures[8];
			else
				return piecesTextures[9];
		case 'r':
			if(t > 0)
				return piecesTextures[10];
			else
				return piecesTextures[11];
		default:
			if(t > 0)
				return piecesTextures[0];
			else
				return piecesTextures[1];
	}
}

// Afficher une conf sur l'équichier
void setBoardConf(App *app, Board *board, struct config conf){
	for(int i=0; i<8; i++)
	{
		for(int j=0; j<8; j++)
		{
			if(conf.mat[i][j] != 0)
			{
				board->pieces[i][j].x = BOARD_POS_X + BOARD_OFFSET_X + GRID_OFFSET_X + GRID_SIZE*j;
				board->pieces[i][j].y = BOARD_POS_Y + BOARD_OFFSET_Y + GRID_OFFSET_Y + GRID_SIZE*(8-(i+1));
				board->pieces[i][j].texture = getTextureFromPieceType(conf.mat[i][j]);
			}
			else
			{
				board->pieces[i][j].x = 0;
				board->pieces[i][j].y = 0;
				board->pieces[i][j].texture = NULL;
			}
		}
	}
}

// Charge l'échiquier sur une configuration initiale
Board initBoard(App *app, struct config conf)
{
	// Allocation de l'échiquier
	Board board;
	board.entity.x = BOARD_POS_X;
	board.entity.y = BOARD_POS_Y;
	board.entity.texture = loadTexture(app, "assets/board.jpg");
	
	// Initialiser les ressources
	initAssets(app);

	setBoardConf(app, &board, conf);

	return board;
}

// Afficher l'échiquier et ses pièces
void drawBoard(App *app, Board board, struct config conf, int selectedX, int selectedY){

	blit(app, board.entity);

	for(int i=0; i<8; i++)
	{
		for(int j=0; j<8; j++)
		{
			if(board.pieces[i][j].texture)
			{
				// Si la pièce est sélectionnée, l'achiffer comme étant activée
				if(j == selectedX && i == selectedY){
					// Marquer la pièce active
					Entity e;
					e.x = BOARD_POS_X + BOARD_OFFSET_X + GRID_SIZE*j;
					e.y = BOARD_POS_Y + BOARD_OFFSET_Y + GRID_SIZE*(8-(i+1));
					e.texture = activeSquareTexture;
					blit(app, e);
				}

				// Afficher la pièce
				blit(app, board.pieces[i][j]);

				// La pièce est sélectionnée
				if(j == selectedX && i == selectedY){
					// Récupérer les mouvement légaux
					struct config T[50];
					int n = 0;
					if(conf.mat[i][j] > 0){
						deplacementsB(&conf, i, j, T, &n);
					}
					else{
						deplacementsN(&conf, i, j, T, &n);
					}

					for(int k=0; k<n; k++){
						for(int ki=0; ki< 8; ki++){
							for(int kj=0; kj < 8; kj++){
								if(T[k].mat[ki][kj] == conf.mat[i][j] && T[k].mat[ki][kj] != conf.mat[ki][kj]){
									Entity e;
									e.x = BOARD_POS_X + BOARD_OFFSET_X + GRID_SIZE*kj + (GRID_SIZE - MOVE_INDICATOR_SIZE)/2;
									e.y = BOARD_POS_Y + BOARD_OFFSET_Y + GRID_SIZE*(8-(ki+1)) + (GRID_SIZE - MOVE_INDICATOR_SIZE)/2;
									e.texture = moveIndicatorTexture;
									blit(app, e);
								}
							}
						}
					}
				}
			}
		}
	}
}