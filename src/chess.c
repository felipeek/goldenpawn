#include "chess.h"
#include "logger.h"
#include <string.h>
#include <assert.h>

static int available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed, int discard_non_attacking_moves);

static Chess_Move chess_move_with_promotion(Chess_Board_Position from, Chess_Board_Position to, Chess_Piece_Type promote_to) {
    Chess_Move move;
    move.from = from;
    move.to = to;
    move.will_promote = 1;
    move.promotion_type = promote_to;
    return move;
}

static Chess_Move chess_move_no_promotion(Chess_Board_Position from, Chess_Board_Position to) {
    Chess_Move move;
    move.from = from;
    move.to = to;
    move.will_promote = 0;
    return move;
}

static void king_positions_fill(Chess_Context* chess_ctx) {
    int found_white_king = 0, found_black_king = 0;
    for (int y = 0; y < CHESS_BOARD_HEIGHT && !(found_white_king && found_black_king); ++y) {
        for (int x = 0; x < CHESS_BOARD_WIDTH && !(found_white_king && found_black_king); ++x) {
            Chess_Piece* current_piece = &chess_ctx->board[y][x];
            if (current_piece->type == CHESS_PIECE_KING) {
                if (current_piece->color == CHESS_COLOR_WHITE) {
                    chess_ctx->white_state.king_position = CHESS_POS(y, x);
                    found_white_king = 1;
                } else {
                    chess_ctx->black_state.king_position = CHESS_POS(y, x);
                    found_black_king = 1;
                }
            }
        }
    }
    assert(found_white_king);
    assert(found_black_king);
}

static Chess_Piece empty_piece() {
    Chess_Piece piece;
    piece.color = CHESS_COLOR_COLORLESS;
    piece.type = CHESS_PIECE_EMPTY;
    return piece;
}

