/*
 *
 *  k101 | zafiyetli LKM
 *  ###################################
 *  Bu root dosya sisteminde sağlanan zafiyetli
 *  modülün kaynak kodudur, derlemek için make
 *  aracını kullanabilirsiniz
 *
 */

#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>

#define DEV_NAME "vuln"
#define MAX_SIZE 32

MODULE_LICENSE("GPL");

static struct proc_dir_entry *proc_file;
static unsigned long procfs_buffer_size = 0;
static char procfs_buffer[MAX_SIZE];

static ssize_t procfile_read(
    struct file *file, char *buffer, size_t count, loff_t *offset){
  
  char local[MAX_SIZE];

  memcpy(local, procfs_buffer, MAX_SIZE);
  printk(KERN_INFO "[vuln]: Reading %d bytes\n", count);
  memcpy(buffer, local, count);

  return count;
}

static ssize_t procfile_write(
    struct file *file, const char *buffer, size_t count, loff_t *offset){
  
  char local[8];
  procfs_buffer_size = count;

  if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size))
    return -EFAULT;

  memcpy(local, procfs_buffer, procfs_buffer_size);
  printk(KERN_INFO "[vuln]: Copied to buffer: %s\n", local);

  return procfs_buffer_size;
}

static struct proc_ops fops = {
  .proc_read = procfile_read,
  .proc_write = procfile_write,
};

int init_module(){
  proc_file = proc_create(DEV_NAME, 0666, NULL, &fops);
  memset(procfs_buffer, 'A', MAX_SIZE);

  if (proc_file == NULL) {
    remove_proc_entry(DEV_NAME, NULL);
    printk(KERN_ALERT "[vuln] Cannot create /proc/%s\n", DEV_NAME);
    return -ENOMEM;
  }

  printk(KERN_INFO "[vuln] /proc/%s created\n", DEV_NAME);
  return 0;
}

void cant_get_here(void){
  printk(KERN_INFO "[vuln] How did we get here?\n");
}

void cleanup_module(){
  remove_proc_entry(DEV_NAME, NULL);
  printk(KERN_INFO "[vuln] /proc/%s removed\n", DEV_NAME);
}
