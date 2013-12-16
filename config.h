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

#ifndef CONFIG_H_
#define CONFIG_H_

#define APPLICATION_NAME "minidragonfly"
#define DB_VERSION 5

/*Special Visual Studio fixes */
#ifdef _MSC_VER
#define _WIN32_WINNT 0x0600
#define strcasecmp(x,y) _stricmp(x,y)


#define mkdir(dir, flags) _mkdir(dir)

//Visual Studio Fixes
#ifndef _MODE_T_
#define	_MODE_T_
typedef unsigned short _mode_t;

#ifndef	_NO_OLDNAMES
typedef _mode_t	mode_t;
#endif
#endif	/* Not _MODE_T_ */

#define	__S_ISTYPE(mode, mask)	(((mode) & _S_IFMT) == (mask))
#define	S_ISDIR(mode)	 __S_ISTYPE((mode), _S_IFDIR)
#endif

/*Goes for mingw and cygwin */
#ifdef WIN32

#undef HAVE_FORK

#ifndef S_IRWXU
#define S_IRWXU 0
#endif

#define S_IRWXG 0
#define S_IROTH 0
#define S_IXOTH 0

//#define WIN32_LEAN_AND_MEAN 1
//#define _WIN32_WINNT 0x0600
#include <windows.h>
#define inline __inline
#define strncasecmp(a, b, c) _strnicmp(a, b, c)
#define usleep(usec) Sleep((usec) / 1000)
#define size_t SIZE_T

#define snprintf _snprintf
#define F_OK    0
#define mkdir(x,y) mkdir(x)

#else

#define MAX_PATH 255


#endif

#endif /* CONFIG_H_ */
