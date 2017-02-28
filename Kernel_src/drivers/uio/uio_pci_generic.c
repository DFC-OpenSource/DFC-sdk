/* uio_pci_generic - generic UIO driver for PCI 2.3 devices
 *
 * Copyright (C) 2009 Red Hat, Inc.
 * Author: Michael S. Tsirkin <mst@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 *
 * Since the driver does not declare any device ids, you must allocate
 * id and bind the device to the driver yourself.  For example:
 *
 * # echo "8086 10f5" > /sys/bus/pci/drivers/uio_pci_generic/new_id
 * # echo -n 0000:00:19.0 > /sys/bus/pci/drivers/e1000e/unbind
 * # echo -n 0000:00:19.0 > /sys/bus/pci/drivers/uio_pci_generic/bind
 * # ls -l /sys/bus/pci/devices/0000:00:19.0/driver

 * # modprobe uio_pci_generic
 * # ////%%%///echo "1665 3140" > /sys/bus/pci/drivers/uio_pci_generic/new_id
 * # echo -n 0000:01:00.0 > /sys/bus/pci/drivers/ls2_rc/unbind
 * # echo -n 0000:01:00.0 > /sys/bus/pci/drivers/uio_pci_generic/bind
 * # ls -l /sys/bus/pci/devices/0000\:01\:00.0/driver
 * .../0000:00:19.0/driver -> ../../../bus/pci/drivers/uio_pci_generic
 *
 * Driver won't bind to devices which do not support the Interrupt Disable Bit
 * in the command register. All devices compliant to PCI 2.3 (circa 2002) and
 * all compliant PCI Express devices should support this bit.
 */

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
#include <inic_config.h>

#define DRIVER_VERSION	"0.02.0"
#define DRIVER_AUTHOR	"Michael S. Tsirkin <mst@redhat.com>"
#define DRIVER_DESC	"Generic UIO driver for PCI 2.3 devices"
#define UIO_MSI
#define BAR0	0
#define MEMALLOC_4MB	80	
//#define GPIO_INT
#define VENDOR_ID 0x1957
#define DEVICE_ID 0x0953
#define CLASS_ID  0x010802

#define PCIE_REGS_START         0x3600000
#define PCIE_REGS_SIZE          16*1024UL

#define DBI_REG                 0x8bc
#define LUT_CTRL_REG            0x807f8
#define INDEX_REG               0x900 
#define CONTROL1_REG            0x904
#define CONTROL2_REG            0x908
#define BASE_ADD_LOW            0x90c
#define BASE_ADD_HIGH           0x910
#define ADD_RANGE_REG           0x914
#define TARGET_ADD_LOW          0x918
#define TARGET_ADD_HIGH         0x91c

static char pci_num = 0;
static char pci_num_check = 0;
static unsigned long long int phy_add[MEMALLOC_4MB];
static void *v_add[MEMALLOC_4MB];
static dev_t dev; 
static struct cdev c_dev;
static struct class *cl;

static struct
pci_device_id pci_drv_ids[]  = {
	{ PCI_DEVICE(VENDOR_ID, DEVICE_ID), },
	{ 0, }
};

#ifdef GPIO_INT
static struct of_device_id gic_match[] = {
	{ .compatible = "arm,gic-v3", },
	{ },
};
#endif

static struct phy_addr {
	unsigned long long int addr;
};

struct uio_pdrv_genirq_platdata {
	struct uio_info *uioinfo;
	spinlock_t lock;
	unsigned long flags;
	struct platform_device *pdev;
};

static struct device_node *gic_node;
static struct file_operations fops;

struct uio_pci_generic_dev {
	struct uio_info info;
	struct pci_dev *pdev;
#ifdef UIO_MSI
	bool have_msi;
#endif    
};

