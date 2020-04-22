#include "chess.h"
#include "logger.h"
#include <string.h>
#include <assert.h>

static int available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed, int discard_non_attacking_moves);

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
                Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH];
                int available_moves_num = available_moves_get(chess_ctx, CHESS_POS(y, x), available_moves, 1, 1);

                for (int k = 0; k < available_moves_num; ++k) {
                    if (available_moves[k].x == position.x && available_moves[k].y == position.y) {
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

// Note: this function MUST support chess_ctx == new_ctx !
void chess_move_piece(const Chess_Context* chess_ctx, Chess_Context* new_ctx, Chess_Board_Position from, Chess_Board_Position to) {
    *new_ctx = *chess_ctx;
    Chess_Piece* piece = &new_ctx->board[from.y][from.x];

    // Update castles
    if (chess_ctx->white_state.short_castling_available) {
        if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_WHITE) {
            // King is being moved.
            new_ctx->white_state.short_castling_available = 0;
        } else if (piece->type == CHESS_PIECE_ROOK && piece->color == CHESS_COLOR_WHITE && from.x == 7 && from.y == 0) {
            // Rook is being moved.
            new_ctx->white_state.short_castling_available = 0;
        }
    }
    if (chess_ctx->white_state.long_castling_available) {
        if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_WHITE) {
            // King is being moved.
            new_ctx->white_state.long_castling_available = 0;
        } else if (piece->type == CHESS_PIECE_ROOK && piece->color == CHESS_COLOR_WHITE && from.x == 0 && from.y == 0) {
            // Rook is being moved.
            new_ctx->white_state.long_castling_available = 0;
        }
    }
    if (chess_ctx->black_state.short_castling_available) {
        if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_BLACK) {
            // King is being moved.
            new_ctx->black_state.short_castling_available = 0;
        } else if (piece->type == CHESS_PIECE_ROOK && piece->color == CHESS_COLOR_BLACK && from.x == 7 && from.y == 7) {
            // Rook is being moved.
            new_ctx->black_state.short_castling_available = 0;
        }
    }
    if (chess_ctx->black_state.long_castling_available) {
        if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_BLACK) {
            // King is being moved.
            new_ctx->black_state.long_castling_available = 0;
        } else if (piece->type == CHESS_PIECE_ROOK && piece->color == CHESS_COLOR_BLACK && from.x == 0 && from.y == 7) {
            // Rook is being moved.
            new_ctx->black_state.long_castling_available = 0;
        }
    }

    if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_WHITE && from.x == 4 && from.y == 0 && to.x == 6 && to.y == 0) {
        // Special case: white castles short
        // We do not check if the rook is there... we trust the GUI.

        // move the king
        new_ctx->board[0][6] = *piece;
        new_ctx->board[0][4] = empty_piece();

        // move the rook
        Chess_Piece* rook = &new_ctx->board[0][7];
        new_ctx->board[0][5] = *rook;
        new_ctx->board[0][7] = empty_piece();
    } else if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_WHITE && from.x == 4 && from.y == 0 && to.x == 2 && to.y == 0) {
        // Special case: white castles long
        // We do not check if the rook is there... we trust the GUI.

        // move the king
        new_ctx->board[0][2] = *piece;
        new_ctx->board[0][4] = empty_piece();

        // move the rook
        Chess_Piece* rook = &new_ctx->board[0][0];
        new_ctx->board[0][3] = *rook;
        new_ctx->board[0][0] = empty_piece();
    } else if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_BLACK && from.x == 4 && from.y == 7 && to.x == 6 && to.y == 7) {
        // Special case: black castles short
        // We do not check if the rook is there... we trust the GUI.

        // move the king
        new_ctx->board[7][6] = *piece;
        new_ctx->board[7][4] = empty_piece();

        // move the rook
        Chess_Piece* rook = &new_ctx->board[7][7];
        new_ctx->board[7][5] = *rook;
        new_ctx->board[7][7] = empty_piece();
    } else if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_BLACK && from.x == 4 && from.y == 7 && to.x == 2 && to.y == 7) {
        // Special case: black castles long
        // We do not check if the rook is there... we trust the GUI.

        // move the king
        new_ctx->board[7][2] = *piece;
        new_ctx->board[7][4] = empty_piece();

        // move the rook
        Chess_Piece* rook = &new_ctx->board[7][0];
        new_ctx->board[7][3] = *rook;
        new_ctx->board[7][0] = empty_piece();
    } else {
        new_ctx->board[to.y][to.x] = *piece;
        new_ctx->board[from.y][from.x] = empty_piece();
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

static int is_king_exposed_if_moved_to(const Chess_Context* chess_ctx, Chess_Board_Position king_position, Chess_Board_Position target_position) {
    Chess_Context ctx_king_moved;
    const Chess_Piece* piece = &chess_ctx->board[king_position.y][king_position.x];
    chess_move_piece(chess_ctx, &ctx_king_moved, king_position, target_position);
    return piece->color == CHESS_COLOR_WHITE ? ctx_king_moved.white_state.is_king_under_attack : ctx_king_moved.black_state.is_king_under_attack;
}

static int is_king_exposed_if_piece_is_moved(const Chess_Context* chess_ctx, Chess_Board_Position from, Chess_Board_Position to) {
    Chess_Context ctx_without_piece;
    chess_move_piece(chess_ctx, &ctx_without_piece, from, to);
    const Chess_Piece* piece = &ctx_without_piece.board[to.y][to.x];
    return piece->color == CHESS_COLOR_WHITE ? ctx_without_piece.white_state.is_king_under_attack : ctx_without_piece.black_state.is_king_under_attack;
}

static void position_to_uci_notation(Chess_Board_Position* from, Chess_Board_Position* to, char* uci_str) {
    uci_str[0] = from->x + 'a';
    uci_str[1] = from->y + '0' + 1;
    uci_str[2] = to->x + 'a';
    uci_str[3] = to->y + '0' + 1;
}

static void uci_notation_to_position(const char* uci_str, Chess_Board_Position* from, Chess_Board_Position* to) {
    *from = CHESS_POS(uci_str[1] - '0' - 1, uci_str[0] - 'a');
    *to = CHESS_POS(uci_str[3] - '0' - 1, uci_str[2] - 'a');
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

    Chess_Board_Position from, to;
    for (int i = 2; i < argc; ++i) {
        assert(strlen(argv[i]) == 4);
        uci_notation_to_position(argv[i], &from, &to);
        chess_move_piece(chess_ctx, chess_ctx, from, to);
    }

    if ((argc - 2) % 2 == 0) {
        chess_ctx->current_turn = CHESS_COLOR_WHITE;
    } else {
        chess_ctx->current_turn = CHESS_COLOR_BLACK;
    }
}

void chess_get_random_move(const Chess_Context* chess_ctx, char* move) {
    Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH];
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

    Chess_Board_Position from, to;

    //////////////// TEST ////////////////

    const Chess_Piece* piece = &chess_ctx->board[0][4];
    if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_WHITE) {
        available_moves_num = available_moves_get(chess_ctx, CHESS_POS(0, 4), available_moves, 0, 0);
        for (int i = 0; i < available_moves_num; ++i) {
            Chess_Board_Position current = available_moves[i];
            if (current.x == 6 && current.y == 0) {
                log_debug("white short castling is available.");
            } else if (current.x == 2 && current.y == 0) {
                log_debug("white long castling is available.");
            }
        }
    }

    piece = &chess_ctx->board[7][4];
    if (piece->type == CHESS_PIECE_KING && piece->color == CHESS_COLOR_BLACK) {
        available_moves_num = available_moves_get(chess_ctx, CHESS_POS(7, 4), available_moves, 0, 0);
        for (int i = 0; i < available_moves_num; ++i) {
            Chess_Board_Position current = available_moves[i];
            if (current.x == 6 && current.y == 7) {
                log_debug("black short castling is available.");
            } else if (current.x == 2 && current.y == 7) {
                log_debug("black long castling is available.");
            }
        }
    }

    available_moves_num = 0;

    ////////////////////////////////////

    while (1) {
        int h = rand() % CHESS_BOARD_HEIGHT;
        int w = rand() % CHESS_BOARD_WIDTH;
        from = CHESS_POS(h, w);
        if (chess_ctx->board[from.y][from.x].type != CHESS_PIECE_EMPTY &&
            chess_ctx->board[from.y][from.x].color == chess_ctx->current_turn) {
            available_moves_num = available_moves_get(chess_ctx, from, available_moves, 0, 0);
            if (available_moves_num > 0) {
                int r = rand() % available_moves_num;
                to = available_moves[r];
                break;
            }
        }
    }

    position_to_uci_notation(&from, &to, move);
}

