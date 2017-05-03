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
/*
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/kthread.h>
#include <linux/delay.h> 
*/
#include "debug.h"
#include "params.h"
#include "bdbm_drv.h"
#include "ioctl.h"


/* NOTE: refer to the following URLs 
 * [1] linux-source/drivers/mtd/mtd_blkdevs.c
 * [2] linux-source/drivers/mtd/nftlcore.c */

extern struct bdbm_drv_info* _bdi;

/*DECLARE_COMPLETION(task_completion); */
static struct task_struct *task = NULL;


int badblock_scan_thread_fn (void* arg) 
{
	struct bdbm_ftl_inf_t* ftl = NULL;
	uint32_t ret;

	/* get the ftl */
	if ((ftl = _bdi->ptr_ftl_inf) == NULL) {
		bdbm_warning ("ftl is not created");
		goto exit;
	}

	/* run the bad-block scan */
	if ((ret = ftl->scan_badblocks (_bdi))) {
		bdbm_msg ("scan_badblocks failed (%u)", ret);
	}

exit:
	/*complete (&task_completion); */
	return 0;
}
#if 0
int bdbm_blk_getgeo (struct block_device *bdev, struct hd_geometry* geo)
{
	struct nand_params* np = &_bdi->ptr_bdbm_params->nand;
	int nr_sectors = np->device_capacity_in_byte >> 9;

	/* NOTE: Heads * Cylinders * Sectors = # of sectors (512B) in SSDs */
	geo->heads = 16;
	geo->cylinders = 1024;
	geo->sectors = nr_sectors / (geo->heads * geo->cylinders);
	if (geo->heads * geo->cylinders * geo->sectors != nr_sectors) {
		bdbm_warning ("bdbm_blk_getgeo: heads=%d, cylinders=%d, sectors=%d (total sectors=%d)",
			geo->heads, 
			geo->cylinders, 
			geo->sectors,
			nr_sectors);
		return 1;
	}
	return 0;
}

int bdbm_blk_ioctl (struct block_device *bdev, fmode_t mode, unsigned cmd, unsigned long arg)
{
	struct hd_geometry geo;
	struct gendisk *disk = bdev->bd_disk;
	int ret;

	switch (cmd) {
	case HDIO_GETGEO:
	case HDIO_GETGEO_BIG:
	case HDIO_GETGEO_BIG_RAW:
		if (!arg) {
			bdbm_warning ("invalid argument");
			return -EINVAL;
		}
		if (!disk->fops->getgeo) {
			bdbm_warning ("disk->fops->getgeo is NULL");
			return -ENOTTY;
		}

		bdbm_memset(&geo, 0, sizeof(geo));
		geo.start = get_start_sect(bdev);
		ret = disk->fops->getgeo(bdev, &geo);
		if (ret) {
			bdbm_warning ("disk->fops->getgeo returns (%d)", ret);
			return ret;
		}
		if (copy_to_user((struct hd_geometry __user *)arg, &geo, sizeof(geo))) {
			bdbm_warning ("copy_to_user failed");
			return -EFAULT;
		}
		break;

	case BDBM_BADBLOCK_SCAN:
		bdbm_msg ("Get a BDBM_BADBLOCK_SCAN command: %u (%X)", cmd, cmd);

		if (task != NULL) {
			bdbm_msg ("badblock_scan_thread is running");
		} else {
			/* create thread */
			if ((task = kthread_create (badblock_scan_thread_fn, NULL, "badblock_scan_thread")) == NULL) {
				bdbm_msg ("badblock_scan_thread failed to create");
			} else {
				wake_up_process (task);
			}
		}
		break;

	case BDBM_BADBLOCK_SCAN_CHECK:
		/* check the status of the thread */
		if (task == NULL) {
			bdbm_msg ("badblock_scan_thread is not created...");
			ret = 1; /* done */
			copy_to_user ((int*)arg, &ret, sizeof (int));
			break;
		}

		/* is it still running? */
		if (!bdbm_try_wait_for_completion (task_completion)) {
			ret = 0; /* still running */
			copy_to_user ((int*)arg, &ret, sizeof (int));
			break;
		}
		ret = 1; /* done */
		
		/* reinit some variables */
		task = NULL;
		copy_to_user ((int*)arg, &ret, sizeof (int));
		bdbm_reinit_completion (task_completion);
		break;

	case BDBM_GET_PHYADDR:
		/* get a physical address */
		{
			/*struct phyaddr paddr;*/
			/* copy_from_user */
			/* copy_to_user */
		}
		break;

	default:
		/*bdbm_msg ("unknown bdm_blk_ioctl: %u (%X)", cmd, cmd);*/
		break;
	}

	return 0;
}
#endif
