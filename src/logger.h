#ifndef GOLDENPAWN_LOGGER_H
#define GOLDENPAWN_LOGGER_H

enum Log_Level {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO = 1
};

// core functions
void log_level_set(enum Log_Level level);
void log_debug(const char* format, ...);
void log_info(const char* format, ...);

#endif