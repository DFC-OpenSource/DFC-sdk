/*
 * QDMA Engine test module
 *
 * Copyright (C) VVDN 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/freezer.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include<linux/fs.h>
#include<linux/debugfs.h>
#include<linux/mm.h>
#include<linux/gfp.h>
#include<linux/types.h>

#ifndef VM_RESERVED
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#define DMA_START 0x1
#define DMA_GOING 0x2
#define DMA_DONE  0x3
#define DMA_FREE  0x4 /*callers responsibilty to make it free after they have done with it*/

int QDMA_DESC_COUNT=128;
module_param(QDMA_DESC_COUNT,int,0644);
#define QDMA_CHANNELS_COUNT 8
#define PAGE_ALLOC_ORDER 9

struct qdma_desc{
	volatile uint64_t src_addr;
	volatile uint64_t dest_addr;
	volatile uint32_t len;
	//volatile uint32_t status;
	atomic_t status;
};

static volatile int qdma_posted = 0;
static volatile int qdma_completed = 0;
static spinlock_t qdma_work_queue_lock;
static struct list_head qdma_work_queue;
struct task_struct *qdma_work;
int order; //2MB


static volatile int txd_null_cnt = 0;
static volatile int dma_submit_error_cnt = 0;
static volatile int thread_trace = 0;

static unsigned int qdma_bind_core_id=7;
module_param(qdma_bind_core_id, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(qdma_bind_core_id,"Core to bind thread");

struct dentry *file[QDMA_CHANNELS_COUNT];

struct mmap_info
{
	volatile char *data;
	volatile int allocated_order;
	volatile int dma_channel_no;
	volatile int reference;
	volatile int cur_index;
	volatile struct list_head list_element; 
};


void mmap_open(struct vm_area_struct *vma)
{
	struct mmap_info *info  =  (struct mmap_info *) vma->vm_private_data;
	printk("vvdn debug :  mmap_open for dma channel\n ");
	printk("channel no :%d\n",info->dma_channel_no);
	info->reference++;
}

void mmap_close(struct vm_area_struct *vma)
{
	struct mmap_info *info  =  (struct mmap_info *) vma->vm_private_data;
	printk("vvdn debug :  mmap_close for dma channel\n ");
	printk("channel no :%d\n",info->dma_channel_no);
	info->reference--;
}

//static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
        struct page *page;
        struct mmap_info *info;
        info = (struct mmap_info *) vma->vm_private_data;

	printk("vvdn debug : mmap fault dma channel %d\n",info->dma_channel_no);

        if(!info->data) {
                printk("No Data\n");
                return 0;
        }

        page = virt_to_page(info->data);

        get_page(page);
        vmf->page = page;

        return 0;
}

struct vm_operations_struct mmap_vm_ops = {
	.open  = mmap_open,
	.close = mmap_close,
	.fault = mmap_fault,// TODO if fault occurs
};

int op_mmap(struct file *filp, struct vm_area_struct *vma)
{
#if 0
    vma->vm_ops = &mmap_vm_ops;
    vma->vm_flags |= VM_RESERVED;
    vma->vm_private_data = filp->private_data;
    mmap_open(vma);
    return 0;
#endif

#if 1
    int status;
    unsigned int size;
    struct mmap_info *info;
    info = filp->private_data;
    size = vma->vm_end - vma->vm_start;
    printk("Mapping userspace vaddr : %lx and size :%x to phyaddr : %lx\n",
            vma->vm_start,size,virt_to_phys(info->data));
    status =  remap_pfn_range(vma,vma->vm_start,(virt_to_phys(info->data))>>PAGE_SHIFT,size,vma->vm_page_prot);
    printk("status of mmap : %d\n",status);
    return status;
#endif
}

void initalize_dma_descriptors(void *data, int num_desc)
{
	
	int i = 0;
	struct qdma_desc *dma_desc = (struct qdma_desc *)data;
	for(i = 0; i < num_desc; i++) {
		//dma_desc[i].status = DMA_FREE;
		atomic_set(&dma_desc[i].status,DMA_FREE);
		wmb();
	}
}