static int inbound_config(int bar_no, uint64_t bar_address)
{
	uint32_t address = bar_address & 0xffffffff;
	uint32_t bar_low = address;
	uint32_t bar_high = (bar_address >> 32);
	uint32_t control2 = 0xc0000000 + (bar_no << 8);
	void __iomem *reg_vir;

	reg_vir = ioremap_nocache(PCIE_REGS_START, PCIE_REGS_SIZE);
	if (!reg_vir) {
		printk(KERN_ERR "Register remapping failed\n");
		return -1;
	}

	writel(1, reg_vir + DBI_REG);
	writel(0x80000003 + bar_no, reg_vir + INDEX_REG);
	writel(bar_low, reg_vir + TARGET_ADD_LOW);
	writel(bar_high, reg_vir + TARGET_ADD_HIGH);
	writel(0, reg_vir + CONTROL1_REG);
	writel(control2, reg_vir + CONTROL2_REG);
	writel(0, reg_vir + DBI_REG);

	iounmap(reg_vir);
	return 0;
}

static int outbound_config(void)
{
	void __iomem *reg_vir;
	reg_vir = ioremap_nocache(PCIE_REGS_START, PCIE_REGS_SIZE);
	if (!reg_vir) {
		printk(KERN_ERR "Register remapping failed\n");
		return -1;
	}

    writel(0x00000000, reg_vir + INDEX_REG);
	writel(0xffffffff, reg_vir + ADD_RANGE_REG);
	writel(0x00000000, reg_vir + TARGET_ADD_LOW);
    writel(0x00000000, reg_vir + TARGET_ADD_HIGH);
    writel(0x00000000, reg_vir + BASE_ADD_LOW);
    writel(0x14, reg_vir + BASE_ADD_HIGH);
    writel(0, reg_vir + CONTROL1_REG);
    writel(0x80000000, reg_vir + CONTROL2_REG);

    writel(0x00000001, reg_vir + INDEX_REG);
    writel(0xffffffff, reg_vir + ADD_RANGE_REG);
    writel(0x00000000, reg_vir + TARGET_ADD_LOW);
    writel(0x00000001, reg_vir + TARGET_ADD_HIGH);
    writel(0x00000000, reg_vir + BASE_ADD_LOW);
    writel(0x15, reg_vir + BASE_ADD_HIGH);
    writel(0, reg_vir + CONTROL1_REG);
    writel(0x80000000, reg_vir + CONTROL2_REG);

    iounmap(reg_vir);
	return 0;
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

static int end_point_configuration(uint64_t bar0)
{
	int ret = 0;
	if ((ret = inbound_config(BAR0, bar0)) < 0) {
		return ret;
	}
    if ((ret = outbound_config()) < 0) {
		return ret;
	}
	return ret;
} 

static inline struct uio_pci_generic_dev *to_uio_pci_generic_dev(struct uio_info *info)
{
	return container_of(info, struct uio_pci_generic_dev, info);
}

/* Interrupt handler. Read/modify/write the command register to disable
 * the interrupt. */
static irqreturn_t irqhandler(int irq, struct uio_info *info)
{
	struct uio_pci_generic_dev *gdev = to_uio_pci_generic_dev(info);

#ifdef UIO_MSI
	if (!gdev->have_msi && !pci_check_and_mask_intx(gdev->pdev))
#else
		if (!pci_check_and_mask_intx(gdev->pdev))
#endif    
			return IRQ_NONE;

	/* UIO core will signal the user process. */
	return IRQ_HANDLED;
}

#ifdef GPIO_INT
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
#endif

static int probe(struct pci_dev *pdev,
		const struct pci_device_id *id)
{
	struct uio_pci_generic_dev *gdev;
	int err, no_interrupts = 0;
	int i;
	unsigned char bar_num=0;
	int val;
	struct uio_info *uioinfo = NULL;
	char *name = "uio_pdrv_genirq";
	void __iomem *reg_vir;
#ifdef UIO_MSI
	bool have_msi = true;
#endif

	reg_vir = ioremap_nocache(PCIE_REGS_START, PCIE_REGS_SIZE);
	if (!reg_vir) {
		printk(KERN_ERR "Register remapping failed\n");
		return 0;
	}
	writel(0, reg_vir + LUT_CTRL_REG);
	iounmap(reg_vir);
	pci_num++;
	printk("UIO_PCI_GENERIC Device 0x%x has been found at"
			" bus %d dev %d func %d pci_num %d\n",
			pdev->device, pdev->bus->number, PCI_SLOT(pdev->devfn),
			PCI_FUNC(pdev->devfn), pci_num);

	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "%s: pci_enable_device failed: %d\n",
				__func__, err);
		printk("%s %d err : %d\n",__func__,__LINE__,err);
		return err;
	}	/* Enable Message Signaled Interrupt */
	/* set master */
	pci_set_master(pdev);

	gdev = kzalloc(sizeof(struct uio_pci_generic_dev), GFP_KERNEL);
	if (!gdev) {
		err = -ENOMEM;
		printk("%s %d err : %d\n",__func__,__LINE__,err);
		goto err_alloc;
	}

	for (i = 0; i < PCI_NUM_RESOURCES; i++) {
		if (pdev->resource[i].flags &&
				(pdev->resource[i].flags & IORESOURCE_IO)) {
			gdev->info.port[i].size = 0;
			gdev->info.port[i].porttype = UIO_PORT_OTHER;
#ifdef CONFIG_X86
			gdev->info.port[i].porttype = UIO_PORT_X86;
#endif
		}

		if (pdev->resource[i].flags &&
				(pdev->resource[i].flags & IORESOURCE_MEM)) {
			gdev->info.mem[i].addr = pci_resource_start(pdev, i);
			printk("UIO_PCI_GENERIC pcie: BAR%d --> 0x%x\n", i, (unsigned int)gdev->info.mem[i].addr);
			gdev->info.mem[i].size = pci_resource_len(pdev, i);
			gdev->info.mem[i].internal_addr = NULL;
			gdev->info.mem[i].memtype = UIO_MEM_PHYS;
			//bar_num++;
		}
	}