static int available_move_add_to_list_if_valid(const Chess_Context* chess_ctx,
    Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int available_moves_num,
    int need_to_check_if_king_is_exposed, Chess_Board_Position from, Chess_Board_Position to) {
    if (need_to_check_if_king_is_exposed) {
        if (!is_king_exposed_if_piece_is_moved(chess_ctx, from, to)) {
            available_moves[available_moves_num++] = CHESS_POS(to.y, to.x);
        }
    } else {
        available_moves[available_moves_num++] = CHESS_POS(to.y, to.x);
    }
    return available_moves_num;
}

static int rook_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_ROOK || current_piece->type == CHESS_PIECE_QUEEN);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);

    int need_to_check_if_king_is_exposed = !king_can_be_exposed;

    // Left
    for (int x = position.x - 1; x >= 0; --x) {
        if (chess_ctx->board[position.y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(position.y, x));
        } else if (chess_ctx->board[position.y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(position.y, x));
            break;
        } else {
            break;
        }
    }

    // Right
    for (int x = position.x + 1; x < CHESS_BOARD_WIDTH; ++x) {
        if (chess_ctx->board[position.y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(position.y, x));
        } else if (chess_ctx->board[position.y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(position.y, x));
            break;
        } else {
            break;
        }
    }

    // Bottom
    for (int y = position.y - 1; y >= 0; --y) {
        if (chess_ctx->board[y][position.x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, position.x));
        } else if (chess_ctx->board[y][position.x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, position.x));
            break;
        } else {
            break;
        }
    }

    // Top
    for (int y = position.y + 1; y < CHESS_BOARD_HEIGHT; ++y) {
        if (chess_ctx->board[y][position.x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, position.x));
        } else if (chess_ctx->board[y][position.x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, position.x));
            break;
        } else {
            break;
        }
    }

    return available_moves_num;
}

