#pragma once

#include <cstdio>

const char *pathToFileName(const char *path);

#define SIM_LOG_COLOR_BLACK "30"
#define SIM_LOG_COLOR_RED "31"    // ERROR
#define SIM_LOG_COLOR_GREEN "32"  // INFO
#define SIM_LOG_COLOR_YELLOW "33" // WARNING
#define SIM_LOG_COLOR_BLUE "34"
#define SIM_LOG_COLOR_MAGENTA "35"
#define SIM_LOG_COLOR_CYAN "36" // DEBUG
#define SIM_LOG_COLOR_GRAY "37" // VERBOSE
#define SIM_LOG_COLOR_WHITE "38"

#define SIM_LOG_COLOR(COLOR) "\033[0;" COLOR "m"
#define SIM_LOG_BOLD(COLOR) "\033[1;" COLOR "m"
#define SIM_LOG_RESET_COLOR "\033[0m"

#define SIM_LOG_COLOR_E SIM_LOG_COLOR(SIM_LOG_COLOR_RED)
#define SIM_LOG_COLOR_W SIM_LOG_COLOR(SIM_LOG_COLOR_YELLOW)
#define SIM_LOG_COLOR_I SIM_LOG_COLOR(SIM_LOG_COLOR_GREEN)
#define SIM_LOG_COLOR_D SIM_LOG_COLOR(SIM_LOG_COLOR_CYAN)
#define SIM_LOG_COLOR_V SIM_LOG_COLOR(SIM_LOG_COLOR_GRAY)

#define log_printf(fmt, ...)                                                                       \
    fprintf(stdout, fmt, ##__VA_ARGS__);                                                           \
    fflush(stdout)
#define SIM_LOG_FORMAT(letter, format)                                                             \
    SIM_LOG_COLOR_##letter "[" #letter "][%s:%u] %s(): " format SIM_LOG_RESET_COLOR "\r\n",        \
        pathToFileName(__FILE__), __LINE__, __FUNCTION__

#define log_v(format, ...) log_printf(SIM_LOG_FORMAT(V, format), ##__VA_ARGS__)
#define log_d(format, ...) log_printf(SIM_LOG_FORMAT(D, format), ##__VA_ARGS__)
#define log_i(format, ...) log_printf(SIM_LOG_FORMAT(I, format), ##__VA_ARGS__)
#define log_w(format, ...) log_printf(SIM_LOG_FORMAT(W, format), ##__VA_ARGS__)
#define log_e(format, ...) log_printf(SIM_LOG_FORMAT(E, format), ##__VA_ARGS__)