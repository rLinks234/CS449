/*
 * "Hello, world!" minimal kernel module - /dev version
 *
 * Valerie Henson <val@nmt.edu>
 *
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/random.h>
#include <asm/uaccess.h>

static unsigned char get_random_byte(int max) {
			
	unsigned char c;
			
	get_random_bytes(&c, 1);
			
	return c % max;
			
}

static ssize_t dice_read(struct file* file, char* buf, size_t count, loff_t* ppos) {    
	
	int i;
	char* data;    

	if(count == 0){
		return 0;
	}

	data = kmalloc(count, GFP_KERNEL);

	for(i = 0; i < count; i++) {
		data[i] = get_random_byte(6) + 1;
	}

	if(copy_to_user(buf, data, count)) {
		kfree(data);
	}


	*ppos += count;

	return *ppos;

}

static const struct file_operations dice_fops = {

	.owner		= THIS_MODULE,
	.read		= dice_read

};

static struct miscdevice dice_dev = {

	MISC_DYNAMIC_MINOR,
	/*
	 * Name ourselves /dev/hello.
	 */
	"dice",
	/*
	 * What functions to call when a program performs file
	 * operations on the device.
	 */
	&dice_fops

};

static int __init dice_init(void) {

	int ret;

	/*
	 * Create the "hello" device in the /sys/class/misc directory.
	 * Udev will automatically create the /dev/hello device using
	 * the default rules.
	 */
	ret = misc_register(&dice_dev);
	if (ret)
		printk(KERN_ERR
		       "Unable to register \"Hello, world!\" misc device\n");

	return ret;

}

module_init(dice_init);

static void __exit dice_exit(void) {
	misc_deregister(&dice_dev);
}

module_exit(dice_exit);

MODULE_AUTHOR("Daniel Wright <djw67@pitt.edu>");
MODULE_DESCRIPTION("\"Dice\" minimal module");
MODULE_VERSION("dev");