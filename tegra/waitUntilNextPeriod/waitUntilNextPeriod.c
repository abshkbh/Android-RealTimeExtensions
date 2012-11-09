/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

asmlinkage int sys_waitUntilNextPeriod(pid_t pid) {

    struct task_struct * curr;
    struct task_struct * temp;

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

    //If the budget is not set then just returning it.
    if((curr->budget_time).tv_sec == -1 && (curr->budget_time).tv_nsec == -1){
	printk("No budget set to wait\n");
	return -EINVAL;
    }

    //Setting compute time to Budget time just 
    //for safety in case it ever comes in run queue
    //before time period , it will automatically be put
    //to sleep
    (curr->compute_time).tv_sec = (curr->budget_time).tv_sec;
    (curr->compute_time).tv_nsec = (curr->budget_time).tv_nsec;

    temp = curr;

    //Putting all the tasks to sleep
    do{
	set_task_state(temp, TASK_UNINTERRUPTIBLE);
	set_tsk_need_resched(temp);
    }while_each_thread(curr, temp);

    schedule(); //BIG DEBATE ??

    printk("Wait until next period %d\n",pid);

    return 0;
}