static int is_square_being_attacked(const Chess_Context* chess_ctx, Chess_Board_Position position, Chess_Color by_color) {
    for (int y = 0; y < CHESS_BOARD_HEIGHT; ++y) {
        for (int x = 0; x < CHESS_BOARD_WIDTH; ++x) {
            const Chess_Piece* current_piece = &chess_ctx->board[y][x];
            if (current_piece->type != CHESS_PIECE_EMPTY && current_piece->color == by_color) {
                Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH];
                int available_moves_num = available_moves_get(chess_ctx, CHESS_POS(y, x), available_moves, 1, 1);

                for (int k = 0; k < available_moves_num; ++k) {
                    if (available_moves[k].to.x == position.x && available_moves[k].to.y == position.y) {
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

// Note: this function MUST support chess_ctx == new_ctx !
void chess_move_piece(const Chess_Context* chess_ctx, Chess_Context* new_ctx, const Chess_Move* move) {
    *new_ctx = *chess_ctx;
    Chess_Piece* piece = &new_ctx->board[move->from.y][move->from.x];

    // Update castles
    if (chess_ctx->white_state.short_castling_available) {
        if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_WHITE) {
            // King is being moved.
            new_ctx->white_state.short_castling_available = 0;
        } else if (piece->type == CHESS_PIECE_ROOK && piece->color == CHESS_COLOR_WHITE && move->from.x == 7 && move->from.y == 0) {
            // Rook is being moved.
            new_ctx->white_state.short_castling_available = 0;
        }
    }
    if (chess_ctx->white_state.long_castling_available) {
        if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_WHITE) {
            // King is being moved.
            new_ctx->white_state.long_castling_available = 0;
        } else if (piece->type == CHESS_PIECE_ROOK && piece->color == CHESS_COLOR_WHITE && move->from.x == 0 && move->from.y == 0) {
            // Rook is being moved.
            new_ctx->white_state.long_castling_available = 0;
        }
    }
    if (chess_ctx->black_state.short_castling_available) {
        if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_BLACK) {
            // King is being moved.
            new_ctx->black_state.short_castling_available = 0;
        } else if (piece->type == CHESS_PIECE_ROOK && piece->color == CHESS_COLOR_BLACK && move->from.x == 7 && move->from.y == 7) {
            // Rook is being moved.
            new_ctx->black_state.short_castling_available = 0;
        }
    }
    if (chess_ctx->black_state.long_castling_available) {
        if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_BLACK) {
            // King is being moved.
            new_ctx->black_state.long_castling_available = 0;
        } else if (piece->type == CHESS_PIECE_ROOK && piece->color == CHESS_COLOR_BLACK && move->from.x == 0 && move->from.y == 7) {
            // Rook is being moved.
            new_ctx->black_state.long_castling_available = 0;
        }
    }

    // Update en passant
    if (piece->type == CHESS_PIECE_PAWN && abs(move->to.y - move->from.y) == 2) {
        new_ctx->en_passant_info.available = 1;
        new_ctx->en_passant_info.target = piece->color == CHESS_COLOR_WHITE ? CHESS_POS(move->to.y - 1, move->to.x) : CHESS_POS(move->to.y + 1, move->to.x);
        new_ctx->en_passant_info.pawn_position = move->to;
    } else {
        new_ctx->en_passant_info.available = 0;
    }

    if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_WHITE && move->from.x == 4 && move->from.y == 0 && move->to.x == 6 && move->to.y == 0) {
        // Special case: white castles short
        // We do not check if the rook is there... we trust the GUI.

        // move the king
        new_ctx->board[0][6] = *piece;
        new_ctx->board[0][4] = empty_piece();

        // move the rook
        Chess_Piece* rook = &new_ctx->board[0][7];
        new_ctx->board[0][5] = *rook;
        new_ctx->board[0][7] = empty_piece();
    } else if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_WHITE && move->from.x == 4 && move->from.y == 0 && move->to.x == 2 && move->to.y == 0) {
        // Special case: white castles long
        // We do not check if the rook is there... we trust the GUI.

        // move the king
        new_ctx->board[0][2] = *piece;
        new_ctx->board[0][4] = empty_piece();

        // move the rook
        Chess_Piece* rook = &new_ctx->board[0][0];
        new_ctx->board[0][3] = *rook;
        new_ctx->board[0][0] = empty_piece();
    } else if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_BLACK && move->from.x == 4 && move->from.y == 7 && move->to.x == 6 && move->to.y == 7) {
        // Special case: black castles short
        // We do not check if the rook is there... we trust the GUI.

        // move the king
        new_ctx->board[7][6] = *piece;
        new_ctx->board[7][4] = empty_piece();

        // move the rook
        Chess_Piece* rook = &new_ctx->board[7][7];
        new_ctx->board[7][5] = *rook;
        new_ctx->board[7][7] = empty_piece();
    } else if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_BLACK && move->from.x == 4 && move->from.y == 7 && move->to.x == 2 && move->to.y == 7) {
        // Special case: black castles long
        // We do not check if the rook is there... we trust the GUI.

        // move the king
        new_ctx->board[7][2] = *piece;
        new_ctx->board[7][4] = empty_piece();

        // move the rook
        Chess_Piece* rook = &new_ctx->board[7][0];
        new_ctx->board[7][3] = *rook;
        new_ctx->board[7][0] = empty_piece();
    } else if (piece->type == CHESS_PIECE_PAWN && move->to.x != move->from.x && new_ctx->board[move->to.y][move->to.x].type == CHESS_PIECE_EMPTY) {
        // if we are moving a pawn diagonally to a square that contains no piece, we certainly have an en passant case.
        Chess_Board_Position eaten_pawn_position = piece->color == CHESS_COLOR_WHITE ? CHESS_POS(move->to.y - 1, move->to.x) : CHESS_POS(move->to.y + 1, move->to.x);
        assert(new_ctx->board[eaten_pawn_position.y][eaten_pawn_position.x].type == CHESS_PIECE_PAWN && new_ctx->board[eaten_pawn_position.y][eaten_pawn_position.x].color != piece->color);
        new_ctx->board[eaten_pawn_position.y][eaten_pawn_position.x] = empty_piece();
        new_ctx->board[move->to.y][move->to.x] = *piece;
        new_ctx->board[move->from.y][move->from.x] = empty_piece();
    } else {
        new_ctx->board[move->to.y][move->to.x] = *piece;
        new_ctx->board[move->from.y][move->from.x] = empty_piece();

        if (move->will_promote) {
            new_ctx->board[move->to.y][move->to.x].type = move->promotion_type;
        }
    }

    king_positions_fill(new_ctx);
    new_ctx->white_state.is_king_under_attack = is_square_being_attacked(new_ctx, new_ctx->white_state.king_position, CHESS_COLOR_BLACK);
    new_ctx->black_state.is_king_under_attack = is_square_being_attacked(new_ctx, new_ctx->black_state.king_position, CHESS_COLOR_WHITE);

    new_ctx->current_turn = !new_ctx->current_turn;
}

