//
//  wrappers.h
//  http_server
//
//  Created by Tim K. on 26.02.23.
//  Copyright © 2023 Tim K. All rights reserved.
//

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "http_server.h"

/// zero malloc
void* hizalloc(const http_size_t size);

#define hizalloc_struct(stt) hizalloc(sizeof(struct stt))

/// Ruby-like if-null condition
#define HI_IF_NULL(stv, sto) (stv ? stv : sto)

/// debug printf string (-> stderr) - do not use directly!
void hiprintf(const char* fn, const http_size_t fc,
              const char* msg, ...);

#define HI_DATETIME_MAX 30

/// make current date-time string
char* hi_make_current_datetime(void);

/// short filename macro
#ifdef __FILE_NAME__
#define __HI_COMPILER_FILE_NAME__ __FILE_NAME__
#else
#define __HI_COMPILER_FILE_NAME__ __FILE__
#endif

#define HI_DEBUG(...) hiprintf(__HI_COMPILER_FILE_NAME__, __LINE__, \
                               __VA_ARGS__)
#define HI_ERRNO_DEBUG(msg) hiprintf(__HI_COMPILER_FILE_NAME__, __LINE__, \
                                    "%s: %s", msg, \
                                    strerror(errno))

#define HI_UNUSED(vl) (void)(vl)
