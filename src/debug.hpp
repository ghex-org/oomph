/* 
 * ghex-org
 * 
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 * 
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */
#pragma once

#include <string.h>
#include <time.h>

#if (OOMPH_DEBUG_LEVEL >= 2)
#define OOMPH_LOG(msg, ...)                                                                        \
    do                                                                                             \
    {                                                                                              \
        time_t tm = time(NULL);                                                                    \
        char*  stm = ctime(&tm);                                                                   \
        stm[strlen(stm) - 1] = 0;                                                                  \
        (void)fprintf(stderr, "%s %s:%d  " msg "\n", stm, __FILE__, __LINE__, ##__VA_ARGS__);      \
        (void)fflush(stderr);                                                                      \
    } while (0);
#else
#define OOMPH_LOG(msg, ...)                                                                        \
    do                                                                                             \
    {                                                                                              \
    } while (0);
#endif

#define OOMPH_WARN(msg, ...)                                                                       \
    do                                                                                             \
    {                                                                                              \
        time_t tm = time(NULL);                                                                    \
        char*  stm = ctime(&tm);                                                                   \
        stm[strlen(stm) - 1] = 0;                                                                  \
        (void)fprintf(                                                                             \
            stderr, "%s WARNING: %s:%d  " msg "\n", stm, __FILE__, __LINE__, ##__VA_ARGS__);       \
    } while (0);
