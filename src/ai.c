#include "ai.h"
#include "io.h"
#include "logger.h"
#include <float.h>

static float ai_evaluate_position(const Chess_Context* chess_ctx, Chess_Color color) {
    float evaluation = 0.0f;

    for (int y = 0; y < CHESS_BOARD_HEIGHT; ++y) {
        for (int x = 0; x < CHESS_BOARD_WIDTH; ++x) {
            const Chess_Piece* piece = &chess_ctx->board[y][x];
            if (piece->type != CHESS_PIECE_EMPTY && piece->color == color) {
                switch (piece->type) {
                    case CHESS_PIECE_KING: evaluation += 1000.0f;
                    case CHESS_PIECE_QUEEN: evaluation += 9.0f;
                    case CHESS_PIECE_ROOK: evaluation += 5.0f;
                    case CHESS_PIECE_BISHOP: evaluation += 3.0f;
                    case CHESS_PIECE_KNIGHT: evaluation += 5.0f;
                    case CHESS_PIECE_PAWN: evaluation += 1.0f;
                }
            } else if (piece->type != CHESS_PIECE_EMPTY && piece->color != color) {
                switch (piece->type) {
                    case CHESS_PIECE_KING: evaluation -= 1000.0f;
                    case CHESS_PIECE_QUEEN: evaluation -= 9.0f;
                    case CHESS_PIECE_ROOK: evaluation -= 5.0f;
                    case CHESS_PIECE_BISHOP: evaluation -= 3.0f;
                    case CHESS_PIECE_KNIGHT: evaluation -= 5.0f;
                    case CHESS_PIECE_PAWN: evaluation -= 1.0f;
                }
            }
        }
    }

    return evaluation;
}

static float min(const Chess_Context* chess_ctx, Chess_Color color, Chess_Move* chosen_move, int depth);
static float max(const Chess_Context* chess_ctx, Chess_Color color, Chess_Move* chosen_move, int depth);

static float max(const Chess_Context* chess_ctx, Chess_Color color, Chess_Move* chosen_move, int depth) {
    Chess_Context auxiliar_ctx;
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH];
    int available_moves_num;
    float max = -FLT_MAX;
    float current_max;

    for (int y = 0; y < CHESS_BOARD_HEIGHT; ++y) {
        for (int x = 0; x < CHESS_BOARD_WIDTH; ++x) {
            const Chess_Piece* piece = &chess_ctx->board[y][x];
            if (piece->type != CHESS_PIECE_EMPTY && piece->color == chess_ctx->current_turn) {
                available_moves_num = chess_available_moves_get(chess_ctx, CHESS_POS(y, x), available_moves);
                for (int k = 0; k < available_moves_num; ++k) {
                    chess_move_piece(chess_ctx, &auxiliar_ctx, &available_moves[k]);
                    if (depth > 1) {
                        current_max = min(&auxiliar_ctx, color, 0, depth - 1);
                    } else {
                        current_max = ai_evaluate_position(&auxiliar_ctx, color);
                    }

                    if (current_max >= max) {
                        max = current_max;
                        if (chosen_move) *chosen_move = available_moves[k];
                    }
                }
            }
        }
    }

    return max;
}

static float min(const Chess_Context* chess_ctx, Chess_Color color, Chess_Move* chosen_move, int depth) {
    Chess_Context auxiliar_ctx;
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH];
    int available_moves_num;
    float min = FLT_MAX;
    float current_min;

    for (int y = 0; y < CHESS_BOARD_HEIGHT; ++y) {
        for (int x = 0; x < CHESS_BOARD_WIDTH; ++x) {
            const Chess_Piece* piece = &chess_ctx->board[y][x];
            if (piece->type != CHESS_PIECE_EMPTY && piece->color == chess_ctx->current_turn) {
                available_moves_num = chess_available_moves_get(chess_ctx, CHESS_POS(y, x), available_moves);
                for (int k = 0; k < available_moves_num; ++k) {
                    chess_move_piece(chess_ctx, &auxiliar_ctx, &available_moves[k]);
                    if (depth > 1) {
                        current_min = max(&auxiliar_ctx, color, 0, depth - 1);
                    } else {
                        current_min = ai_evaluate_position(&auxiliar_ctx, color);
                    }

                    if (current_min <= min) {
                        min = current_min;
                        if (chosen_move) *chosen_move = available_moves[k];
                    }
                }
            }
        }
    }

    return min;
}

void ai_get_best_move(const Chess_Context* chess_ctx, char* move_str) {
    Chess_Move chosen_move;
    float evaluation = max(chess_ctx, chess_ctx->current_turn, &chosen_move, 4);
    io_move_to_uci_notation(&chosen_move, move_str);
    log_debug("best move is %s, with evaluation of %.3f", move_str, evaluation);
}

void ai_get_random_move(const Chess_Context* chess_ctx, char* move_str) {
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH];
    int available_moves_num = 0;

    #if 0
    for (int y = 0; y < CHESS_BOARD_HEIGHT; ++y) {
        for (int x = 0; x < CHESS_BOARD_WIDTH; ++x) {
            Chess_Piece* piece = &chess_ctx->board[y][x];
            if (piece->type != CHESS_PIECE_EMPTY) {
                available_moves_num += available_moves_get(chess_ctx, CHESS_POS(y, x), available_moves + available_moves_num);
            }
        }
    }
    #endif

    //////////////// TEST ////////////////

    const Chess_Piece* piece = &chess_ctx->board[0][4];
    if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_WHITE) {
        available_moves_num = chess_available_moves_get(chess_ctx, CHESS_POS(0, 4), available_moves);
        for (int i = 0; i < available_moves_num; ++i) {
            Chess_Move current = available_moves[i];
            if (current.to.x == 6 && current.to.y == 0) {
                log_debug("white short castling is available.");
            } else if (current.to.x == 2 && current.to.y == 0) {
                log_debug("white long castling is available.");
            }
        }
    }

    piece = &chess_ctx->board[7][4];
    if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_BLACK) {
        available_moves_num = chess_available_moves_get(chess_ctx, CHESS_POS(7, 4), available_moves);
        for (int i = 0; i < available_moves_num; ++i) {
            Chess_Move current = available_moves[i];
            if (current.to.x == 6 && current.to.y == 7) {
                log_debug("black short castling is available.");
            } else if (current.to.x == 2 && current.to.y == 7) {
                log_debug("black long castling is available.");
            }
        }
    }

    available_moves_num = 0;

    ////////////////////////////////////

    Chess_Move move;
    while (1) {
        int h = rand() % CHESS_BOARD_HEIGHT;
        int w = rand() % CHESS_BOARD_WIDTH;
        Chess_Board_Position pos = CHESS_POS(h, w);
        if (chess_ctx->board[pos.y][pos.x].type != CHESS_PIECE_EMPTY &&
            chess_ctx->board[pos.y][pos.x].color == chess_ctx->current_turn) {
            available_moves_num = chess_available_moves_get(chess_ctx, pos, available_moves);
            if (available_moves_num > 0) {
                int r = rand() % available_moves_num;
                move = available_moves[r];
                break;
            }
        }
    }

    io_move_to_uci_notation(&move, move_str);
}