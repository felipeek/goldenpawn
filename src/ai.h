#ifndef GOLDENPAWN_AI_H
#define GOLDENPAWN_AI_H
#include "chess.h"

void ai_get_best_move(const Chess_Context* chess_ctx, char* move);
void ai_get_random_move(const Chess_Context* chess_ctx, char* move_str);

#endif