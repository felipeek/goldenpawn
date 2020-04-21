#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static enum Log_Level log_level = LOG_LEVEL_INFO;

static void
log_out(FILE* target, const char* level, const char* format, va_list argptr)
{
  size_t needed = snprintf(NULL, 0, "info %s %s\n", level, format) + 1;
  char *buf = malloc(needed);
  sprintf(buf, "info %s %s\n", level, format);
  vfprintf(target, buf, argptr);
  free(buf);
}

void
log_level_set(enum Log_Level level)
{
  log_level = level;
}

void
log_debug(const char* format, ...)
{
  if (log_level > LOG_LEVEL_DEBUG)
    return;
  va_list argptr;
  va_start(argptr, format);
  log_out(stdout, "string", format, argptr);
  va_end(argptr);
}