#ifdef UIO_MSI
	pci_disable_msi(pdev);
	no_interrupts = 8;
#if 0
	if(pci_num == 1){
		no_interrupts = 8;
	}else if(pci_num == 2){
		no_interrupts = 4;
	}
#endif
	err = pci_enable_msi_range(pdev, 1, no_interrupts);
	if (err) {
		have_msi = true;
		printk("MSI UIO is enabled %d\n", err);
	}else{
		printk("MSI UIO failed to enable err: %d\n", err);
		if (!pci_intx_mask_supported(pdev)) {
#else
			if (!pci_intx_mask_supported(pdev)) {
#endif    
				err = -ENODEV;
				printk("%s %d !msi err : %d\n",__func__,__LINE__,err);
				goto err_verify;
			}
#ifdef UIO_MSI
		}
#endif

		printk("UIO_PCI_GENERIC IRQ assigned to device: %d \n",pdev->irq);

#ifdef UIO_MSI
		gdev->have_msi = have_msi;
#endif    
		gdev->info.name = "uio_pci_generic";
		gdev->info.version = DRIVER_VERSION;
#ifdef GPIO_INT
		gdev->info.irq = hardinterrupt(42);
#else
		gdev->info.irq = pdev->irq;
#endif    
#ifdef UIO_MSI
		gdev->info.irq_flags = have_msi ? 0 : IRQF_SHARED;
#else
		gdev->info.irq_flags = IRQF_SHARED;
#endif    
		gdev->info.handler = irqhandler;
		gdev->pdev = pdev;
		/* request regions */
		err = pci_request_regions(pdev, "uio_pci_generic");
		if (err) {
			dev_err(&pdev->dev, "Couldn't get PCI resources, aborting\n");
			printk("%s %d err : %d\n",__func__,__LINE__,err);
			return err;
		}

		/* create attributes for BAR mappings */
#if 0
		if ( (pdev->irq) && (err = request_irq(pdev->irq, irqhandler, IRQF_SHARED, "uio_pci_generic", pdev))) {
			printk(KERN_ERR "pci_template: IRQ %d not free. %d\n", pdev->irq ,err);
		}
#endif
		err = uio_register_device(&pdev->dev, &gdev->info);
		if (err){
			printk("UIO PCI GENERIC driver registered is failed\n");
			goto err_register;
		}
		pci_set_drvdata(pdev, &gdev->info);
		pr_info("UIO_PCI_GENERIC : initialized new device (%x %x)\n",pdev->vendor, pdev->device);
		if(pci_num == 1) {	/*End point configuration for the PEX3 after getting PEX2 details*/
			if ((err = end_point_configuration(gdev->info.mem[2].addr)) < 0) {
				printk("END POINT CONFIGURATION is failed\n");
			}
			if ((err = gc_phy_addr()) < 0) {
				printk("Physical memory for garbage collection is failed\n");
			}
		}

		{
			struct platform_device *pldev[no_interrupts];
			struct uio_info uioinfoarr[no_interrupts];
#if 1
			for(i = 0; i < (no_interrupts - 1); i++) {
				pldev[i] = devm_kzalloc(&pdev->dev, sizeof(struct platform_device), GFP_KERNEL);
				val = (pci_num == 1) ? i : 7+i;	/*7+i ==> (num_of_interrupt_in_pex2 -1) + i*/
				pldev[i] = platform_device_alloc(name, val);
				if (pldev[i]) {
					uioinfo = &uioinfoarr[i];
					uioinfo = dev_get_platdata(&pldev[i]->dev);
					uioinfo = devm_kzalloc(&pdev->dev, sizeof(*uioinfo), GFP_KERNEL);
					uioinfo->name = "uio_platform";
					uioinfo->version = "0";
					uioinfo->irq = pdev->irq + 1 + i;
					pldev[i]->dev.platform_data = uioinfo;
					err = platform_device_add_resources(pldev[i], NULL, 0);
					if (err == 0){
						err = platform_device_add(pldev[i]);
						if(err)
							dev_err(&(pldev[i]->dev), "unable to register platform device\n");
					}
				} else {
					err = -ENOMEM;
				}
			}
		}
#endif
		return 0;
err_register:
		kfree(gdev);
err_alloc:
#ifdef UIO_MSI
		if (have_msi){
			pci_disable_msi(pdev);
		}
#endif    
err_verify:
		pci_disable_device(pdev);
		printk("%s %d err : %d\n",__func__,__LINE__,err);
		return err;
}