static int chess_position_is_within_bounds(Chess_Board_Position position) {
    if (position.x >= 0 && position.x < CHESS_BOARD_WIDTH) {
        if (position.y >= 0 && position.y < CHESS_BOARD_HEIGHT) {
            return 1;
        }
    }

    return 0;
}

static int is_king_exposed_if_piece_is_moved(const Chess_Context* chess_ctx, const Chess_Move* move) {
    Chess_Context ctx_without_piece;
    chess_move_piece(chess_ctx, &ctx_without_piece, move);
    const Chess_Piece* piece = &ctx_without_piece.board[move->to.y][move->to.x];
    return piece->color == CHESS_COLOR_WHITE ? ctx_without_piece.white_state.is_king_under_attack : ctx_without_piece.black_state.is_king_under_attack;
}

static void move_to_uci_notation(const Chess_Move* move, char* uci_str) {
    uci_str[0] = move->from.x + 'a';
    uci_str[1] = move->from.y + '0' + 1;
    uci_str[2] = move->to.x + 'a';
    uci_str[3] = move->to.y + '0' + 1;

    if (move->will_promote) {
        switch (move->promotion_type) {
            case CHESS_PIECE_QUEEN: {
                uci_str[4] = 'q';
            } break;
            case CHESS_PIECE_ROOK: {
                uci_str[4] = 'r';
            } break;
            case CHESS_PIECE_BISHOP: {
                uci_str[4] = 'b';
            } break;
            case CHESS_PIECE_KNIGHT: {
                uci_str[4] = 'n';
            } break;
            default: {
                assert(0);
            } break;
        }
        uci_str[5] = '\0';
    } else {
        uci_str[4] = '\0';
    }
}

static void uci_notation_to_move(const char* uci_str, Chess_Move* move) {
    int uci_str_len = strlen(uci_str);
    move->from = CHESS_POS(uci_str[1] - '0' - 1, uci_str[0] - 'a');
    move->to = CHESS_POS(uci_str[3] - '0' - 1, uci_str[2] - 'a');

    // Pawn reached last rank.
    if (uci_str_len == 5) {
        switch(uci_str[4]) {
            case 'b': move->promotion_type = CHESS_PIECE_BISHOP; break;
            case 'n': move->promotion_type = CHESS_PIECE_KNIGHT; break;
            case 'q': move->promotion_type = CHESS_PIECE_QUEEN; break;
            case 'r': move->promotion_type = CHESS_PIECE_ROOK; break;
            default: assert(0);
        }
        move->will_promote = 1;
    } else {
        move->will_promote = 0;
    }
}

