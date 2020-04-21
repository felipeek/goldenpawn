#include "io.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

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

void io_init(IO_Context* io_ctx, Chess_Context* chess_ctx) {
    io_ctx->buffer = malloc(sizeof(char) * IO_BUFFER_SIZE);
    io_ctx->argv = malloc(sizeof(char*) * IO_ARGV_SIZE);
    io_ctx->chess_ctx = chess_ctx;
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
        } else if (!strcmp(io_ctx->argv[0], "isready")) {
            command_send("readyok");
        } else if (!strcmp(io_ctx->argv[0], "ucinewgame")) {
        } else if (!strcmp(io_ctx->argv[0], "position")) {
            chess_board_position_set_from_input(io_ctx->chess_ctx, argc - 1, io_ctx->argv + 1);
        } else if (!strcmp(io_ctx->argv[0], "go")) {
            char buffer[256];
            strcpy(buffer, "bestmove ");
            chess_get_random_move(io_ctx->chess_ctx, buffer + strlen(buffer));
            command_send(buffer);
        }
    }
}