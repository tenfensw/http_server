//
//  wrappers.c
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wrappers.h"

void* hizalloc(const http_size_t size) {
    if (size < 1)
        return NULL; // malloc(0) = NULL;
    
    void* data = malloc(size);
    bzero(data, size);
    
    return data;
}

char* hi_make_current_datetime() {
    // get current C time first
    time_t tmRaw = time(0);
    struct tm* tmNow = localtime(&tmRaw);
    
    // allocate buffer and fill it in
    char* result = calloc(HI_DATETIME_MAX, sizeof(char));
    strftime(result, HI_DATETIME_MAX, "%Y-%m-%d %H:%M:%S", tmNow);
    
    return result;
}

char* hiitoa(const http_ssize_t value) {
    char* result = calloc(10, sizeof(char));
    sprintf(result, "%d", value);
    
    return result;
}

void hiprintf(const char* fn, const http_size_t fc,
              const char* msg, ...) {
#ifndef DEBUG
    HI_UNUSED(fn);
    HI_UNUSED(fc);
    HI_UNUSED(msg);
#else
    va_list vl;
    va_start(vl, msg);
    
    // make printable time and print header
    char* timeStr = hi_make_current_datetime();
    fprintf(stderr, "[%s:%s:%u] ", timeStr, fn, fc);
    
    free(timeStr);
    
    // print msg and vl
    vfprintf(stderr, msg, vl);
    fprintf(stderr, "%c", '\n');
    
    va_end(vl);
#endif
}