static void chess_board_reset(Chess_Context* chess_ctx) {
    memset(chess_ctx->board, 0, CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH * sizeof(Chess_Piece));

    // White Side
    chess_ctx->board[0][0].type = CHESS_PIECE_ROOK;
    chess_ctx->board[0][0].color = CHESS_COLOR_WHITE;
    chess_ctx->board[0][1].type = CHESS_PIECE_KNIGHT;
    chess_ctx->board[0][1].color = CHESS_COLOR_WHITE;
    chess_ctx->board[0][2].type = CHESS_PIECE_BISHOP;
    chess_ctx->board[0][2].color = CHESS_COLOR_WHITE;
    chess_ctx->board[0][3].type = CHESS_PIECE_QUEEN;
    chess_ctx->board[0][3].color = CHESS_COLOR_WHITE;
    chess_ctx->board[0][4].type = CHESS_PIECE_KING;
    chess_ctx->board[0][4].color = CHESS_COLOR_WHITE;
    chess_ctx->board[0][5].type = CHESS_PIECE_BISHOP;
    chess_ctx->board[0][5].color = CHESS_COLOR_WHITE;
    chess_ctx->board[0][6].type = CHESS_PIECE_KNIGHT;
    chess_ctx->board[0][6].color = CHESS_COLOR_WHITE;
    chess_ctx->board[0][7].type = CHESS_PIECE_ROOK;
    chess_ctx->board[0][7].color = CHESS_COLOR_WHITE;
    chess_ctx->board[1][0].type = CHESS_PIECE_PAWN;
    chess_ctx->board[1][0].color = CHESS_COLOR_WHITE;
    chess_ctx->board[1][1].type = CHESS_PIECE_PAWN;
    chess_ctx->board[1][1].color = CHESS_COLOR_WHITE;
    chess_ctx->board[1][2].type = CHESS_PIECE_PAWN;
    chess_ctx->board[1][2].color = CHESS_COLOR_WHITE;
    chess_ctx->board[1][3].type = CHESS_PIECE_PAWN;
    chess_ctx->board[1][3].color = CHESS_COLOR_WHITE;
    chess_ctx->board[1][4].type = CHESS_PIECE_PAWN;
    chess_ctx->board[1][4].color = CHESS_COLOR_WHITE;
    chess_ctx->board[1][5].type = CHESS_PIECE_PAWN;
    chess_ctx->board[1][5].color = CHESS_COLOR_WHITE;
    chess_ctx->board[1][6].type = CHESS_PIECE_PAWN;
    chess_ctx->board[1][6].color = CHESS_COLOR_WHITE;
    chess_ctx->board[1][7].type = CHESS_PIECE_PAWN;
    chess_ctx->board[1][7].color = CHESS_COLOR_WHITE;

    // Black Side
    chess_ctx->board[7][0].type = CHESS_PIECE_ROOK;
    chess_ctx->board[7][0].color = CHESS_COLOR_BLACK;
    chess_ctx->board[7][1].type = CHESS_PIECE_KNIGHT;
    chess_ctx->board[7][1].color = CHESS_COLOR_BLACK;
    chess_ctx->board[7][2].type = CHESS_PIECE_BISHOP;
    chess_ctx->board[7][2].color = CHESS_COLOR_BLACK;
    chess_ctx->board[7][3].type = CHESS_PIECE_QUEEN;
    chess_ctx->board[7][3].color = CHESS_COLOR_BLACK;
    chess_ctx->board[7][4].type = CHESS_PIECE_KING;
    chess_ctx->board[7][4].color = CHESS_COLOR_BLACK;
    chess_ctx->board[7][5].type = CHESS_PIECE_BISHOP;
    chess_ctx->board[7][5].color = CHESS_COLOR_BLACK;
    chess_ctx->board[7][6].type = CHESS_PIECE_KNIGHT;
    chess_ctx->board[7][6].color = CHESS_COLOR_BLACK;
    chess_ctx->board[7][7].type = CHESS_PIECE_ROOK;
    chess_ctx->board[7][7].color = CHESS_COLOR_BLACK;
    chess_ctx->board[6][0].type = CHESS_PIECE_PAWN;
    chess_ctx->board[6][0].color = CHESS_COLOR_BLACK;
    chess_ctx->board[6][1].type = CHESS_PIECE_PAWN;
    chess_ctx->board[6][1].color = CHESS_COLOR_BLACK;
    chess_ctx->board[6][2].type = CHESS_PIECE_PAWN;
    chess_ctx->board[6][2].color = CHESS_COLOR_BLACK;
    chess_ctx->board[6][3].type = CHESS_PIECE_PAWN;
    chess_ctx->board[6][3].color = CHESS_COLOR_BLACK;
    chess_ctx->board[6][4].type = CHESS_PIECE_PAWN;
    chess_ctx->board[6][4].color = CHESS_COLOR_BLACK;
    chess_ctx->board[6][5].type = CHESS_PIECE_PAWN;
    chess_ctx->board[6][5].color = CHESS_COLOR_BLACK;
    chess_ctx->board[6][6].type = CHESS_PIECE_PAWN;
    chess_ctx->board[6][6].color = CHESS_COLOR_BLACK;
    chess_ctx->board[6][7].type = CHESS_PIECE_PAWN;
    chess_ctx->board[6][7].color = CHESS_COLOR_BLACK;
}