int mmapfop_open(struct inode *inode, struct file *filp,int channelno)
{

	int bytes_required;
	int pages_required;
	int order_required;
	struct mmap_info *info = kmalloc(sizeof(struct mmap_info),GFP_KERNEL);

	if(!info) {
		printk("mmap_info cannot be allocated for channel %d\n",channelno);
		return -ENOMEM;
	}
	else {
		printk("mmap_info allocated at vaddr %p for channelno %d\n",info,channelno);
	}

	bytes_required =  QDMA_DESC_COUNT*sizeof(struct qdma_desc);

	if( (PAGE_SIZE*(bytes_required/PAGE_SIZE)) == bytes_required ) {
		pages_required = bytes_required/PAGE_SIZE;
	}
	else {
		pages_required = (bytes_required/PAGE_SIZE) + 1;
	}

	order_required = ilog2(pages_required);

	if((1<<order_required) != pages_required) {
		order_required++;
	}

	printk("Required dma_channel : %d bytes req : %d pages req : %d pages order : %d\n",
			channelno,bytes_required,pages_required,order_required);	

	info->data = __get_free_pages(GFP_KERNEL,order_required);

	if(!info->data) {
		printk("qdma descriptor table cannot be allocated for channel %d\n",channelno);
		kfree(info);
		return -ENOMEM;
	}
	else {
		printk("qdma descriptor table allocated at vaddr : %p for channel : %d\n",info->data,channelno);
	}

	initalize_dma_descriptors(info->data,QDMA_DESC_COUNT);
	info->cur_index = 0;
	info->dma_channel_no = channelno;
	info->allocated_order = order_required;
	filp->private_data = info;

	spin_lock(&qdma_work_queue_lock);
	list_add_tail(&(info->list_element),&qdma_work_queue);
	spin_unlock(&qdma_work_queue_lock);

	printk("returning 0 from func : %s\n",__func__);

	return 0;
}

int mmapfop_close(struct inode *inode,struct file *filp)
{
	struct mmap_info *info = filp->private_data;

	printk("Freeing for channelno %d at info vaddr %p and data at vaddr  %p\n",
		info->dma_channel_no,info,info->data);

	spin_lock(&qdma_work_queue_lock);
	list_del_init(&(info->list_element));
	spin_unlock(&qdma_work_queue_lock);
	free_pages((unsigned long) info->data,info->allocated_order);
	kfree(info);
	filp->private_data = NULL;
	return 0;
}

/*MMAP for different QDMA channels */
int mmapfop_open_0(struct inode *inode, struct file *filp)
{
	return mmapfop_open(inode,filp,0);	
}

int mmapfop_open_1(struct inode *inode, struct file *filp)
{
	return mmapfop_open(inode,filp,1);	
}

int mmapfop_open_2(struct inode *inode, struct file *filp)
{
	return mmapfop_open(inode,filp,2);	
}

int mmapfop_open_3(struct inode *inode, struct file *filp)
{
	return mmapfop_open(inode,filp,3);	
}

int mmapfop_open_4(struct inode *inode, struct file *filp)
{
	return mmapfop_open(inode,filp,4);	
}

int mmapfop_open_5(struct inode *inode, struct file *filp)
{
	return mmapfop_open(inode,filp,5);	
}

int mmapfop_open_6(struct inode *inode, struct file *filp)
{
	return mmapfop_open(inode,filp,6);	
}

int mmapfop_open_7(struct inode *inode, struct file *filp)
{
	return mmapfop_open(inode,filp,7);	
}

static char qdma_status_string[5][15] = 
{
	"",/*0*/
	"DMA_START",/*1*/
	"DMA_GOING",/*2*/
	"DMA_DONE",/*3*/
	"DMA_FREE",/*4*/
};

static ssize_t fop_write (struct file *fp, const char __user * usp, size_t datalen, loff_t * loff_t_ptr)
{

	struct mmap_info *tmp;
	struct qdma_desc *qdesc;
	int i;
	tmp = fp->private_data;
	qdesc = tmp->data;
	printk("\n\n");
	printk("DMA CHANNEL : %02d  DESC_COUNT : %04d and CUR_INDEX : %d\n",tmp->dma_channel_no,QDMA_DESC_COUNT,tmp->cur_index);
	for(i=0;i<QDMA_DESC_COUNT;i++) {
		printk("[%04d] %016llx %016llx %08llx %s\n",i,
			qdesc[i].src_addr,
			qdesc[i].dest_addr,
			qdesc[i].len,
			qdma_status_string[atomic_read(&qdesc[i].status)]
			);
	}
	printk("\n\n");
	return datalen;
}


/*file operations for different debugfs files*/
static const struct file_operations mmap_fops_0 = {
	.open  = mmapfop_open_0,
	.release = mmapfop_close,
	.mmap  = op_mmap,
	.write = fop_write,
};

