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


#ifndef __TYPES_H_
#define __TYPES_H_

#include <stdlib.h>
#include <stddef.h>

#include "queue.h"
#include "config.h"


typedef struct newznab_site_struct {
	unsigned char *valid;
	unsigned char *active;
	char *url;
	char *apikey;
} newznab_site_t;

struct memory_struct
{
	char *memory;
	size_t size;
};

typedef struct tv_episode_struct
{
	int tv_series_id;
	int season;
	int episode;
	char *title;
	int status;
	char *language;

	char *tv_show;
	int quality;
	char *quality_raw;

	struct tv_episode_struct *next;

} tv_epidose_t;

struct NameValue {
    LIST_ENTRY(NameValue) entries;
    char name[64];
    char value[];
};

struct NameValueParserData {
    LIST_HEAD(listhead, NameValue) head;
    char curelt[64];
};



typedef enum {
	EPISODE_STATUS_SKIPPED 		= 0,
	EPISODE_STATUS_WANTED 		= 1,
	EPISODE_STATUS_ARCHIVED 	= 2,
	EPISODE_STATUS_IGNORED 		= 3,
	EPISODE_STATUS_SNATCHED 	= 4,

	EPISODE_STATUS_MAX
} episode_status_e;


#endif /* TYPES_H_ */
