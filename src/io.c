#include "io.h"
#include "logger.h"
#include "ai.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include <unistd.h>
#include <assert.h>

#define IO_BUFFER_SIZE 4096
#define IO_ARGV_SIZE 256

typedef enum {
    FEN_BOARD,
    FEN_TO_MOVE,
    FEN_CASTLING,
    FEN_EN_PASSANT,
    FEN_HALFMOVE,
    FEN_FULLMOVE,
    FEN_END,
} Fen_Format;

static void eat_spaces(char** at) {
    while (1) {
        char c = **at;
        if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r')
            ++(*at);
        else
            break;
    }
}

static int is_number(char c) {
    if (c >= '0' && c <= '9')
        return 1;
    return 0;
}

static int is_whitespace(char c) {
    return (c == ' ' || c == '\n' || c == '\v' || c == '\t' || c == '\r');
}

int str_to_s32(char* text, int length) {
    int result = 0;
    int tenths = 1;
    for (int i = length - 1; i >= 0; --i, tenths *= 10)
        result += (text[i] - 0x30) * tenths;
    return result;
}

int io_parse_fen(char* fen, Chess_Context* ctx) {
    // startpos
    // rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    
    memset(ctx->board, 0, sizeof(ctx->board));
    ctx->en_passant_info.available = 0;
    memset(&ctx->white_state, 0, sizeof(ctx->white_state));
    memset(&ctx->black_state, 0, sizeof(ctx->black_state));

    Fen_Format parsing_state = FEN_BOARD;

    int rank = 7;
    int file = 0;
    while (*fen != 0) {
        char c = *fen;
        if (is_whitespace(c)) {
            fen++;
            continue;
        }

        switch (parsing_state) 
        {
            case FEN_BOARD: {
                switch (c) 
                {
                    case 'r':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_ROOK, CHESS_COLOR_BLACK}; break;
                    case 'n':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_KNIGHT, CHESS_COLOR_BLACK}; break;
                    case 'b':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_BISHOP, CHESS_COLOR_BLACK}; break;
                    case 'q':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_QUEEN, CHESS_COLOR_BLACK}; break;
                    case 'k':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_KING, CHESS_COLOR_BLACK}; break;
                    case 'p':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_PAWN, CHESS_COLOR_BLACK}; break;

                    case 'R':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_ROOK, CHESS_COLOR_WHITE}; break;
                    case 'N':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_KNIGHT, CHESS_COLOR_WHITE}; break;
                    case 'B':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_BISHOP, CHESS_COLOR_WHITE}; break;
                    case 'Q':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_QUEEN, CHESS_COLOR_WHITE}; break;
                    case 'K':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_KING, CHESS_COLOR_WHITE}; break;
                    case 'P':	ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_PAWN, CHESS_COLOR_WHITE}; break;

                    case '/': {
                        rank -= 1;
                        file = -1;
                    } break;

                    default: {
                        if (is_number(c)) {
                            int start = 0;
                            while (start < (c - 0x30) - 1) {
                                ctx->board[rank][file] = (Chess_Piece){CHESS_PIECE_EMPTY, CHESS_COLOR_COLORLESS};
                                start++;
                                file++;
                            }
                        }
                    }break;
                }

                if (file >= 7 && rank == 0) {
                    parsing_state = FEN_TO_MOVE;
                } else {
                    file += 1;
                }
            } break;

            case FEN_TO_MOVE: {
                if (c == 'w')
                    ctx->current_turn = CHESS_COLOR_WHITE;
                else if (c == 'b')
                    ctx->current_turn = CHESS_COLOR_BLACK;
                else
                    return -1;
                parsing_state = FEN_CASTLING;
            }break;

            case FEN_CASTLING: {
                while (!is_whitespace(*fen)) {
                    char c = *fen;
                    if (c == '-')
                        break;
                    
                    switch (c) {
                        case 'K': ctx->white_state.short_castling_available = 1;; break;
                        case 'Q': ctx->white_state.long_castling_available = 1; break;
                        case 'k': ctx->black_state.short_castling_available = 1; break;
                        case 'q': ctx->black_state.long_castling_available = 1; break;
                        default: return -1;
                    }
                    ++fen;
                }
                parsing_state = FEN_EN_PASSANT;
            }break;

            case FEN_EN_PASSANT: {
                if (c == '-') {
                    ctx->en_passant_info.available = 0;
                    parsing_state = FEN_HALFMOVE;
                }
                else {
                    if (c >= 'a' && c <= 'h') {
                        ctx->en_passant_info.available = 1;
                        if(ctx->current_turn == CHESS_COLOR_WHITE) {
                            ctx->en_passant_info.pawn_position.y = 5;
                            ctx->en_passant_info.target.y = 6;
                        } else {
                            ctx->en_passant_info.pawn_position.y = 3;
                            ctx->en_passant_info.target.y = 2;
                        }
                        ctx->en_passant_info.pawn_position.x = c - 0x61;
                        ctx->en_passant_info.target.x = c - 0x61;
                    }
                    fen++;	// skip rank
                    parsing_state = FEN_HALFMOVE;
                }
            }break;

            case FEN_HALFMOVE: {
                int length = 0;
                while (!is_whitespace(*(fen + length))) length++;
                char half_move_clock = (char)str_to_s32(fen, length);
                parsing_state = FEN_FULLMOVE;
            }break;

            case FEN_FULLMOVE: {
                int length = 0;
                while (!is_whitespace(*(fen + length)) && *(fen + length)) length++;
                char full_move_count = (char)str_to_s32(fen, length);
                parsing_state = FEN_END;
            }break;

            case FEN_END: return 0;
        }
        
        ++fen;
    }

    return 0;
}

