#ifndef GOLDENPAWN_CHESS_H
#define GOLDENPAWN_CHESS_H

#define CHESS_BOARD_HEIGHT 8
#define CHESS_BOARD_WIDTH 8

#define CHESS_POS(y,x) (Chess_Board_Position){y,x}

typedef struct {
    int y;
    int x;
} Chess_Board_Position;

typedef enum {
    CHESS_COLOR_COLORLESS = 0,
    CHESS_COLOR_WHITE = 1,
    CHESS_COLOR_BLACK = 2
} Chess_Color;

typedef enum {
    CHESS_PIECE_EMPTY = 0,
    CHESS_PIECE_KING = 1,
    CHESS_PIECE_QUEEN = 2,
    CHESS_PIECE_KNIGHT = 3,
    CHESS_PIECE_BISHOP = 4,
    CHESS_PIECE_ROOK = 5,
    CHESS_PIECE_PAWN = 6
} Chess_Piece_Type;

typedef struct {
    Chess_Piece_Type type;
    Chess_Color color;
} Chess_Piece;

typedef struct {
    Chess_Board_Position king_position;
    int is_king_under_attack;
    int short_castling_available;
    int long_castling_available;
} Chess_Color_State;

typedef struct {
    Chess_Piece board[CHESS_BOARD_HEIGHT][CHESS_BOARD_WIDTH];
    Chess_Color current_turn;
    Chess_Color_State white_state;
    Chess_Color_State black_state;
} Chess_Context;

void chess_context_from_position_input(Chess_Context* chess_ctx, int argc, const char** argv);
void chess_get_random_move(const Chess_Context* chess_ctx, char* move);
void chess_move_piece(const Chess_Context* chess_ctx, Chess_Context* new_ctx, Chess_Board_Position from, Chess_Board_Position to);

#endif