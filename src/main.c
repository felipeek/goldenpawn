#include "io.h"
#include "logger.h"
#include "chess.h"
#include <stdio.h>

int main() {
    IO_Context io_ctx;
    log_level_set(LOG_LEVEL_DEBUG);
    io_init(&io_ctx);
    io_start(&io_ctx);
    return 0;
}