static int bishop_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_BISHOP || current_piece->type == CHESS_PIECE_QUEEN);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);

    int need_to_check_if_king_is_exposed = !king_can_be_exposed;

    // Top-Right
    for (int x = position.x + 1, y = position.y + 1; x < CHESS_BOARD_HEIGHT && y < CHESS_BOARD_HEIGHT; ++x, ++y) {
        if (chess_ctx->board[y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, x));
        } else if (chess_ctx->board[y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, x));
            break;
        } else {
            break;
        }
    }

    // Top-Left
    for (int x = position.x - 1, y = position.y + 1; x >= 0 && y < CHESS_BOARD_HEIGHT; --x, ++y) {
        if (chess_ctx->board[y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, x));
        } else if (chess_ctx->board[y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, x));
            break;
        } else {
            break;
        }
    }

    // Bottom-Right
    for (int x = position.x + 1, y = position.y - 1; x < CHESS_BOARD_HEIGHT && y >= 0; ++x, --y) {
        if (chess_ctx->board[y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, x));
        } else if (chess_ctx->board[y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, x));
            break;
        } else {
            break;
        }
    }

    // Bottom-Left
    for (int x = position.x - 1, y = position.y - 1; x >= 0 && y >= 0; --x, --y) {
        if (chess_ctx->board[y][x].type == CHESS_PIECE_EMPTY) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, x));
        } else if (chess_ctx->board[y][x].color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, CHESS_POS(y, x));
            break;
        } else {
            break;
        }
    }

    return available_moves_num;
}

