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

#ifndef _BLUEDBM_TIME_H
#define _BLUEDBM_TIME_H

/*#include <linux/time.h>*/
#include <linux/ktime.h>

uint32_t time_get_timestamp_in_us (void);
uint32_t time_get_timestamp_in_sec (void);
void time_init (void);

/* stopwatch functions */
struct bdbm_stopwatch {
#ifdef USE_KTIMER
	ktime_t start;
#else
	struct timeval start;
#endif
};

void bdbm_stopwatch_start (struct bdbm_stopwatch* sw);
int64_t bdbm_stopwatch_get_elapsed_time_ms (struct bdbm_stopwatch* sw);
int64_t bdbm_stopwatch_get_elapsed_time_us (struct bdbm_stopwatch* sw);

#endif