static void remove(struct pci_dev *pdev)
{
	int i;
	struct uio_pci_generic_dev *gdev = pci_get_drvdata(pdev);

	uio_unregister_device(&gdev->info);
	pci_release_regions(pdev);
#ifdef UIO_MSI
	if (gdev->have_msi){
		pci_disable_msi(pdev);
	}
#endif    
	for(i = 0; i < MEMALLOC_4MB; i++) {
		if(v_add[i]) {
			kfree(v_add[i]);
		}
	}
	pci_disable_device(pdev);
	kfree(gdev);
	cdev_del(&c_dev);
	device_destroy(cl, dev);
	class_destroy(cl);
	unregister_chrdev_region(dev, 1);

}

static struct pci_driver uio_pci_driver = {
	.name = "uio_pci_generic",
	.id_table = pci_drv_ids, /* only dynamic id's */
	.probe = probe,
	.remove = remove,
};

static int uio_open(struct inode *inode, struct file *fil)
{
	printk(KERN_ALERT,"UIO character driver opened \n");
	return 0;
}

static int uio_release(struct inode *inode, struct file *fil)
{
	printk(KERN_ALERT,"UIO character driver released \n");
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

module_pci_driver(uio_pci_driver);
MODULE_VERSION(DRIVER_VERSION);
MODULE_DEVICE_TABLE(uio,pci_drv_ids);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
