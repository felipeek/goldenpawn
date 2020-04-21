#include "io.h"
#include "logger.h"
#include "chess.h"
#include <stdio.h>

int main() {
    Chess_Context chess_ctx;
    IO_Context io_ctx;
    log_level_set(LOG_LEVEL_DEBUG);
    chess_init(&chess_ctx);
    io_init(&io_ctx, &chess_ctx);
    io_start(&io_ctx);
    return 0;
}