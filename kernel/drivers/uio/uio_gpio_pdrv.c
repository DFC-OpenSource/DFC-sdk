#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/uio_driver.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/irqdomain.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>

#define DRIVER_VERSION	"0.02.0"
#define DRIVER_AUTHOR	"Michael S. Tsirkin <mst@redhat.com>"
#define DRIVER_DESC	"Generic UIO driver for PCI 2.3 devices"
//#define GPIO_INT

#define MEMALLOC_4MB	80	
static unsigned long long int phy_add[MEMALLOC_4MB];
static void *v_add[MEMALLOC_4MB];
static dev_t dev; 
static struct cdev c_dev;
static struct class *cl;

static struct of_device_id gic_match[] = {
    { .compatible = "arm,gic-v3", },
    { },
};

static struct file_operations fops;
static struct phy_addr {
	unsigned long long int addr;
};
static struct device_node *gic_node;

struct uio_pci_generic_dev {
    struct uio_info info;
    struct pci_dev *pdev;
};

static int no_gpiointerrupts = 4;
static struct platform_device *pldev[14];

static unsigned int hardinterrupt(unsigned int hwirq)
{
    struct of_phandle_args irq_data;
    unsigned int irqn;

    if (!gic_node)
        gic_node = of_find_matching_node(NULL, gic_match);

    if (WARN_ON(!gic_node)){
        printk(KERN_ALERT "%s %d Error IN GIC_NODE\n", __func__, __LINE__);
        return hwirq;
    }

    irq_data.np = gic_node;
    irq_data.args_count = 3;
    irq_data.args[0] = 0;
    irq_data.args[1] = hwirq - 32;
    irq_data.args[2] = IRQ_TYPE_EDGE_RISING;

    irqn = irq_create_of_mapping(&irq_data);
    if (WARN_ON(!irqn)){
        printk(KERN_ALERT "%s %d HW irq %d is not mapped with virtual IRQ number\n", __func__, __LINE__, hwirq);
        irqn = hwirq;
    }

    printk(KERN_ALERT "%s %d Virtual IRQN %d\n", __func__, __LINE__, irqn);
    return irqn;
}

static int gc_phy_addr(void)
{
	static int ret, i;
	struct device *dev_ret;
	static unsigned long int size = 4 * 1024UL * 1024UL;

	if ((ret = alloc_chrdev_region(&dev, 0, 1, "pci")) < 0) {
		return ret;
	}
	if (IS_ERR(cl = class_create(THIS_MODULE, "chardrv"))) {
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(cl);
	}
	if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "uio_rc"))) {
		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(dev_ret);
	}

	cdev_init(&c_dev, &fops);
	if ((ret = cdev_add(&c_dev, dev, 1)) < 0) {
		device_destroy(cl, dev);
		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
		return ret;
	}

	for(i = 0; i < MEMALLOC_4MB; i++) {
		v_add[i] = kmalloc(size,GFP_KERNEL);
		if(v_add[i] == NULL) {
			printk("Memory not allocated\n"); 
			goto free_memory;
		}
		memset(v_add[i], 0, size);
		phy_add[i] = __pa(v_add[i]);
	}
	return 0;
free_memory:
	for(ret = 0; ret < i; ret++) {
		if(v_add[i]) {
			kfree(v_add[i]);
		}
	}
	return -1;
}

static int __init ls2_ep_init(void)
{
    char *name = "uio_pdrv_genirq";
    struct uio_info uioinfoarr[no_gpiointerrupts];
    int irq[no_gpiointerrupts], i, val, err;
    struct uio_info *uioinfo = NULL;
    struct device *dev = NULL;

    irq[0] = hardinterrupt(40);
    irq[1] = hardinterrupt(41);
    irq[2] = hardinterrupt(42);
    irq[3] = hardinterrupt(43);

    for(i = 0; i < no_gpiointerrupts; i++) {
        val = i;	/*7+i ==> (num_of_interrupt_in_pex2 -1) + i*/
        pldev[val] = kzalloc(sizeof(struct platform_device), GFP_KERNEL);
        pldev[val] = platform_device_alloc(name, val);
        if (pldev[val]) {
            uioinfo = &uioinfoarr[i];
            uioinfo = dev_get_platdata(&pldev[i]->dev);
            uioinfo = kzalloc(sizeof(*uioinfo), GFP_KERNEL);
            uioinfo->name = "uio_platform";
            uioinfo->version = "0";
            uioinfo->irq = irq[i];
            pldev[val]->dev.platform_data = uioinfo;
            err = platform_device_add_resources(pldev[val], NULL, 0);
            if (err == 0){
                err = platform_device_add(pldev[val]);
                if(err)
                    dev_err(&(pldev[val]->dev), "unable to register platform device\n");
            }
        } else {
            err = -ENOMEM;
        }
    }
    if ((err = gc_phy_addr()) < 0) {
        printk("Physical memory for garbage collection is failed\n");
    }
    return 0;

}

static void __exit ls2_ep_exit(void)
{
    int i, val;

    for(i = 0; i < no_gpiointerrupts; i++) {
        val = i;
        if (pldev[val]) {
            platform_device_del(pldev[val]);
            kfree(pldev[val]);
        }
    }
    for (i=0; i<MEMALLOC_4MB; i++) {
        if(v_add[i]) {
            kfree(v_add[i]);
        }
    }
    cdev_del(&c_dev);
    device_destroy(cl, dev);
    class_destroy(cl);
    unregister_chrdev_region(dev, 1);
}

static int uio_open(struct inode *inode, struct file *fil)
{
	printk("UIO character driver opened \n");
	return 0;
}

static int uio_release(struct inode *inode, struct file *fil)
{
	printk("UIO character driver released \n");
	return 0;
}

static long int uio_read(struct file *fp, char *buffer, size_t size)
{
	int i;
	struct phy_addr *buff;
	buff = kmalloc(sizeof(struct phy_addr) * (size >> 3),GFP_KERNEL);
	for(i = 0; (i < (size >> 3)) && (i < MEMALLOC_4MB); i++){
		buff[i].addr = phy_add[i];
	}
	if(copy_to_user(buffer, (char *)buff, size) < 0) {
		printk("Read Failed \n");
		return -EFAULT;
	}
	kfree(buff);
	return size;
}

static struct file_operations fops=
{
	.read = uio_read,
	.open = uio_open,
	.release = uio_release,
};

module_init(ls2_ep_init);
module_exit(ls2_ep_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
