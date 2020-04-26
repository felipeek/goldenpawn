#include "io.h"
#include "logger.h"
#include "ai.h"
#include "fen.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define IO_BUFFER_SIZE 4096
#define IO_ARGV_SIZE 256


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
        if (!strcmp("quit", io_ctx->argv[0])) {
            log_debug("Exiting goldenpawn engine");
            return;
        } else if (!strcmp(io_ctx->argv[0], "uci")) {
            command_send("id name Goldenpawn");
            command_send("id author Felipe Kersting");
            command_send("uciok");
        } else if (!strcmp(io_ctx->argv[0], "isready")) {
            command_send("readyok");
        } else if (!strcmp(io_ctx->argv[0], "ucinewgame")) {
        } else if (!strcmp(io_ctx->argv[0], "position")) {
            if (!strcmp(io_ctx->argv[1], "fen")) {
                fen_chess_context_get(&io_ctx->chess_ctx, argc - 2, io_ctx->argv + 2);
            } else if (!strcmp(io_ctx->argv[1], "startpos")) {
                chess_context_from_position_input(&io_ctx->chess_ctx, argc - 2, io_ctx->argv + 2);
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