void chess_context_from_position_input(Chess_Context* chess_ctx, int argc, const char** argv) {
    if (strcmp(argv[0], "startpos")) {
        log_debug("Error: currently, startpos is needed");
        return;
    }

    if (strcmp(argv[1], "moves")) {
        log_debug("Error: moves expected");
        return;
    }

    chess_board_reset(chess_ctx);

    chess_ctx->white_state.long_castling_available = 1;
    chess_ctx->white_state.short_castling_available = 1;
    chess_ctx->black_state.long_castling_available = 1;
    chess_ctx->black_state.short_castling_available = 1;
    chess_ctx->en_passant_info.available = 0;

    Chess_Move move;
    for (int i = 2; i < argc; ++i) {
        assert(strlen(argv[i]) == 4 || strlen(argv[i]) == 5);
        uci_notation_to_move(argv[i], &move);
        chess_move_piece(chess_ctx, chess_ctx, &move);
    }

    if ((argc - 2) % 2 == 0) {
        chess_ctx->current_turn = CHESS_COLOR_WHITE;
    } else {
        chess_ctx->current_turn = CHESS_COLOR_BLACK;
    }
}

void chess_get_random_move(const Chess_Context* chess_ctx, char* move_str) {
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
        available_moves_num = available_moves_get(chess_ctx, CHESS_POS(0, 4), available_moves, 0, 0);
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
        available_moves_num = available_moves_get(chess_ctx, CHESS_POS(7, 4), available_moves, 0, 0);
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
        if (chess_ctx->board[pos.y][pos.x].type == CHESS_PIECE_PAWN &&
            chess_ctx->board[pos.y][pos.x].color == chess_ctx->current_turn) {
            available_moves_num = available_moves_get(chess_ctx, pos, available_moves, 0, 0);
            if (available_moves_num > 0) {
                int r = rand() % available_moves_num;
                move = available_moves[r];
                break;
            }
        }
    }

    move_to_uci_notation(&move, move_str);
}

static int available_move_add_to_list_if_valid(const Chess_Context* chess_ctx,
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int available_moves_num,
    int need_to_check_if_king_is_exposed, const Chess_Move* move) {
    if (need_to_check_if_king_is_exposed) {
        if (!is_king_exposed_if_piece_is_moved(chess_ctx, move)) {
            available_moves[available_moves_num++] = *move;
        }
    } else {
        available_moves[available_moves_num++] = *move;
    }
    return available_moves_num;
}

