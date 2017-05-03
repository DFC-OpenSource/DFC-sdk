/* Copyright (c) 2016 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include "dce.h"

struct dma_item {
	void *vaddr;
	struct dpaa2_fd fd;
	dma_addr_t paddr;
	size_t size;
};

struct dce_ioctl_process {
	enum dce_engine dce_mode;
	dma_addr_t input;
	dma_addr_t output;
	size_t input_len;
	size_t output_len;
};

struct work_unit {
	uint8_t status;
	size_t output_produced;
	volatile int done;
	wait_queue_head_t reply_wait;
};

struct dce_session comp_session;
struct dce_session decomp_session;

static void dce_callback(struct dce_session *session,
			uint8_t status,
			dma_addr_t input,
			dma_addr_t output,
			size_t input_consumed,
			size_t output_produced,
			void *context)
{
	struct work_unit *work_unit = context;

	work_unit->done = true;
	work_unit->status = status;
	work_unit->output_produced = output_produced;
	wake_up(&work_unit->reply_wait);
}

static int fsl_dce_dev_open(struct inode *inode, struct file *filep)
{
	struct dce_session_params params = {
		.engine = DCE_COMPRESSION,
		.paradigm = DCE_SESSION_STATELESS,
		.compression_format = DCE_SESSION_CF_ZLIB,
		.compression_effort = DCE_SESSION_CE_BEST_POSSIBLE,
		/* gz_header not used in ZLIB format mode */
		/* buffer_pool_id not used */
		/* buffer_pool_id2 not used */
		/* release_buffers not used */
		/* encode_base_64 not used */
		/* callback_frame not used, will use callback_data instead */
		.callback_data = dce_callback
	};
	int ret;

	ret = dce_session_create(&comp_session, &params);
	if (ret)
		return -EACCES;

	params.engine = DCE_DECOMPRESSION;
	ret = dce_session_create(&decomp_session, &params);
	if (ret) {
		dce_session_destroy(&comp_session);
		return -EACCES;
	}
	return 0;
}

static int fsl_dce_dev_release(struct inode *inode, struct file *filep)
{
	dce_session_destroy(&comp_session);
	dce_session_destroy(&decomp_session);
	return 0;
}

static long fsl_dce_dev_ioctl(struct file *filep,
			unsigned int cmd,
			unsigned long arg)
{
	struct dce_ioctl_process param;
	struct work_unit work_unit;
	struct dma_item input;
	struct dma_item output;
	struct fsl_mc_device *device;
	struct dce_session *session;
	int ret = -ENOMEM, busy_count = 0;
	unsigned long timeout;

	ret = copy_from_user(&param, (void __user *)arg, sizeof(param));
	if (ret) {
		pr_err("Copy from user failed in DCE driver\n");
		return -EIO;
	}

	input.vaddr = kmalloc(param.input_len, GFP_DMA);
	if (!input.vaddr)
		goto err_alloc_in_data;
	input.size = param.input_len;
	ret = copy_from_user(input.vaddr, (void __user *)param.input,
					input.size);
	if (ret) {
		ret = -EIO;
		pr_err("Copy from user failed in DCE driver\n");
		goto err_alloc_out_data;
	}

	output.vaddr = kzalloc(param.output_len, GFP_DMA);
	if (!output.vaddr) {
		ret = -ENOMEM;
		goto err_alloc_out_data;
	}
	output.size = param.output_len;

	session = param.dce_mode == DCE_COMPRESSION ?
			&comp_session : &decomp_session;
	device = dce_session_device(session);
	input.paddr = dma_map_single(&device->dev,
		input.vaddr, input.size,
		DMA_BIDIRECTIONAL);
	output.paddr = dma_map_single(&device->dev,
		output.vaddr, output.size,
		DMA_BIDIRECTIONAL);

try_again:
	work_unit.done = false;
	init_waitqueue_head(&work_unit.reply_wait);
	ret = dce_process_data(session,
		     input.paddr,
		     output.paddr,
		     input.size,
		     output.size,
		     DCE_Z_FINISH,
		     1, /* Initial */
		     0, /* Recycle */
		     &work_unit);
	if (ret == -EBUSY && busy_count++ < 10)
		goto try_again;
	timeout = wait_event_timeout(work_unit.reply_wait, work_unit.done,
				msecs_to_jiffies(3500));
	if (!timeout) {
		pr_err("Error, didn't get expected callback\n");
		goto err_timedout;
	}
	dma_unmap_single(&device->dev, input.paddr, input.size,
				DMA_BIDIRECTIONAL);
	dma_unmap_single(&device->dev, output.paddr, output.size,
				DMA_BIDIRECTIONAL);
	if (work_unit.status != STREAM_END)
		pr_err("Unexpected DCE status 0x%x\n", work_unit.status);
	ret = copy_to_user((void __user *)param.output, output.vaddr,
			work_unit.output_produced);
	if (ret) {
		ret = -EIO;
		pr_err("Copy to user failed in DCE driver\n");
	}
	param.output_len = work_unit.output_produced;
	ret = copy_to_user((void __user *)arg, &param, sizeof(param));
	if (ret) {
		ret = -EIO;
		pr_err("Copy to user failed in DCE driver\n");
	}
err_timedout:
	kfree(output.vaddr);
err_alloc_out_data:
	kfree(input.vaddr);
err_alloc_in_data:
	return ret;
}

static const struct file_operations fsl_dce_dev_fops = {
	.owner = THIS_MODULE,
	.open = fsl_dce_dev_open,
	.release = fsl_dce_dev_release,
	.unlocked_ioctl = fsl_dce_dev_ioctl,
	.compat_ioctl = fsl_dce_dev_ioctl,
};

static struct miscdevice fsl_dce_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "dce",
	.fops = &fsl_dce_dev_fops
};

static int __init fsl_dce_dev_init(void)
{
	int ret;

	pr_info("%s\n", __func__);
	ret = misc_register(&fsl_dce_dev);
	if (ret)
		pr_err("Could not register DCE device\n");
	return ret;
}

module_init(fsl_dce_dev_init);

static void __exit fsl_dce_dev_exit(void)
{
	pr_info("%s\n", __func__);
}

module_exit(fsl_dce_dev_exit);

MODULE_AUTHOR("Freescale Semiconductor Inc.");
MODULE_DESCRIPTION("Freescale's MC restool driver");
MODULE_LICENSE("GPL");
