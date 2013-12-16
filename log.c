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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include "queue.h"
#include "log.h"





static FILE *log_fp = NULL;
static int default_log_level = E_WARN;
int log_level[L_MAX];

char *facility_name[] = {
	"general",
	"artwork",
	"database",
	"inotify",
	"scanner",
	"metadata",
	"http",
	"ssdp",
	"tivo",
	0
};

char *level_name[] = {
	"off",					/* E_OFF */
	"fatal",				/* E_FATAL */
	"error",				/* E_ERROR */
	"warning",				/* E_WARN -- warn */
	"info",					/* E_INFO */
	"debug",				/* E_DEBUG */
	0
};


int vasprintf( char **sptr, char *fmt, va_list argv ) {
    int wanted = vsnprintf( *sptr = NULL, 0, fmt, argv );
    if( (wanted > 0) && ((*sptr = malloc( 1 + wanted )) != NULL) )
        return vsprintf( *sptr, fmt, argv );

    return wanted;
}

int asprintf( char **sptr, char *fmt, ... ) {
    int retval;
    va_list argv;
    va_start( argv, fmt );
    retval = vasprintf( sptr, fmt, argv );
    va_end( argv );
    return retval;
}


int log_init(const char *fname, const char *debug) {
	int i;
	FILE *fp;
	short int log_level_set[L_MAX];


	if (debug)	{
		const char *rhs, *lhs, *nlhs, *p;
		int n;
		int level, facility;
		memset(&log_level_set, 0, sizeof(log_level_set));
		rhs = nlhs = debug;
		while (rhs && (rhs = strchr(rhs, '='))) {
			rhs++;
			p = strchr(rhs, ',');
			n = p ? p - rhs : strlen(rhs);
			for (level=0; level_name[level]; level++) {
				if (!(strncasecmp(level_name[level], rhs, n)))
					break;
			}
			lhs = nlhs;
			rhs = nlhs = p;
			if (!(level_name[level])) {
				/*// unknown level*/
				continue;
			}
			do {
				if (*lhs==',') lhs++;
				p = strpbrk(lhs, ",=");
				n = p ? p - lhs : strlen(lhs);
				for (facility=0; facility_name[facility]; facility++) {
					if (!(strncasecmp(facility_name[facility], lhs, n)))
						break;
				}
				if ((facility_name[facility])) {
					log_level[facility] = level;
					log_level_set[facility] = 1;
				}
				lhs = p;
			} while (*lhs && *lhs==',');
		}
		for (i=0; i<L_MAX; i++)
		{
			if( !log_level_set[i] )
			{
				log_level[i] = default_log_level;
			}
		}
	}
	else {
		for (i=0; i<L_MAX; i++)
			log_level[i] = default_log_level;
	}

	if (!fname)					/* use default i.e. stdout */
		return 0;

	if (!(fp = fopen(fname, "a")))
		return 1;
	log_fp = fp;
	return 0;
}


unsigned int log_items_size = 0;


void log_err(int level, enum _log_facility facility, char *fname, int lineno, char *fmt, ...) {
	
	char * errbuf;
	va_list ap;

	time_t t;
	struct tm *tm;

	if (level && level>log_level[facility] && level>E_FATAL)
		return;

	//if (!log_fp)
	//	log_fp = stdout;
	/*
	// user log
	 *
	 */
	va_start(ap, fmt);
	
	/*
	//vsnprintf(errbuf, sizeof(errbuf), fmt, ap);	
	*/
	if (vasprintf(&errbuf, fmt, ap) == -1)
	{
		va_end(ap);
		return;
	}
	
	va_end(ap);

	t = time(NULL);
		tm = localtime(&t);


/*
	// timestamp




//	if (level)
//		fprintf(log_fp, "%s:%d: %s: %s", fname, lineno, level_name[level], errbuf);
//	else
//		fprintf(log_fp, "%s:%d: %s", fname, lineno, errbuf);
*/

	fprintf(stdout, "[%04d/%02d/%02d %02d:%02d:%02d] ",
		        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		        tm->tm_hour, tm->tm_min, tm->tm_sec);

	if (level)
		fprintf(stdout /*log_fp*/, "[%s] %s:%d: %s", level_name[level], fname, lineno, errbuf);
	else
		fprintf(stdout /*log_fp*/, "%s:%d: %s", fname, lineno, errbuf);


	fflush(stdout/*log_fp*/);


	if(log_fp) {
		if (level)
			fprintf(log_fp, "[%s] %s:%d: %s", level_name[level], fname, lineno, errbuf);
		else
			fprintf(log_fp, "%s:%d: %s", fname, lineno, errbuf);

		fflush(log_fp);
	}


	free(errbuf);

	if (level==E_FATAL)
		exit(-1);

	return;
}
