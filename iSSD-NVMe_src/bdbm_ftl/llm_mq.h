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

#ifndef _BLUEDBM_LLM_SCHED_H
#define _BLUEDBM_LLM_SCHED_H

extern struct bdbm_llm_inf_t _llm_mq_inf;

uint32_t llm_mq_create (struct bdbm_drv_info* bdi);
void llm_mq_destroy (struct bdbm_drv_info* bdi);
uint32_t llm_mq_make_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);
void llm_mq_flush (struct bdbm_drv_info* bdi);
void llm_mq_end_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* req);

#endif