static int knight_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_KNIGHT);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);
    
    int need_to_check_if_king_is_exposed = !king_can_be_exposed;

    Chess_Board_Position candidate_move;

    candidate_move = CHESS_POS(position.y + 2, position.x + 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, candidate_move);
        }
    }

    candidate_move = CHESS_POS(position.y + 2, position.x - 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, candidate_move);
        }
    }

    candidate_move = CHESS_POS(position.y + 1, position.x + 2);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, candidate_move);
        }
    }

    candidate_move = CHESS_POS(position.y + 1, position.x - 2);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, candidate_move);
        }
    }

    candidate_move = CHESS_POS(position.y - 1, position.x + 2);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, candidate_move);
        }
    }

    candidate_move = CHESS_POS(position.y - 1, position.x - 2);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, candidate_move);
        }
    }

    candidate_move = CHESS_POS(position.y - 2, position.x + 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, candidate_move);
        }
    }

    candidate_move = CHESS_POS(position.y - 2, position.x - 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, candidate_move);
        }
    }

    return available_moves_num;
}

static int queen_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_QUEEN);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);

    available_moves_num += rook_available_moves_get(chess_ctx, position, available_moves, king_can_be_exposed);
    available_moves_num += bishop_available_moves_get(chess_ctx, position, available_moves + available_moves_num, king_can_be_exposed);
    return available_moves_num;
}