static const struct file_operations mmap_fops_1 = {
	.open  = mmapfop_open_1,
	.release = mmapfop_close,
	.mmap  = op_mmap,
	.write = fop_write,
};

static const struct file_operations mmap_fops_2 = {
	.open  = mmapfop_open_2,
	.release = mmapfop_close,
	.mmap  = op_mmap,
	.write = fop_write,
};

static const struct file_operations mmap_fops_3 = {
	.open  = mmapfop_open_3,
	.release = mmapfop_close,
	.mmap  = op_mmap,
	.write = fop_write,
};

static const struct file_operations mmap_fops_4 = {
	.open  = mmapfop_open_4,
	.release = mmapfop_close,
	.mmap  = op_mmap,
	.write = fop_write,
};

static const struct file_operations mmap_fops_5 = {
	.open  = mmapfop_open_5,
	.release = mmapfop_close,
	.mmap  = op_mmap,
	.write = fop_write,
};

static const struct file_operations mmap_fops_6 = {
	.open  = mmapfop_open_6,
	.release = mmapfop_close,
	.mmap  = op_mmap,
	.write = fop_write,
};

static const struct file_operations mmap_fops_7 = {
	.open  = mmapfop_open_7,
	.release = mmapfop_close,
	.mmap  = op_mmap,
	.write = fop_write,
};

void qdma_wrapper_callback(void *arg)
{
	struct qdma_desc *addr_st=(struct qdma_desc *)(arg);
	//addr_st->status=DMA_DONE;
	atomic_set(&addr_st->status,DMA_DONE);
	wmb();
	qdma_completed++;
	wmb();
}
EXPORT_SYMBOL_GPL(qdma_wrapper_callback);


long long int diff_gettimeofday(struct timeval *t1, struct timeval *t2) 
{
	long long int var1,var2,diff;
	var1 = 1000000*(t1->tv_sec) + t1->tv_usec;
	var2 = 1000000*(t2->tv_sec) + t2->tv_usec;

	if(var1 >= var2) {
		diff = var1 - var2;
	}
	else {
		diff = var2 - var1;
	}
	return diff;
}

void udelay_own(int num)
{
	struct timeval t1,t2;
	long long int diff = 0;

	do_gettimeofday(&t1);

	while(diff < num) {
		do_gettimeofday(&t2);
		diff = diff_gettimeofday(&t1,&t2);
	}
}

//static int qdma_userspace_interface(void *data)
int __cold qdma_userspace_interface(void *data)
{



	struct qdma_desc *cur_addr;
	struct dma_chan *dma_chan[QDMA_CHANNELS_COUNT];/*to store qdma engine dma channel */
	struct dma_device *dma_device[QDMA_CHANNELS_COUNT];/*to store device having multiple channels*/
	dma_cookie_t cookie;
	struct dma_async_tx_descriptor *txd;
	struct list_head *cur_list_element;
	struct mmap_info *cur_mmap_info;
	int i;


	/* qdma engine channel relted registrations*/
	dma_cap_mask_t mask;
	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);

	for(i=0;i<QDMA_CHANNELS_COUNT;i++) {
		dma_chan[i] = dma_request_channel(mask, NULL, NULL);
		if(!dma_chan[i])
		{
			//TODO filter channel
			printk("qdma-client unable to get channel %d , use cpu memcpy \n",i);
			//return -ENOMEM;
		}
		dma_device[i] = dma_chan[i]->device;
		printk("QDMA:%s qdma channel name %s \n",__func__,dma_chan_name(dma_chan[i]));
		if(dma_device[i]->copy_align)
		{
			// TODO
			printk("WARN :qdma-client alignment required align before starting dma transaction\n");
		}

		//	cur_addr[i] = qdma_desc_addr_base[i];
	}

	//printk("Will sleep for 2 seconds\n");
	//msleep(2000);
	//printk("proceeding with while loop\n");