static int rook_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_ROOK || current_piece->type == CHESS_PIECE_QUEEN);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);

    int need_to_check_if_king_is_exposed = !king_can_be_exposed;

    Chess_Move candidate_move;

    // Left
    for (int x = position.x - 1; x >= 0; --x) {
        candidate_move = chess_move_no_promotion(position, CHESS_POS(position.y, x));
        if (chess_ctx->board[position.y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        } else if (chess_ctx->board[position.y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
            break;
        } else {
            break;
        }
    }

    // Right
    for (int x = position.x + 1; x < CHESS_BOARD_WIDTH; ++x) {
        candidate_move = chess_move_no_promotion(position, CHESS_POS(position.y, x));
        if (chess_ctx->board[position.y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        } else if (chess_ctx->board[position.y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
            break;
        } else {
            break;
        }
    }

    // Bottom
    for (int y = position.y - 1; y >= 0; --y) {
        candidate_move = chess_move_no_promotion(position, CHESS_POS(y, position.x));
        if (chess_ctx->board[y][position.x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        } else if (chess_ctx->board[y][position.x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
            break;
        } else {
            break;
        }
    }

    // Top
    for (int y = position.y + 1; y < CHESS_BOARD_HEIGHT; ++y) {
        candidate_move = chess_move_no_promotion(position, CHESS_POS(y, position.x));
        if (chess_ctx->board[y][position.x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        } else if (chess_ctx->board[y][position.x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
            break;
        } else {
            break;
        }
    }

    return available_moves_num;
}

static int bishop_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_BISHOP || current_piece->type == CHESS_PIECE_QUEEN);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);

    int need_to_check_if_king_is_exposed = !king_can_be_exposed;

    Chess_Move candidate_move;

    // Top-Right
    for (int x = position.x + 1, y = position.y + 1; x < CHESS_BOARD_HEIGHT && y < CHESS_BOARD_HEIGHT; ++x, ++y) {
        candidate_move = chess_move_no_promotion(position, CHESS_POS(y, x));
        if (chess_ctx->board[y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        } else if (chess_ctx->board[y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
            break;
        } else {
            break;
        }
    }

    // Top-Left
    for (int x = position.x - 1, y = position.y + 1; x >= 0 && y < CHESS_BOARD_HEIGHT; --x, ++y) {
        candidate_move = chess_move_no_promotion(position, CHESS_POS(y, x));
        if (chess_ctx->board[y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        } else if (chess_ctx->board[y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
            break;
        } else {
            break;
        }
    }

    // Bottom-Right
    for (int x = position.x + 1, y = position.y - 1; x < CHESS_BOARD_HEIGHT && y >= 0; ++x, --y) {
        candidate_move = chess_move_no_promotion(position, CHESS_POS(y, x));
        if (chess_ctx->board[y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        } else if (chess_ctx->board[y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
            break;
        } else {
            break;
        }
    }

    // Bottom-Left
    for (int x = position.x - 1, y = position.y - 1; x >= 0 && y >= 0; --x, --y) {
        candidate_move = chess_move_no_promotion(position, CHESS_POS(y, x));
        if (chess_ctx->board[y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        } else if (chess_ctx->board[y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
            break;
        } else {
            break;
        }
    }

    return available_moves_num;
}

static int knight_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_KNIGHT);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);
    
    int need_to_check_if_king_is_exposed = !king_can_be_exposed;

    Chess_Board_Position candidate_position;
    Chess_Move candidate_move;

    candidate_position = CHESS_POS(position.y + 2, position.x + 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        }
    }

    candidate_position = CHESS_POS(position.y + 2, position.x - 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        }
    }

    candidate_position = CHESS_POS(position.y + 1, position.x + 2);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        }
    }

    candidate_position = CHESS_POS(position.y + 1, position.x - 2);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        }
    }

    candidate_position = CHESS_POS(position.y - 1, position.x + 2);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        }
    }

    candidate_position = CHESS_POS(position.y - 1, position.x - 2);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        }
    }

    candidate_position = CHESS_POS(position.y - 2, position.x + 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        }
    }

    candidate_position = CHESS_POS(position.y - 2, position.x - 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        }
    }

    return available_moves_num;
}

static int queen_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_QUEEN);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);

    available_moves_num += rook_available_moves_get(chess_ctx, position, available_moves, king_can_be_exposed);
    available_moves_num += bishop_available_moves_get(chess_ctx, position, available_moves + available_moves_num, king_can_be_exposed);
    return available_moves_num;
}

