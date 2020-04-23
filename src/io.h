#ifndef GOLDENPAWN_IO_H
#define GOLDENPAWN_IO_H
#include "chess.h"

typedef struct {
    char* buffer;
    char** argv;
    Chess_Context chess_ctx;
} IO_Context;

void io_init(IO_Context* io_ctx);
void io_start(IO_Context* io_ctx);
void io_move_to_uci_notation(const Chess_Move* move, char* uci_str);
void io_uci_notation_to_move(const char* uci_str, Chess_Move* move);

#endif