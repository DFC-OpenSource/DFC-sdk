/*
The MIT License (MIT)

Copyright (c) 2014-2015 CSAIL, MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _BLUEDBM_DEBUG_H
#define _BLUEDBM_DEBUG_H

#include "platform.h"
#define CONFIG_ENABLE_MSG
#define CONFIG_ENABLE_DEBUG

#ifdef CONFIG_ENABLE_MSG
	#define bdbm_msg(fmt, ...)  \
		do {    \
			printf(  "bdbm: " fmt "\n", ##__VA_ARGS__);  \
		} while (0);
#else
	#define bdbm_msg(fmt, ...)
#endif
#define bdbm_warning(fmt, ...)  \
	do {    \
		printf(  "bdbm-warning: " fmt " (%d@%s)\n", ##__VA_ARGS__, __LINE__, __FILE__);    \
	} while (0);
#define bdbm_error(fmt, ...)  \
	do {    \
		printf(  "bdbm-error: " fmt " (%d@%s)\n", ##__VA_ARGS__, __LINE__, __FILE__);    \
	} while (0);
#define bdbm_warn_on(condition) \
	do { 	\
		if (condition)	\
			bdbm_warning ("bdbm_warn_on"); \
		WARN_ON(condition); \
	} while (0);


#ifdef CONFIG_ENABLE_DEBUG
	#define bdbm_bug_on(condition)  \
		do { 	\
			if (condition) {	\
				bdbm_error ("bdbm_bug_on"); \
				while(1) sleep(1); \
			} \
		} while (0);
	#define bdbm_dbg_msg(fmt, ...)  \
		do {    \
			printf(  "bdbm: " fmt " (%d@%s)\n", ##__VA_ARGS__, __LINE__, __FILE__);    \
		} while (0);
#else
	#define bdbm_bug_on(condition)
	#define bdbm_dbg_msg(fmt, ...)
#endif

#endif /* _BLUEDBM_DEBUG_H */
