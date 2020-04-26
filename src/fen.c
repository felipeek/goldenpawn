#include "fen.h"
#include "io.h"
#include <string.h>
#include <memory.h>
#include <stdlib.h>

#define FEN_PARSER_BUFFER_SIZE 64 * 1024

typedef enum {
    FEN_BOARD,
    FEN_TO_MOVE,
    FEN_CASTLING,
    FEN_EN_PASSANT,
    FEN_HALFMOVE,
    FEN_FULLMOVE,
    FEN_END,
} Fen_Format;

static int is_number(char c) {
    if (c >= '0' && c <= '9')
        return 1;
    return 0;
}

static int is_whitespace(char c) {
    return (c == ' ' || c == '\n' || c == '\v' || c == '\t' || c == '\r');
}

static int str_to_s32(char* text, int length) {
    int result = 0;
    int tenths = 1;
    for (int i = length - 1; i >= 0; --i, tenths *= 10)
        result += (text[i] - 0x30) * tenths;
    return result;
}

static int build_fen_input(int argc, const char** argv, char* out) {
    char* aux = out;
    int moves_position = -1;
    int i = 1;
    for (; i < argc; ++i) {
        // We need to stop when 'moves' is found. (UCI)
        if (strcmp(argv[i], "moves") == 0) {
            moves_position = i;
            break;
        }
        int len = strlen(argv[i]);
        memcpy(aux, argv[i], len);
        aux += len;
        *aux = ' ';
        aux++;
    }
    aux--;
    *aux = 0;
    return moves_position;
}

static int fen_to_chess_ctx(Chess_Context* chess_ctx, char* fen_input) {
    memset(chess_ctx, 0, sizeof(Chess_Context));

    Fen_Format parsing_state = FEN_BOARD;

    int rank = 7;
    int file = 0;

    while (*fen_input != 0) {
        char c = *fen_input;
        if (is_whitespace(c)) {
            fen_input++;
            continue;
        }

        switch (parsing_state) 
        {
            case FEN_BOARD: {
                switch (c) 
                {
                    case 'r': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_ROOK, CHESS_COLOR_BLACK}; break;
                    case 'n': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_KNIGHT, CHESS_COLOR_BLACK}; break;
                    case 'b': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_BISHOP, CHESS_COLOR_BLACK}; break;
                    case 'q': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_QUEEN, CHESS_COLOR_BLACK}; break;
                    case 'k': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_KING, CHESS_COLOR_BLACK}; break;
                    case 'p': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_PAWN, CHESS_COLOR_BLACK}; break;

                    case 'R': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_ROOK, CHESS_COLOR_WHITE}; break;
                    case 'N': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_KNIGHT, CHESS_COLOR_WHITE}; break;
                    case 'B': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_BISHOP, CHESS_COLOR_WHITE}; break;
                    case 'Q': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_QUEEN, CHESS_COLOR_WHITE}; break;
                    case 'K': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_KING, CHESS_COLOR_WHITE}; break;
                    case 'P': chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_PAWN, CHESS_COLOR_WHITE}; break;

                    case '/': {
                        rank -= 1;
                        file = -1;
                    } break;

                    default: {
                        if (is_number(c)) {
                            int start = 0;
                            while (start < (c - 0x30) - 1) {
                                chess_ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_EMPTY, CHESS_COLOR_COLORLESS};
                                start++;
                                file++;
                            }
                        }
                    } break;
                }

                if (file >= 7 && rank == 0) {
                    parsing_state = FEN_TO_MOVE;
                } else {
                    file += 1;
                }
            } break;

            case FEN_TO_MOVE: {
                if (c == 'w')
                    chess_ctx->current_turn = CHESS_COLOR_WHITE;
                else if (c == 'b')
                    chess_ctx->current_turn = CHESS_COLOR_BLACK;
                else
                    return -1;
                parsing_state = FEN_CASTLING;
            } break;

            case FEN_CASTLING: {
                while (!is_whitespace(*fen_input)) {
                    char c = *fen_input;
                    if (c == '-')
                        break;
                    
                    switch (c) {
                        case 'K': chess_ctx->white_state.short_castling_available = 1;; break;
                        case 'Q': chess_ctx->white_state.long_castling_available = 1; break;
                        case 'k': chess_ctx->black_state.short_castling_available = 1; break;
                        case 'q': chess_ctx->black_state.long_castling_available = 1; break;
                        default: return -1;
                    }
                    ++fen_input;
                }
                parsing_state = FEN_EN_PASSANT;
            } break;

            case FEN_EN_PASSANT: {
                if (c == '-') {
                    chess_ctx->en_passant_info.available = 0;
                    parsing_state = FEN_HALFMOVE;
                } else {
                    if (c >= 'a' && c <= 'h') {
                        chess_ctx->en_passant_info.available = 1;
                        if (chess_ctx->current_turn == CHESS_COLOR_WHITE) {
                            chess_ctx->en_passant_info.pawn_position.y = 5;
                            chess_ctx->en_passant_info.target.y = 6;
                        } else {
                            chess_ctx->en_passant_info.pawn_position.y = 3;
                            chess_ctx->en_passant_info.target.y = 2;
                        }
                        chess_ctx->en_passant_info.pawn_position.x = c - 0x61;
                        chess_ctx->en_passant_info.target.x = c - 0x61;
                    }
                    fen_input++;	// skip rank
                    parsing_state = FEN_HALFMOVE;
                }
            } break;

            case FEN_HALFMOVE: {
                int length = 0;
                while (!is_whitespace(*(fen_input + length))) length++;
                char half_move_clock = (char)str_to_s32(fen_input, length);
                parsing_state = FEN_FULLMOVE;
            } break;

            case FEN_FULLMOVE: {
                int length = 0;
                while (!is_whitespace(*(fen_input + length)) && *(fen_input + length)) length++;
                char full_move_count = (char)str_to_s32(fen_input, length);
                parsing_state = FEN_END;
            } break;

            case FEN_END: return 0;
        }
        
        ++fen_input;
    }

    return 0;
}

int fen_chess_context_get(Chess_Context* chess_ctx, int argc, const char** argv) {
    char* fen_input = malloc(FEN_PARSER_BUFFER_SIZE);
    int moves_arg_position = build_fen_input(argc, argv, fen_input);

    if (fen_to_chess_ctx(chess_ctx, fen_input)) {
        free(fen_input);
        return -1;
    }

    if (moves_arg_position != -1) {
        for (int i = moves_arg_position + 1; i < argc; ++i) {
            const char* move = argv[i];
            Chess_Move chess_move = {0};
            io_uci_notation_to_move(move, &chess_move);
            chess_move_piece(chess_ctx, chess_ctx, &chess_move);
        }
    }

    free(fen_input);
    return 0;
}