/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

void del_periodic_task(struct list_head * entry);

asmlinkage int sys_cancelBudget(pid_t pid) {

    struct task_struct * curr;
    struct task_struct * temp;

    //Error checks for input arguments
    if (pid <= 0) {
	printk("Invalid PID\n");
	return -EINVAL;
    }

    //Finding task struct given its pid
    write_lock(&tasklist_lock);

    curr = (struct task_struct *) find_task_by_vpid(pid);
    if(curr == NULL){
	printk("Couldn't find task\n");
	write_unlock(&tasklist_lock);
	return -ESRCH;
    }

    temp = curr;
    do {
	temp->is_budget_set = 0;
	printk("Cancelled Budget for %d\n",temp->pid);
    }while_each_thread(curr,temp);

    //First check if a period timer already exists from a previous edition of this syscall.
    //If yes then we cancel it.
    // If this syscall returns 0 or 1 then timer is succesfully cancelled  
    if (((curr -> time_period).tv_sec > 0) || ((curr -> time_period).tv_nsec > 0)) {
	hrtimer_cancel(&(curr->period_timer));
    }

    (curr->time_period).tv_sec = -1;
    (curr->time_period).tv_nsec = -1;


    //First check if a budget timer already exists from a previous edition of this syscall.
    //If yes then we cancel it.
    // If this syscall returns 0 or 1 then timer is succesfully cancelled  
    if (((curr -> budget_time).tv_sec > 0) || ((curr -> budget_time).tv_nsec > 0)) {
	hrtimer_cancel(&(curr->budget_timer));
    }

    (curr->budget_time).tv_sec = -1;
    (curr->budget_time).tv_nsec = -1;

    //Removing the periodic task from the list
    del_periodic_task(&(curr->periodic_task));

    //Waking up the process in case if it was sleeping
    temp = curr;
    do {
	if(!wake_up_process(temp)){
	    printk("Process already running %d\n",temp->pid);
	}
    }while_each_thread(curr,temp);

    printk("Budget cancelled\n");

    write_unlock(&tasklist_lock);

    return 0;
}
