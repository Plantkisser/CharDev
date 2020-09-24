#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>


#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/fcntl.h>
#include <linux/slab.h>	
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

#ifndef MY_MAJOR
#define MY_MAJOR 0
#endif

#ifndef MY_NUMBER_OF_DEV
#define MY_NUMBER_OF_DEV 4
#endif

#ifndef BUF_SIZE
#define BUF_SIZE 20
#endif




struct data
{
	char buf[BUF_SIZE];

	int rd_off, wr_off;
	int size;
};

struct data *data;

MODULE_AUTHOR("Acro Team");
MODULE_LICENSE("GPL");







int my_open(struct inode *inode, struct file *filp)
{


	return 0;
}

int my_release(struct inode *inode, struct file *filp)
{
	return 0;
}


int get_dataptr(struct data *data, char **pstr, size_t count)
{
	if (data->wr_off - data->rd_off  < 0) {
		count = -1;
		goto out;
	}

	if (data->wr_off - data->rd_off < count) 
		count = data->wr_off - data->rd_off;

	*pstr = data->buf + (data->rd_off % data->size);
	data->rd_off += count;

out:
	return count;
}

ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	char *str = NULL;
	int read_smbl = get_dataptr(data, &str, count);

	printk(KERN_NOTICE "READ_SMBL = %d", read_smbl);

	if (read_smbl < 0)
		goto out;
	if (read_smbl == 0) {
		copy_to_user(buf, "h", 2);
		goto out;
	}

	if (copy_to_user(buf, str, read_smbl)); 
		//return -EFAULT;

	*f_pos += read_smbl;

out:
	return read_smbl;
}

int write_user_data(struct data *data, const char __user *buf, size_t count)
{
	if (data->wr_off - data->rd_off < 0) {
		data->rd_off = data->wr_off;
	}

	count = count > data->size ? data->size : count; // to avoid problems with overflowing

	if ((data->wr_off + count - data->rd_off + 1) > data->size)  
		count = data->size - data->wr_off + data->rd_off - 1;

	if (copy_from_user(data->buf + (data->wr_off + 1 % data->size), buf, count))
		return -EFAULT;

	data->wr_off += count;

	return count;
}


ssize_t my_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) 
{
	int written_smbl = write_user_data(data, buf, count);

	*f_pos += written_smbl;

	return written_smbl;
}


struct file_operations my_fops = {
	.owner 	=    	THIS_MODULE,
	.read 	=   	my_read,
	.write 	=		my_write,
	.open 	= 		my_open,
	.release=		my_release
};

struct miscdevice miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &my_fops,
	.name = "chrdev",
};	



void __exit my_cleanup_module(void) 
{
	misc_deregister(&miscdevice);

	kfree(data);
}


int __init my_init_module(void) 
{
	int ret = 0;

	data = kmalloc(sizeof(struct data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto out;
	}
	memset(data, 0, sizeof(struct data));

	data->size = BUF_SIZE;
	data->rd_off = 0;
	data->wr_off = 0;


	ret = misc_register(&miscdevice);

	if (ret) {
		goto out;
	}

out: 
	return ret;
}



module_init(my_init_module);
module_exit(my_cleanup_module);



