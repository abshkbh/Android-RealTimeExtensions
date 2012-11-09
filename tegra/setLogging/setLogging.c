/* get_current_processes.c 
 * 
 * Our syscall which enables the logging.
 */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h> /* For task_struct */
#include <linux/time.h>
#include <linux/slab.h>

asmlinkage int sys_setLogging( pid_t log_pid, pid_t user_pid, int no_data_points ){

    struct task_struct * curr;
    int size;

    if(log_pid <= 0){
	printk("Logging pid is invalid\n");
	return -EINVAL;
    }
    
    if(user_pid <= 0){
	printk("User Process pid is invalid\n");
	return -EINVAL;
    }

    //TODO put access ok error check

    printk("In logging syscall for %d from %d\n", log_pid, user_pid);

    read_lock(&tasklist_lock);
    curr = (struct task_struct *) find_task_by_vpid(log_pid);
    read_unlock(&tasklist_lock);

    if(curr == NULL){
	printk("Couldn't find task\n");
	return -ESRCH;
    }

    if(curr->is_log_enabled == 1){
	printk("Logging already enabled for %d\n", curr->pid);
	return -EINVAL;
    }

    size = no_data_points * 2 * sizeof(struct timespec);

    //Setting up all the parameters in the task structure for logging
    write_lock(&tasklist_lock);
    curr->user_pid = user_pid;
    curr->buf = kmalloc(size, GFP_KERNEL);
    memset(curr->buf, '0', size);
    curr->is_log_enabled = 1;
    curr->buf_offset = 0;
    curr->no_data_points = no_data_points;
    write_unlock(&tasklist_lock);

    printk("Buffer : %s\n", curr->buf);

    printk("Exiting logging syscall\n");
    
    return 0;
}
