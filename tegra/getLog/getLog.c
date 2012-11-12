/* get_current_processes.c 
 * 
 * Our syscall which enables the logging.
 */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h> /* For task_struct */
#include <linux/time.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

asmlinkage int sys_getLog( pid_t pid, char * buf ){

    struct task_struct * curr;
    int size;

    if(pid <= 0){
	printk("Invalid pid\n");
	return -EINVAL;
    }

    //check access ok

    read_lock(&tasklist_lock);
    curr = find_task_by_vpid(pid);
    read_unlock(&tasklist_lock);

    if(curr == NULL){
	printk("Couldn't find task\n");
	return -ESRCH;
    }

    size = curr->buf_offset * sizeof(struct timespec);

    printk("In get log, size is %d\n", size);

    if(copy_to_user(buf, curr->buf, size) != 0){
	kfree(curr->buf);
	return -EFAULT;
    }

    kfree(curr->buf);
    return size;

    return 0;
}
