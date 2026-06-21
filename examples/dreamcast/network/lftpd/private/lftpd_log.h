#pragma once

//#define DEBUG 1

#define lftpd_log_error(format, ...) lftpd_log_internal("ERROR", format, ##__VA_ARGS__)
#define lftpd_log_info(format, ...) lftpd_log_internal("INFO", format, ##__VA_ARGS__)
#ifdef DEBUG
#define lftpd_log_debug(format, ...) lftpd_log_internal("DEBUG", format, ##__VA_ARGS__)
#else
#define lftpd_log_debug(format, ...)
#endif

void lftpd_log_internal(const char *level, const char *format, ...);

