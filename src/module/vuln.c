#include <linux/module.h>       
#include <linux/kernel.h>       
#include <linux/proc_fs.h>      
#include <linux/uaccess.h>        

#define PROCFS_MAX_SIZE         32
#define PROCFS_NAME             "vuln"

MODULE_LICENSE("GPL");

static struct proc_dir_entry *proc_file;
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;

static ssize_t procfile_read(struct file *file, char *buffer, size_t count, loff_t *offset){
  char local[PROCFS_MAX_SIZE];
  memcpy(local, procfs_buffer, PROCFS_MAX_SIZE);
  printk(KERN_INFO "[vuln] Reading %d bytes\n", count);
  memcpy(buffer, local, count);
  return count;
}

static ssize_t procfile_write(struct file *file, const char *buffer, size_t count, loff_t *offset){
  char local[8];
  procfs_buffer_size = count;

  if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size))
    return -EFAULT;

  memcpy(local, procfs_buffer, procfs_buffer_size);
  printk(KERN_INFO "[vuln] Copied to buffer: %s", local);
  return procfs_buffer_size;
}

static struct proc_ops fops = {
  .proc_read = procfile_read,
  .proc_write = procfile_write,
};

int init_module(){
  proc_file = proc_create(PROCFS_NAME, 0666, NULL, &fops);
  memset(procfs_buffer, 'A', PROCFS_MAX_SIZE);

  if (proc_file == NULL) {
    remove_proc_entry(PROCFS_NAME, NULL);
    printk(KERN_ALERT "[vuln] Cannot create /proc/%s\n", PROCFS_NAME);
    return -ENOMEM;
  }

  printk(KERN_INFO "[vuln] /proc/%s created\n", PROCFS_NAME);
  return 0;
}

void cant_get_here(void){
  printk(KERN_INFO "[vuln] How did we get here?\n");
}

void cleanup_module(){
  remove_proc_entry(PROCFS_NAME, NULL);
  printk(KERN_INFO "[vuln] /proc/%s removed\n", PROCFS_NAME);
}
