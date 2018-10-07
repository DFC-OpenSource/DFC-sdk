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

#ifndef _BLUEDBM_PRIOR_QUEUE_MQ_H
#define _BLUEDBM_PRIOR_QUEUE_MQ_H

#include "../3rd/uthash.h"

enum BDBM_PRIOR_QUEUE_SIZE {
	INFINITE_PRIOR_QUEUE = -1,
};

struct bdbm_prior_queue_item_t {
	struct list_head list; /* list header */
	void* ptr_req;
	uint64_t lpa;
	uint64_t tag;
	uint8_t lock;
};

struct bdbm_prior_lpa_item_t {
	struct list_head list;	/* list header */
	uint64_t lpa;
	uint64_t cur_tag;
	uint64_t max_tag;
	UT_hash_handle hh;	/* hash header */
};

struct bdbm_prior_queue_t {
	uint64_t nr_queues;
	int64_t max_size;
	int64_t qic; /* queue item count */
	bdbm_spinlock_t lock; /* queue lock */
	struct list_head* qlh; /* queue list header */
 	struct bdbm_prior_lpa_item_t* hash_lpa;	/* lpa hash */
};

struct bdbm_prior_queue_t* bdbm_prior_queue_create (uint64_t nr_queues, int64_t size);
void bdbm_prior_queue_destroy (struct bdbm_prior_queue_t* mq);
uint8_t bdbm_prior_queue_enqueue (struct bdbm_prior_queue_t* mq, uint64_t qid, uint64_t lpa, void* req);
void* bdbm_prior_queue_dequeue (struct bdbm_prior_queue_t* mq, uint64_t qid, struct bdbm_prior_queue_item_t** out_q);
uint8_t bdbm_prior_queue_remove (struct bdbm_prior_queue_t* mq, struct bdbm_prior_queue_item_t* q);
uint8_t bdbm_prior_queue_move (struct bdbm_prior_queue_t* mq, uint64_t quid, struct bdbm_prior_queue_item_t* q);
uint8_t bdbm_prior_queue_is_full (struct bdbm_prior_queue_t* mq);
uint8_t bdbm_prior_queue_is_empty (struct bdbm_prior_queue_t* mq, uint64_t qid);
uint8_t bdbm_prior_queue_is_all_empty (struct bdbm_prior_queue_t* mq);
uint64_t bdbm_prior_queue_get_nr_items (struct bdbm_prior_queue_t* mq);

#endif
