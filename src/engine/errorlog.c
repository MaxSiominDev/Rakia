#include "engine/errorlog.h"

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <string.h>

#define LOG_FILE_NAME "rakia.log"
#define TAIL_CAPACITY 4096

static char log_path[MAX_PATH];

void errorlog_init(void)
{
    char temp_dir[MAX_PATH];
    const DWORD length = GetTempPathA((DWORD)sizeof temp_dir, temp_dir);

    log_path[0] = '\0';
    if (length == 0 || length >= sizeof temp_dir ||
        snprintf(log_path, sizeof log_path, "%s%s", temp_dir, LOG_FILE_NAME) >= (int)sizeof log_path) {
        log_path[0] = '\0';
        return;
    }

    freopen(log_path, "w", stderr);
}

static int read_log_tail(char *out, size_t size)
{
    FILE *log;
    long file_size;
    long start;
    size_t bytes_read;

    if (log_path[0] == '\0' || (log = fopen(log_path, "rb")) == NULL) {
        return -1;
    }

    fseek(log, 0, SEEK_END);
    file_size = ftell(log);
    start = file_size > (long)size - 1 ? file_size - ((long)size - 1) : 0;
    fseek(log, start, SEEK_SET);

    bytes_read = fread(out, 1, size - 1, log);
    out[bytes_read] = '\0';
    fclose(log);
    return 0;
}

void errorlog_report_failure(void)
{
    char tail[TAIL_CAPACITY];
    char message[TAIL_CAPACITY + MAX_PATH + 128];

    fflush(stderr);

    if (read_log_tail(tail, sizeof tail) == 0 && tail[0] != '\0') {
        snprintf(message, sizeof message, "Rakia failed to start.\n\n%s\nFull log: %s", tail, log_path);
    } else if (log_path[0] != '\0') {
        snprintf(message, sizeof message, "Rakia failed to start.\nFull log: %s", log_path);
    } else {
        strcpy(message, "Rakia failed to start. No log file is available.");
    }

    MessageBoxA(NULL, message, "Rakia", MB_OK | MB_ICONERROR);
}

#else

void errorlog_init(void)
{
}

void errorlog_report_failure(void)
{
}

#endif