#if 1
	while(1) {

		thread_trace = 1;
		//printk("locking to qdma_work_queue_lock");
		spin_lock(&qdma_work_queue_lock);

		if(list_empty(&qdma_work_queue)) {
			//printk("list is empty will recheck after a second\n");
			spin_unlock(&qdma_work_queue_lock);
			thread_trace = 2;
			msleep(1);
			continue;

		} else {
			thread_trace = 3;
			/*detach a soft dma descriptor table from linked list head*/
			cur_list_element = qdma_work_queue.next;
			list_del_init(cur_list_element);

			/*get the address of dma descriptor and dma channel to use*/
			cur_mmap_info = container_of(cur_list_element,struct mmap_info,list_element);
			cur_addr = cur_mmap_info->data +  (cur_mmap_info->cur_index)*sizeof(struct qdma_desc);
			i = cur_mmap_info->dma_channel_no /*i is dma channel*/;

			/*attach the soft dma descriptor table to linked list tail*/
			list_add_tail(cur_list_element,&qdma_work_queue);
		}

		spin_unlock(&qdma_work_queue_lock);
		//printk("unlocked from qdma_work_queue_lock");

		//if(cur_addr->status==DMA_START) {
		if(atomic_read(&cur_addr->status) == DMA_START) {
			//do qdma 
			thread_trace = 4;

			txd = dma_device[i]->device_prep_dma_memcpy(dma_chan[i],
					cur_addr->dest_addr,cur_addr->src_addr, 
					cur_addr->len,DMA_PREP_INTERRUPT|DMA_CTRL_ACK);

			if (!txd) {
				//TODO memcpy
				//memcpy(cur_addr->dest_addr,cur_addr->src_addr,cur_addr->len);
				txd_null_cnt++;
				//cur_addr->status =  DMA_DONE;
				atomic_set(&cur_addr->status,DMA_DONE);
				wmb();
				continue;
			}
			txd->callback = qdma_wrapper_callback;
			txd->callback_param = cur_addr;
			atomic_set(&cur_addr->status,DMA_GOING);
			wmb();
			thread_trace = 5;
			cookie = dmaengine_submit(txd);
			thread_trace = 6;
			if (dma_submit_error(cookie)) {
				//TODO memcpy           
				//memcpy(cur_addr->dest_addr,cur_addr->src_addr,cur_addr->len);
				thread_trace = 7;
				dma_submit_error_cnt++;
				//cur_addr->status =  DMA_DONE;
				atomic_set(&cur_addr->status,DMA_DONE);
				wmb();
				continue;
			}
			thread_trace = 8;
			dma_async_issue_pending(dma_chan[i]);
			thread_trace = 9;

			//cur_addr->status = DMA_GOING;
			//atomic_set(&cur_addr->status,DMA_GOING);
			//wmb();
			cur_mmap_info->cur_index++;
			if(cur_mmap_info->cur_index >= QDMA_DESC_COUNT) {
				cur_mmap_info->cur_index = 0;
			}
			qdma_posted++;
			wmb();

		}
		else {
			thread_trace = 10;
                        //udelay(1);
			usleep_range(10,20);
			//udelay_own(1);
		}
		//spin_unlock(&qdma_work_queue_lock);
	}
#endif
	return 0;
}

#define for_each_qdma_channel(i,count) \
for(i=0;i<count;i++) 

