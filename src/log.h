#pragma once
#include <stdio.h>

enum {
    LOG_ERROR = 0,
    LOG_WARN  = 1,
    LOG_INFO  = 2
};

extern int g_log_level;

#define LOGE(fmt, ...) \
do { if (g_log_level >= LOG_ERROR) \
    fprintf(stderr, "[ERROR] " fmt , ##__VA_ARGS__); } while (0)

#define LOGW(fmt, ...) \
do { if (g_log_level >= LOG_WARN) \
    fprintf(stderr, "[WARN ] " fmt , ##__VA_ARGS__); } while (0)

#define LOGI(fmt, ...) \
do { if (g_log_level >= LOG_INFO) \
    fprintf(stderr, "[INFO ] " fmt , ##__VA_ARGS__); } while (0)

#define LOGI_RAW(fmt, ...) \
do { if (g_log_level >= LOG_INFO) \
    fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)
