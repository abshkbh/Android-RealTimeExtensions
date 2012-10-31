/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

asmlinkage int sys_waitUntilNextPeriod(pid_t pid) {

    struct task_struct * curr;
   
    if (pid <= 0) {
	printk("Invalid PID\n");
	return -EINVAL;
    }

    //Finding task struct given its pid
    read_lock(&tasklist_lock);
    curr = (struct task_struct *) find_task_by_vpid(pid);
    read_unlock(&tasklist_lock);
    if(curr == NULL){
	printk("Couldn't find task\n");
	return -ESRCH;
    }

    //Putting task to sleep
    //curr->state = TASK_INTERRUPTIBLE;
    set_task_state(curr, TASK_UNINTERRUPTIBLE);
    set_tsk_need_resched(curr);
    schedule();
    
    //Setting compute time to Budget time just 
    //for safety in case it ever comes in run queue
    //before time period , it will automatically be put
    //to sleep
    (curr->compute_time).tv_sec = (curr->budget_time).tv_sec;
    (curr->compute_time).tv_nsec = (curr->budget_time).tv_nsec;

    printk("Wait until next period %d\n",pid);

    return 0;
}
