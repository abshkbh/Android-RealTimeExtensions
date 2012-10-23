/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

extern enum hrtimer_restart timer_callback(struct hrtimer * timer);

asmlinkage int sys_setProcessBudget(pid_t pid, struct timespec budget) {

    struct task_struct * curr;


    read_lock(&tasklist_lock);
    curr = (struct task_struct *) find_task_by_vpid(pid);
    read_unlock(&tasklist_lock);

    if(curr == NULL){
	printk("Couldn't find task\n");
	return -ESRCH;
    }


    //Initializing timer , setting callback and setting expiry period to Budget - Compute 
      hrtimer_init(&(curr->budget_timer),CLOCK_MONOTONIC,HRTIMER_MODE_REL);
      (curr->budget_timer).function = &timer_callback;

    (curr->budget_time).tv_sec = budget.tv_sec;
    (curr->budget_time).tv_nsec = budget.tv_nsec;

    printk("Budget for task %d set to time = %ld secs  %ld nsecs \n",pid,(curr->budget_time).tv_sec,(curr->budget_time).tv_nsec);

    return 0;
}
