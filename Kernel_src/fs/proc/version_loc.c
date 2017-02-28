#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>

char *local_kernel_ver="05.00.00";

static int version_loc_proc_show(struct seq_file *m, void *v)
{
        seq_printf(m, "kernel version: %s\n", local_kernel_ver);
	return 0;
}

static int version_loc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, version_loc_proc_show, NULL);
}

static const struct file_operations version_loc_proc_fops = {
	.open		= version_loc_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_version_loc_init(void)
{
	proc_create("kernel_ver", 0, NULL, &version_loc_proc_fops);
	return 0;
}
fs_initcall(proc_version_loc_init);