static int king_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed, int discard_non_attacking_moves) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_KING);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);

    Chess_Move candidate_move;
    Chess_Board_Position candidate_position;

    candidate_position = CHESS_POS(position.y + 1, position.x + 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_piece_is_moved(chess_ctx, &candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_position = CHESS_POS(position.y + 1, position.x);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_piece_is_moved(chess_ctx, &candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_position = CHESS_POS(position.y + 1, position.x - 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_piece_is_moved(chess_ctx, &candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_position = CHESS_POS(position.y, position.x + 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_piece_is_moved(chess_ctx, &candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_position = CHESS_POS(position.y, position.x - 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_piece_is_moved(chess_ctx, &candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_position = CHESS_POS(position.y - 1, position.x + 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_piece_is_moved(chess_ctx, &candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_position = CHESS_POS(position.y - 1, position.x);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_piece_is_moved(chess_ctx, &candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_position = CHESS_POS(position.y - 1, position.x - 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        candidate_move = chess_move_no_promotion(position, candidate_position);
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_piece_is_moved(chess_ctx, &candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    if (!discard_non_attacking_moves) {
        // Castling moves
        if (current_piece->color == CHESS_COLOR_WHITE) {
            if (!chess_ctx->white_state.is_king_under_attack) {
                if (chess_ctx->white_state.short_castling_available) {
                    if (chess_ctx->board[0][5].type == CHESS_PIECE_EMPTY && chess_ctx->board[0][6].type == CHESS_PIECE_EMPTY) {
                        if (!is_square_being_attacked(chess_ctx, CHESS_POS(0, 5), CHESS_COLOR_BLACK) && !is_square_being_attacked(chess_ctx, CHESS_POS(0, 6), CHESS_COLOR_BLACK)) {
                            available_moves[available_moves_num++] = chess_move_no_promotion(position, CHESS_POS(0, 6));
                        }
                    }
                }
                if (chess_ctx->white_state.long_castling_available) {
                    if (chess_ctx->board[0][3].type == CHESS_PIECE_EMPTY && chess_ctx->board[0][2].type == CHESS_PIECE_EMPTY && chess_ctx->board[0][1].type == CHESS_PIECE_EMPTY) {
                        if (!is_square_being_attacked(chess_ctx, CHESS_POS(0, 3), CHESS_COLOR_BLACK) && !is_square_being_attacked(chess_ctx, CHESS_POS(0, 2), CHESS_COLOR_BLACK)) {
                            available_moves[available_moves_num++] = chess_move_no_promotion(position, CHESS_POS(0, 2));
                        }
                    }
                }
            }
        } else {
            if (!chess_ctx->black_state.is_king_under_attack) {
                if (chess_ctx->black_state.short_castling_available) {
                    if (chess_ctx->board[7][5].type == CHESS_PIECE_EMPTY && chess_ctx->board[7][6].type == CHESS_PIECE_EMPTY) {
                        if (!is_square_being_attacked(chess_ctx, CHESS_POS(7, 5), CHESS_COLOR_WHITE) && !is_square_being_attacked(chess_ctx, CHESS_POS(7, 6), CHESS_COLOR_WHITE)) {
                            available_moves[available_moves_num++] = chess_move_no_promotion(position, CHESS_POS(7, 6));
                        }
                    }
                }
                if (chess_ctx->black_state.long_castling_available) {
                    if (chess_ctx->board[7][3].type == CHESS_PIECE_EMPTY && chess_ctx->board[7][2].type == CHESS_PIECE_EMPTY && chess_ctx->board[7][1].type == CHESS_PIECE_EMPTY) {
                        if (!is_square_being_attacked(chess_ctx, CHESS_POS(7, 3), CHESS_COLOR_WHITE) && !is_square_being_attacked(chess_ctx, CHESS_POS(7, 2), CHESS_COLOR_WHITE)) {
                            available_moves[available_moves_num++] = chess_move_no_promotion(position, CHESS_POS(7, 2));
                        }
                    }
                }
            }
        }
    }

    return available_moves_num;
}

static int pawn_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed, int discard_non_attacking_moves) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_PAWN);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);

    int need_to_check_if_king_is_exposed = !king_can_be_exposed;

    Chess_Move candidate_move;
    Chess_Board_Position candidate_position;

    if (!discard_non_attacking_moves) {
        candidate_position = current_piece->color == CHESS_COLOR_WHITE ? CHESS_POS(position.y + 1, position.x) : CHESS_POS(position.y - 1, position.x);
        if (chess_position_is_within_bounds(candidate_position)) {
            const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
            if (candidate_piece->type == CHESS_PIECE_EMPTY) {
                if ((current_piece->color == CHESS_COLOR_WHITE && candidate_position.y == 7) || (current_piece->color == CHESS_COLOR_BLACK && candidate_position.y == 0)) {
                    // If the pawn is moving to the last rank, we need to promote it!
                    candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_QUEEN);
                    available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                        need_to_check_if_king_is_exposed, &candidate_move);
                    candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_KNIGHT);
                    available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                        need_to_check_if_king_is_exposed, &candidate_move);
                    candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_ROOK);
                    available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                        need_to_check_if_king_is_exposed, &candidate_move);
                    candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_BISHOP);
                    available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                        need_to_check_if_king_is_exposed, &candidate_move);
                } else {
                    candidate_move = chess_move_no_promotion(position, candidate_position);
                    available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                        need_to_check_if_king_is_exposed, &candidate_move);

                    // Check if we can also advance two squares
                    candidate_position = current_piece->color == CHESS_COLOR_WHITE ? CHESS_POS(position.y + 2, position.x) : CHESS_POS(position.y - 2, position.x);
                    int is_pawn_first_move = current_piece->color == CHESS_COLOR_WHITE ? position.y == 1 : position.y == CHESS_BOARD_HEIGHT - 2;
                    if (is_pawn_first_move) {
                        candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
                        candidate_move = chess_move_no_promotion(position, candidate_position);
                        if (candidate_piece->type == CHESS_PIECE_EMPTY) {
                            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                                need_to_check_if_king_is_exposed, &candidate_move);
                        }
                    }
                }
            }
        }
    }

    candidate_position = current_piece->color == CHESS_COLOR_WHITE ? CHESS_POS(position.y + 1, position.x + 1) : CHESS_POS(position.y - 1, position.x + 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        if (candidate_piece->type != CHESS_PIECE_EMPTY && candidate_piece->color != current_piece->color) {
            if ((current_piece->color == CHESS_COLOR_WHITE && candidate_position.y == 7) || (current_piece->color == CHESS_COLOR_BLACK && candidate_position.y == 0)) {
                candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_QUEEN);
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, &candidate_move);
                candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_KNIGHT);
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, &candidate_move);
                candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_ROOK);
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, &candidate_move);
                candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_BISHOP);
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, &candidate_move);
            } else {
                candidate_move = chess_move_no_promotion(position, candidate_position);
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, &candidate_move);
            }
        } else if (candidate_piece->type == CHESS_PIECE_EMPTY && chess_ctx->en_passant_info.available &&
            chess_ctx->en_passant_info.target.y == candidate_position.y && chess_ctx->en_passant_info.target.x == candidate_position.x){
            // EN PASSANT
            candidate_move = chess_move_no_promotion(position, candidate_position);
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        }
    }
    
    candidate_position = current_piece->color == CHESS_COLOR_WHITE ? CHESS_POS(position.y + 1, position.x - 1) : CHESS_POS(position.y - 1, position.x - 1);
    if (chess_position_is_within_bounds(candidate_position)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_position.y][candidate_position.x];
        if (candidate_piece->type != CHESS_PIECE_EMPTY && candidate_piece->color != current_piece->color) {
            if ((current_piece->color == CHESS_COLOR_WHITE && candidate_position.y == 7) || (current_piece->color == CHESS_COLOR_BLACK && candidate_position.y == 0)) {
                candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_QUEEN);
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, &candidate_move);
                candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_KNIGHT);
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, &candidate_move);
                candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_ROOK);
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, &candidate_move);
                candidate_move = chess_move_with_promotion(position, candidate_position, CHESS_PIECE_BISHOP);
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, &candidate_move);
            } else {
                candidate_move = chess_move_no_promotion(position, candidate_position);
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, &candidate_move);
            }
        } else if (candidate_piece->type == CHESS_PIECE_EMPTY && chess_ctx->en_passant_info.available &&
            chess_ctx->en_passant_info.target.y == candidate_position.y && chess_ctx->en_passant_info.target.x == candidate_position.x){
            // EN PASSANT
            candidate_move = chess_move_no_promotion(position, candidate_position);
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, &candidate_move);
        }
    }

    return available_moves_num;
}

static int available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Move available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed, int discard_non_attacking_moves) {
    const Chess_Piece* piece = &chess_ctx->board[position.y][position.x];
    assert(piece->color != CHESS_COLOR_COLORLESS);
    assert(piece->type != CHESS_PIECE_EMPTY);

    switch(piece->type) {
        case CHESS_PIECE_KING: return king_available_moves_get(chess_ctx, position, available_moves, king_can_be_exposed, discard_non_attacking_moves);
        case CHESS_PIECE_QUEEN: return queen_available_moves_get(chess_ctx, position, available_moves, king_can_be_exposed);
        case CHESS_PIECE_ROOK: return rook_available_moves_get(chess_ctx, position, available_moves, king_can_be_exposed);
        case CHESS_PIECE_BISHOP: return bishop_available_moves_get(chess_ctx, position, available_moves, king_can_be_exposed);
        case CHESS_PIECE_KNIGHT: return knight_available_moves_get(chess_ctx, position, available_moves, king_can_be_exposed);
        case CHESS_PIECE_PAWN: return pawn_available_moves_get(chess_ctx, position, available_moves, king_can_be_exposed, discard_non_attacking_moves);
        default: assert(0);
    }
}