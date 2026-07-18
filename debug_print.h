#pragma once

#include "memory/memory.h"
#include "syscalls/syscalls.h"

static char log_buf[1024];

static inline int debug_print(const char *fmt, ...){
    __attribute__((aligned(16))) va_list args;
    va_start(args, fmt); 
    memset(log_buf, 0, 1024);
    size_t n = string_format_va_buf(fmt, log_buf, sizeof(log_buf), args);
    va_end(args);
    if (n >= sizeof(log_buf)) log_buf[sizeof(log_buf)-1] = '\0';
    printl(log_buf);
    return 0;
}