/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2013 element
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef __ERR_H__
#define __ERR_H__

#define E_OFF		0
#define E_FATAL     1
#define E_ERROR		2
#define E_WARN      3
#define E_INFO      4
#define E_DEBUG		5

enum _log_facility
{
  L_GENERAL=0,
  L_DB_SQL,
  L_SCANNER,
  L_WEBSERVER,
  L_PROCESSOR,
  L_MAX
};

extern int log_level[L_MAX];
extern int log_init(const char *fname, const char *debug);
extern void log_err(int level, enum _log_facility facility, char *fname, int lineno, char *fmt, ...);

#if ! defined(_MSC_VER)
	#define DPRINTF(level, facility, fmt, arg...) { log_err(level, facility, __FILE__, __LINE__, fmt, ##arg); }
#else
	#define DPRINTF(level, facility, fmt, ...) { log_err(level, facility, __FILE__, __LINE__, fmt, __VA_ARGS__); }
#endif

#endif /* __ERR_H__ */
