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

#ifndef _BLUEDBM_HOST_BLOCK_H
#define _BLUEDBM_HOST_BLOCK_H

#include <stdint.h>

#include "../nvme_bio.h"
#include "bdbm_drv.h"

extern struct bdbm_host_inf_t _host_block_inf;

struct bdbm_host_block_private {
	volatile unsigned long nr_host_reqs;
	pthread_spinlock_t lock;
	//bdbm_spinlock_t lock;
};


#if 0
unsigned int host_block_open (struct bdbm_drv_info* bdi);
void host_block_close (struct bdbm_drv_info* bdi);
#endif
int host_block_make_req (struct bdbm_drv_info* bdi, struct nvme_bio *bio);
void host_block_end_req (struct bdbm_drv_info* bdi, struct bdbm_hlm_req_t* req);

#endif

