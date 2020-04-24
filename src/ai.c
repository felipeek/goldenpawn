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

static float alphabeta(const Chess_Context* chess_ctx, Chess_Color color, int depth,
    float alpha, float beta, int maximizing_player, Chess_Move* chosen_move) {
    Chess_Context auxiliar_ctx;
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH];
    int available_moves_num;
    float child_result;

    if (depth == 0) {
        return ai_evaluate_position(chess_ctx, color);
    }
    
    if (maximizing_player) {
        float value = -FLT_MAX;
        int beta_cut_off = 0;

        for (int y = 0; y < CHESS_BOARD_HEIGHT && !beta_cut_off; ++y) {
            for (int x = 0; x < CHESS_BOARD_WIDTH && !beta_cut_off; ++x) {
                const Chess_Piece* piece = &chess_ctx->board[y][x];
                if (piece->type != CHESS_PIECE_EMPTY && piece->color == chess_ctx->current_turn) {
                    available_moves_num = chess_available_moves_get(chess_ctx, CHESS_POS(y, x), available_moves);
                    for (int k = 0; k < available_moves_num && !beta_cut_off; ++k) {
                        chess_move_piece(chess_ctx, &auxiliar_ctx, &available_moves[k]);
                        child_result = alphabeta(&auxiliar_ctx, color, depth - 1, alpha, beta, 0, 0);
                        if (child_result >= value) {
                            value = child_result;
                            if (chosen_move) *chosen_move = available_moves[k];
                        }
                        if (value > alpha) {
                            alpha = value;
                        }
                        if (alpha >= beta) {
                            beta_cut_off = 1;
                        }
                    }
                }
            }
        }

        return value;
    } else {
        float value = FLT_MAX;
        int alpha_cut_off = 0;

        for (int y = 0; y < CHESS_BOARD_HEIGHT && !alpha_cut_off; ++y) {
            for (int x = 0; x < CHESS_BOARD_WIDTH && !alpha_cut_off; ++x) {
                const Chess_Piece* piece = &chess_ctx->board[y][x];
                if (piece->type != CHESS_PIECE_EMPTY && piece->color == chess_ctx->current_turn) {
                    available_moves_num = chess_available_moves_get(chess_ctx, CHESS_POS(y, x), available_moves);
                    for (int k = 0; k < available_moves_num && !alpha_cut_off; ++k) {
                        chess_move_piece(chess_ctx, &auxiliar_ctx, &available_moves[k]);
                        child_result = alphabeta(&auxiliar_ctx, color, depth - 1, alpha, beta, 1, 0);
                        if (child_result <= value) {
                            value = child_result;
                            if (chosen_move) *chosen_move = available_moves[k];
                        }
                        if (value < beta) {
                            beta = value;
                        }
                        if (alpha >= beta) {
                            alpha_cut_off = 1;
                        }
                    }
                }
            }
        }

        return value;
    }
}

void ai_get_best_move(const Chess_Context* chess_ctx, char* move_str) {
    Chess_Move chosen_move;
    float evaluation = alphabeta(chess_ctx, chess_ctx->current_turn, 5, -FLT_MAX, FLT_MAX, 1, &chosen_move);
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