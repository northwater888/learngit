/* utils.h
 *
 * Author: Xiang-Yu Wang <rain.wang@mic.com.tw>
 *
 * Copyright (c) 2005, MiTAC Inc.
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * 2005-11-16 made initial version
 *
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

//extern void tprintf(FILE *, const char *, ...);
//extern void tprintl(const char *, FILE *, const char *, ...);

#define tprintf_out(fmt, arg...) fprintf(stdout, fmt, ## arg)
#define tprintf_err(fmt, arg...) fprintf(stderr, fmt, ## arg)
#define tprintl_out(name, fmt, arg...) tprintl(name, stdout, fmt, ## arg)
#define tprintl_err(name, fmt, arg...) tprintl(name, stderr, fmt, ## arg)

#endif /* UTILS_H */