static int king_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed, int discard_non_attacking_moves) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_KING);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);

    Chess_Board_Position candidate_move;

    candidate_move = CHESS_POS(position.y + 1, position.x + 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_moved_to(chess_ctx, position, candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_move = CHESS_POS(position.y + 1, position.x);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_moved_to(chess_ctx, position, candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_move = CHESS_POS(position.y + 1, position.x - 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_moved_to(chess_ctx, position, candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_move = CHESS_POS(position.y, position.x + 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_moved_to(chess_ctx, position, candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_move = CHESS_POS(position.y, position.x - 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_moved_to(chess_ctx, position, candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_move = CHESS_POS(position.y - 1, position.x + 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_moved_to(chess_ctx, position, candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_move = CHESS_POS(position.y - 1, position.x);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_moved_to(chess_ctx, position, candidate_move)) {
                available_moves[available_moves_num++] = candidate_move;
            }
        }
    }

    candidate_move = CHESS_POS(position.y - 1, position.x - 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type == CHESS_PIECE_EMPTY || candidate_piece->color != current_piece->color) {
            if (king_can_be_exposed || !is_king_exposed_if_moved_to(chess_ctx, position, candidate_move)) {
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
                            available_moves[available_moves_num++] = CHESS_POS(0, 6);
                        }
                    }
                }
                if (chess_ctx->white_state.long_castling_available) {
                    if (chess_ctx->board[0][3].type == CHESS_PIECE_EMPTY && chess_ctx->board[0][2].type == CHESS_PIECE_EMPTY && chess_ctx->board[0][1].type == CHESS_PIECE_EMPTY) {
                        if (!is_square_being_attacked(chess_ctx, CHESS_POS(0, 3), CHESS_COLOR_BLACK) && !is_square_being_attacked(chess_ctx, CHESS_POS(0, 2), CHESS_COLOR_BLACK)) {
                            available_moves[available_moves_num++] = CHESS_POS(0, 2);
                        }
                    }
                }
            }
        } else {
            if (!chess_ctx->black_state.is_king_under_attack) {
                if (chess_ctx->black_state.short_castling_available) {
                    if (chess_ctx->board[7][5].type == CHESS_PIECE_EMPTY && chess_ctx->board[7][6].type == CHESS_PIECE_EMPTY) {
                        if (!is_square_being_attacked(chess_ctx, CHESS_POS(7, 5), CHESS_COLOR_WHITE) && !is_square_being_attacked(chess_ctx, CHESS_POS(7, 6), CHESS_COLOR_WHITE)) {
                            available_moves[available_moves_num++] = CHESS_POS(7, 6);
                        }
                    }
                }
                if (chess_ctx->black_state.long_castling_available) {
                    if (chess_ctx->board[7][3].type == CHESS_PIECE_EMPTY && chess_ctx->board[7][2].type == CHESS_PIECE_EMPTY && chess_ctx->board[7][1].type == CHESS_PIECE_EMPTY) {
                        if (!is_square_being_attacked(chess_ctx, CHESS_POS(7, 3), CHESS_COLOR_WHITE) && !is_square_being_attacked(chess_ctx, CHESS_POS(7, 2), CHESS_COLOR_WHITE)) {
                            available_moves[available_moves_num++] = CHESS_POS(7, 2);
                        }
                    }
                }
            }
        }
    }

    return available_moves_num;
}

static int pawn_available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed, int discard_non_attacking_moves) {
    const Chess_Piece* current_piece = &chess_ctx->board[position.y][position.x];
    int available_moves_num = 0;
    assert(current_piece->type == CHESS_PIECE_PAWN);
    assert(current_piece->color != CHESS_COLOR_COLORLESS);

    int need_to_check_if_king_is_exposed = !king_can_be_exposed;

    Chess_Board_Position candidate_move;

    if (!discard_non_attacking_moves) {
        candidate_move = current_piece->color == CHESS_COLOR_WHITE ? CHESS_POS(position.y + 1, position.x) : CHESS_POS(position.y - 1, position.x);
        if (chess_position_is_within_bounds(candidate_move)) {
            const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
            if (candidate_piece->type == CHESS_PIECE_EMPTY) {
                available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                    need_to_check_if_king_is_exposed, position, candidate_move);

                // If we can advance 1 tile, then we can also check for two tiles
                candidate_move = current_piece->color == CHESS_COLOR_WHITE ? CHESS_POS(position.y + 2, position.x) : CHESS_POS(position.y - 2, position.x);
                int is_pawn_first_move = current_piece->color == CHESS_COLOR_WHITE ? position.y == 1 : position.y == CHESS_BOARD_HEIGHT - 2;
                if (is_pawn_first_move) {
                    candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
                    if (candidate_piece->type == CHESS_PIECE_EMPTY) {
                        available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                            need_to_check_if_king_is_exposed, position, candidate_move);
                    }
                }
            }
        }
    }

    candidate_move = current_piece->color == CHESS_COLOR_WHITE ? CHESS_POS(position.y + 1, position.x + 1) : CHESS_POS(position.y - 1, position.x + 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type != CHESS_PIECE_EMPTY && candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, candidate_move);
        }
    }
    
    candidate_move = current_piece->color == CHESS_COLOR_WHITE ? CHESS_POS(position.y + 1, position.x - 1) : CHESS_POS(position.y - 1, position.x - 1);
    if (chess_position_is_within_bounds(candidate_move)) {
        const Chess_Piece* candidate_piece = &chess_ctx->board[candidate_move.y][candidate_move.x];
        if (candidate_piece->type != CHESS_PIECE_EMPTY && candidate_piece->color != current_piece->color) {
            available_moves_num = available_move_add_to_list_if_valid(chess_ctx, available_moves, available_moves_num,
                need_to_check_if_king_is_exposed, position, candidate_move);
        }
    }

    // @TODO: en passant

    return available_moves_num;
}

static int available_moves_get(const Chess_Context* chess_ctx, Chess_Board_Position position,
    Chess_Board_Position available_moves[CHESS_BOARD_HEIGHT * CHESS_BOARD_WIDTH], int king_can_be_exposed, int discard_non_attacking_moves) {
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