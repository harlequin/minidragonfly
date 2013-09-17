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


#ifndef UTILS_H_
#define UTILS_H_

#include <sys/stat.h>

#include "config.h"
#include "types.h"



void *s_malloc(size_t size);


void 	strncpyt(char *dst, const char *src, size_t len);
void 	download_fanart_clearart(int id, const char *type);
int 	make_dir(char * path, mode_t mode);
int		create_database(sqlite3 *db);
char	*str_replace(const char *strbuf, const char *strold, const char *strnew);
int 	download(const char *url, struct memory_struct *chunk);
int 	is_video(const char * file);

/**
 * nzbget interface
 */
void nzbget_appendurl ( const char *filename, const char *category, int priority, int add_to_top, const char *url );

void load_exceptions();

#endif /* UTILS_H_ */
