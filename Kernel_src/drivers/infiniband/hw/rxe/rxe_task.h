/*
 * Copyright (c) 2009-2011 Mellanox Technologies Ltd. All rights reserved.
 * Copyright (c) 2009-2011 System Fabric Works, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *	   Redistribution and use in source and binary forms, with or
 *	   without modification, are permitted provided that the following
 *	   conditions are met:
 *
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef RXE_TASK_H
#define RXE_TASK_H

#define AFFINITY_FOR_TASKLETS 0
#if AFFINITY_FOR_TASKLETS
#include<linux/semaphore.h>
#endif

enum {
	TASK_STATE_START	= 0,
	TASK_STATE_BUSY		= 1,
	TASK_STATE_ARMED	= 2,
};

/*
 * data structure to describe a 'task' which is a short
 * function that returns 0 as long as it needs to be
 * called again.
 */
struct rxe_task {
	void			*obj;
	struct tasklet_struct	tasklet;
	int			state;
	spinlock_t		state_lock; /* spinlock for task state */
	void			*arg;
	int			(*func)(void *arg);
	int			ret;
	char			name[16];
	#if AFFINITY_FOR_TASKLETS
	struct semaphore semvar;
	struct task_struct *sem_thread;
	int thread_stop;
	#endif
};

/*
 * init rxe_task structure
 *	arg  => parameter to pass to fcn
 *	fcn  => function to call until it returns != 0
 */
int rxe_init_task(void *obj, struct rxe_task *task,
		  void *arg, int (*func)(void *), char *name);

/*
 * cleanup task
 */
void rxe_cleanup_task(struct rxe_task *task);

/*
 * raw call to func in loop without any checking
 * can call when tasklets are disabled
 */
int __rxe_do_task(struct rxe_task *task);

/*
 * common function called by any of the main tasklets
 * If there is any chance that there is additional
 * work to do someone must reschedule the task before
 * leaving
 */
void rxe_do_task(unsigned long data);

/*
 * run a task, else schedule it to run as a tasklet
 */
void rxe_run_task(struct rxe_task *task, int sched);

/*
 * keep a task from scheduling
 */
void rxe_disable_task(struct rxe_task *task);

/*
 * allow task to run
 */
void rxe_enable_task(struct rxe_task *task);

#endif /* RXE_TASK_H */