static void *dma_src_temp[QDMA_CHANNELS_COUNT], *dma_dest_temp[QDMA_CHANNELS_COUNT];
#define mmap_fops(n) mmap_fops_##n
static int __init qdmaclient_init(void)
{
	//struct qdma_desc *temp;
	int i,j;
	char debugfs_file_name[30];

	INIT_LIST_HEAD(&qdma_work_queue);
	spin_lock_init(&qdma_work_queue_lock);

	for_each_qdma_channel(i,QDMA_CHANNELS_COUNT) {
		sprintf(debugfs_file_name,"userdmachannel%02d",i);
		//file[i] = debugfs_create_file(debugfs_file_name,0644,NULL,NULL,&mmap_fops);
		switch(i) {

			case 0:
				file[i] = debugfs_create_file(debugfs_file_name,0644,NULL,NULL,&(mmap_fops(0)));
				break;
			case 1:
				file[i] = debugfs_create_file(debugfs_file_name,0644,NULL,NULL,&(mmap_fops(1)));
				break;
			case 2:
				file[i] = debugfs_create_file(debugfs_file_name,0644,NULL,NULL,&(mmap_fops(2)));
				break;
			case 3:
				file[i] = debugfs_create_file(debugfs_file_name,0644,NULL,NULL,&(mmap_fops(3)));
				break;
			case 4:
				file[i] = debugfs_create_file(debugfs_file_name,0644,NULL,NULL,&(mmap_fops(4)));
				break;
			case 5:
				file[i] = debugfs_create_file(debugfs_file_name,0644,NULL,NULL,&(mmap_fops(5)));
				break;
			case 6:
				file[i] = debugfs_create_file(debugfs_file_name,0644,NULL,NULL,&(mmap_fops(6)));
				break;
			case 7:
				file[i] = debugfs_create_file(debugfs_file_name,0644,NULL,NULL,&(mmap_fops(7)));
				break;

		}
	}


	qdma_work = kthread_create(qdma_userspace_interface,NULL,"qdma_wrapper");
	//qdma_work = kthread_create(qdma_userspace_interface_experimental,NULL,"qdma_wrapper");
	kthread_bind(qdma_work,qdma_bind_core_id);
	wake_up_process(qdma_work);/*starting thread*/

/* TEST MEMORY ALLOCATIONS */

	for_each_qdma_channel(i,QDMA_CHANNELS_COUNT){
		dma_src_temp[i] = kmalloc(1024*1024,GFP_KERNEL);
		dma_dest_temp[i] = kmalloc(1024*1024,GFP_KERNEL);

		if(dma_src_temp[i] == NULL) {
			printk("kmalloc failed to allocate temp buffer for source:%d\n",i);
		}
		if(dma_dest_temp[i] == NULL) {
			printk("kmalloc failed to allocate temp buffer for destination:%d\n",i);
		}

		printk("temp buffers for dma usage are 1MB each at physical locations : %p and %p for qdma channel %d\n",
				virt_to_phys(dma_src_temp[i]),virt_to_phys(dma_dest_temp[i]),i);	
	}

/* TEST MEMORY ALLOCATIONS */

	return 0;
}
/* when compiled-in wait for drivers to load first so qdma can load */
late_initcall(qdmaclient_init);

static void __exit qdmaclient_exit(void)
{
	//TODO cleanup part
	#if 1
	int i;
	for_each_qdma_channel(i,QDMA_CHANNELS_COUNT) {
		kfree(dma_src_temp[i]);
		kfree(dma_dest_temp[i]);
	}
	#endif
}
module_exit(qdmaclient_exit);

/*sysfs entries for testing*/

static int qdma_src_buffers_data_verify(const char *val, const struct kernel_param *kp)
{

	int i;
	for_each_qdma_channel(i,QDMA_CHANNELS_COUNT) {
		if (memcmp(dma_src_temp[i],dma_dest_temp[i],1024*1024) == 0) {
			printk("qdma temp buffers for qdma channel %d are same\n",i);
		} else {
			printk("qdma temp buffers for qdma channel %d are not same\n",i);
		}
	}

	printk("reinitalized qdma temp buffers\n");
	return 0;
	
}

static struct kernel_param_ops param_ops_verify_data = {
        .set = qdma_src_buffers_data_verify,
        .get = NULL,
};
module_param_cb(qdma_buf_data_verify, &param_ops_verify_data, NULL, 0200);


static int qdma_src_buffers_data_init(const char *val, const struct kernel_param *kp)
{

	int i;
	for_each_qdma_channel(i,QDMA_CHANNELS_COUNT) {
		memset(dma_src_temp[i],i,1024*1024);
		if(i != (255-i)) {
			memset(dma_dest_temp[i],(255-i),1024*1024);
		}
		else {
			memset(dma_dest_temp[i],(255-i+1),1024*1024);
		}
	}

	printk("reinitalized qdma temp buffers\n");
	return 0;
	
}

static struct kernel_param_ops param_ops_set_src_data = {
        .set = qdma_src_buffers_data_init,
        .get = NULL,
};
module_param_cb(qdma_buf_data_init, &param_ops_set_src_data, NULL, 0200);


static int qdma_counters(const char *val, const struct kernel_param *kp)
{

	wmb();
	printk("txd_null_cnt         : %d\n",txd_null_cnt);
	printk("dma_submit_error_cnt : %d\n",dma_submit_error_cnt);
	printk("qdma_posted         : %d\n",qdma_posted);
	printk("qdma_completed      : %d\n",qdma_completed);
	printk("thread_trace      : %d\n",thread_trace);

	return 0;
}

static struct kernel_param_ops param_ops_qdma_counters = {
        .set = qdma_counters,
        .get = NULL,
};
module_param_cb(qdma_counters, &param_ops_qdma_counters, NULL, 0200);

/*sysfs entries for testing*/


MODULE_AUTHOR("VVDN FSLU_NVME");
MODULE_LICENSE("GPL v2");