static int command_fetch(char* buffer, char** argv) {
    gets(buffer);
    log_debug("received '%s' command", buffer);
    size_t len = strlen(buffer);

    if (len == 0) {
        return 0;
    }

    argv[0] = buffer;
    int argc = 1;
    int last_was_space = 0;
    for (size_t i = 0; i < len; ++i) {
        if (last_was_space) {
            if (buffer[i] != ' ') {
                last_was_space = 0;
                argv[argc++] = buffer + i;
            }
        } else {
            if (buffer[i] == ' ') {
                buffer[i] = '\0';
                last_was_space = 1;
            }
        }
    }

    return argc;
}

static void command_send(char* command) {
    printf("%s\n", command);
    fflush(stdout);
};

void io_init(IO_Context* io_ctx) {
    io_ctx->buffer = malloc(sizeof(char) * IO_BUFFER_SIZE);
    io_ctx->argv = malloc(sizeof(char*) * IO_ARGV_SIZE);
}

void io_start(IO_Context* io_ctx) {
    log_debug("Initializing goldenpawn engine");

    for (;;) {
        int argc = command_fetch(io_ctx->buffer, io_ctx->argv);
        if (!strcmp("end", io_ctx->argv[0])) {
            return;
        }

        if (!strcmp(io_ctx->argv[0], "uci")) {
            command_send("id name Goldenpawn");
            command_send("id author Felipe Kersting");
            command_send("uciok");
        } else if (!strcmp(io_ctx->argv[0], "isready")) {
            command_send("readyok");
        } else if (!strcmp(io_ctx->argv[0], "ucinewgame")) {
        } else if (!strcmp(io_ctx->argv[0], "position")) {
            if(argc > 1 && strcmp(io_ctx->argv[1], "fen") == 0) {
                char temp_buffer[1024] = {0};
                char* aux = temp_buffer;
                int has_move = 0;
                int i = 2;
                for(; i < argc; ++i) {
                    if(strcmp(io_ctx->argv[i], "moves") == 0) {
                        i++;
                        has_move = 1;
                        break;
                    }
                    int len = strlen(io_ctx->argv[i]);
                    memcpy(aux, io_ctx->argv[i], len);
                    aux += len;
                    *aux = ' ';
                    aux++;
                }
                aux--;
                *aux = 0;
                chess_position_from_fen(&io_ctx->chess_ctx, temp_buffer);
                if(has_move) {
                    for(; i < argc; ++i) {
                        char* move = io_ctx->argv[i];
                        Chess_Move chess_move = {0};
                        io_uci_notation_to_move(move, &chess_move);
                        chess_move_piece(&io_ctx->chess_ctx, &io_ctx->chess_ctx, &chess_move);
                    }
                }
            } else {
                chess_context_from_position_input(&io_ctx->chess_ctx, argc - 1, io_ctx->argv + 1);
            }
        } else if (!strcmp(io_ctx->argv[0], "go")) {
            char buffer[256];
            strcpy(buffer, "bestmove ");
            ai_get_best_move(&io_ctx->chess_ctx, buffer + strlen(buffer));
            command_send(buffer);
        }
    }
}

void io_move_to_uci_notation(const Chess_Move* move, char* uci_str) {
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

void io_uci_notation_to_move(const char* uci_str, Chess_Move* move) {
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