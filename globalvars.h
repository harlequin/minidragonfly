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

#ifndef __GLOBALVARS_H__
#define __GLOBALVARS_H__



#define MINIDRAGONFLY_VERSION "0.0.1"
#define SERVER_NAME "minidragonfly"
#define EXCEPTIONS_SITE "http://harlequin.github.io/minidragonfly/exceptions.txt"

#include <sqlite3.h>


extern volatile short int quitting;
extern char *db_path;
extern sqlite3 *db;

extern char *option_names[];
extern char *minidragonfly_options[];


extern char *options[];

#define STATUS_WANTED 0
#define STATUS_IGNORED 1
#define STATUS_CATCHED 2

#define SCAN_TIMER 1800 * 1000

typedef enum {
	OPT_CONTROL_PORT = 1,
	OPT_WEB_DIR = 3,
	OPT_CONTROL_USER = 5,
	OPT_CONTROL_PASSWORD = 7,
	OPT_CACHE_DIR = 9,
	OPT_LIBRARY_DIR = 11,
	OPT_LOG_LEVEL = 13,
	OPT_KEEP_ORIGINAL_FILES = 15,
	OPT_MOVE_ASSOCIATED_FILES = 17,
	OPT_RENAME_EPISODES = 19,
	OPT_BLACK_HOLE_ACTIVE = 21,
	OPT_BLACK_HOLE_DIR = 23,
	OPT_NZBGET_ACTIVE= 25,
	OPT_NZBGET_HOST= 27,
	OPT_NZBGET_PORT = 29,
	OPT_NZBGET_USER = 31,
	OPT_NZBGET_PASSWORD = 33,
	OPT_NZBGET_CATEGORY = 35,
	OPT_TVDB_APIKEY = 37
} option_names_e;

char *read_config_file(const char *config, char **options);
void trim_blanks (char *s);
#endif
