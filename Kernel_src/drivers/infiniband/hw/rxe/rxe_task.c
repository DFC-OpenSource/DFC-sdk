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

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/hardirq.h>

#include "rxe_task.h"

#if AFFINITY_FOR_TASKLETS
#include<linux/semaphore.h>
#include<linux/kthread.h>
#include<linux/delay.h>
#define KILL 0x5
#define START 0x0

static int sem_wait_thread(void *data)
{

	struct rxe_task *task=((struct rxe_task*)(data));
	while(1)
	{
		down(&task->semvar);
		while(unlikely(task->thread_stop==KILL))
		{
			msleep(50);
			if(likely(kthread_should_stop()))
				goto out;
		}

		tasklet_schedule(&task->tasklet);
		//printk("roce: Came out of waiting state\n");
	}
out:
	return 0;
}
#endif

int __rxe_do_task(struct rxe_task *task)

{
	int ret;

	while ((ret = task->func(task->arg)) == 0)
		;

	task->ret = ret;

	return ret;
}

/*
 * this locking is due to a potential race where
 * a second caller finds the task already running
 * but looks just after the last call to func
 */
void rxe_do_task(unsigned long data)
{
	int cont;
	int ret;
	unsigned long flags;
	struct rxe_task *task = (struct rxe_task *)data;

	spin_lock_irqsave(&task->state_lock, flags);
	switch (task->state) {
	case TASK_STATE_START:
		task->state = TASK_STATE_BUSY;
		spin_unlock_irqrestore(&task->state_lock, flags);
		break;

	case TASK_STATE_BUSY:
		task->state = TASK_STATE_ARMED;
		/* fall through to */
	case TASK_STATE_ARMED:
		spin_unlock_irqrestore(&task->state_lock, flags);
		return;

	default:
		spin_unlock_irqrestore(&task->state_lock, flags);
		pr_warn("bad state = %d in rxe_do_task\n", task->state);
		return;
	}

	do {
		cont = 0;
		ret = task->func(task->arg);

		spin_lock_irqsave(&task->state_lock, flags);
		switch (task->state) {
		case TASK_STATE_BUSY:
			if (ret)
				task->state = TASK_STATE_START;
			else
				cont = 1;
			break;

		/* soneone tried to run the task since the
		   last time we called func, so we will call
		   one more time regardless of the return value */
		case TASK_STATE_ARMED:
			task->state = TASK_STATE_BUSY;
			cont = 1;
			break;

		default:
			pr_warn("bad state = %d in rxe_do_task\n",
				task->state);
		}
		spin_unlock_irqrestore(&task->state_lock, flags);
	} while (cont);

	task->ret = ret;
}

int rxe_init_task(void *obj, struct rxe_task *task,
		  void *arg, int (*func)(void *), char *name)
{
	task->obj	= obj;
	task->arg	= arg;
	task->func	= func;
	snprintf(task->name, sizeof(task->name), "%s", name);

	tasklet_init(&task->tasklet, rxe_do_task, (unsigned long)task);


#if AFFINITY_FOR_TASKLETS

        sema_init(&task->semvar,0);
        task->sem_thread =  kthread_create(sem_wait_thread,(void *)task,name);

        if(!strcmp(name,"comp"))
        {
                kthread_bind(task->sem_thread,4);
        }
        if(!strcmp(name,"req"))
        {
                kthread_bind(task->sem_thread,5);
        }
        if(!strcmp(name,"resp"))
        {
                kthread_bind(task->sem_thread,6);
        }
        if(!strcmp(name,"arb"))
        {
                kthread_bind(task->sem_thread,7);
        }

#endif

	task->state = TASK_STATE_START;
	spin_lock_init(&task->state_lock);

#if AFFINITY_FOR_TASKLETS
        wake_up_process(task->sem_thread);
#endif

	return 0;
}

void rxe_cleanup_task(struct rxe_task *task)
{
#if AFFINITY_FOR_TASKLETS
	task->thread_stop=KILL;
	up(&task->semvar);
	if(!kthread_stop(task->sem_thread)) {
		printk("%s kthread destroyed \n",__func__);
	}
#endif
	tasklet_kill(&task->tasklet);
}

void rxe_run_task(struct rxe_task *task, int sched)
{
#if AFFINITY_FOR_TASKLETS
	up(&task->semvar); // FIXME: Need to see whether always scheduling a tasklet is necessary
#else
	if (sched)
		tasklet_schedule(&task->tasklet);
	else
		rxe_do_task((unsigned long)task);
#endif
}

void rxe_disable_task(struct rxe_task *task)
{
	tasklet_disable(&task->tasklet);
}

void rxe_enable_task(struct rxe_task *task)
{
	tasklet_enable(&task->tasklet);
}
