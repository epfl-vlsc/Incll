#include <linux/init.h>           
#include <linux/module.h>         
#include <linux/device.h>        
#include <linux/kernel.h>         
#include <linux/fs.h>            
#include <linux/uaccess.h>    
#define  DEVICE_NAME "global_flush"    
#define  CLASS_NAME  "gf"

MODULE_LICENSE("GPL");  
MODULE_AUTHOR("VLSC");   
MODULE_DESCRIPTION("WBINVD — Write Back and Invalidate Cache");
MODULE_VERSION("1.0"); 

static int majorNumber;
static struct class*  gfClass  = NULL; 
static struct device* gfDevice = NULL;

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops ={
   .open = dev_open,
   .release = dev_release,
   .write = dev_write,
};

static int __init gf_init(void){
   printk(KERN_INFO "Kernel: Init GF\n");

   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "GF failed to register\n");
      return majorNumber;
   }
   printk(KERN_INFO "Kernel: registered - major number %d\n", majorNumber);

	//class
   gfClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(gfClass)){
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register gf device class\n");
      return PTR_ERR(gfClass);      
   }
   printk(KERN_INFO "gf device class registered correctly\n");

	//device
   gfDevice = device_create(gfClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(gfDevice)){ 
      class_destroy(gfClass);
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create gf device\n");
      return PTR_ERR(gfDevice);
   }
   printk(KERN_INFO "Kernel: gf device class created correctly\n");
   return 0;
}

static void __exit gf_exit(void){
   device_destroy(gfClass, MKDEV(majorNumber, 0));
   class_unregister(gfClass);
   class_destroy(gfClass);
   unregister_chrdev(majorNumber, DEVICE_NAME); 
   printk(KERN_INFO "Kernel: gfclass exit\n");
}


static int dev_open(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "Kernel: device connect successful \n");
   return 0;
}


static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "Kernel: device disconnect successful\n");
   return 0;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	asm volatile ("wbinvd");
	return 0;
}

module_init(gf_init);
module_exit(gf_exit);
