/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#define CPU 0

enum hrtimer_restart budget_timer_callback(struct hrtimer * timer);
enum hrtimer_restart period_timer_callback(struct hrtimer * timer);


asmlinkage int sys_setProcessBudget(pid_t pid, struct timespec budget, struct timespec period, int rt_prio) {

    struct task_struct *curr;
    struct task_struct *temp;
    ktime_t p;
    int retval;
    unsigned long flags;
    struct sched_param rt_sched_parameters;
    struct cpumask cpuset;
    cpumask_clear(&cpuset);
    cpumask_set_cpu(CPU, &cpuset);

    //Error checks for input arguments
    if (!((budget.tv_sec > 0) || (budget.tv_nsec > 0))) {
	printk("Invalid budget time\n");
	return -EINVAL;
    }

    if (!((period.tv_sec > 0) || (period.tv_nsec > 0))) {
	printk("Invalid time period\n");
	return -EINVAL;
    }

    if (pid <= 0) {
	printk("Invalid PID\n");
	return -EINVAL;
    }

    if ((rt_prio < 0) || (rt_prio >= MAX_RT_PRIO)){
	printk("RT Prio not in range\n");
	return -EINVAL;
    }


    if(timespec_compare(&budget, &period) != -1){
	printk("Budget >= Period\n");
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

    //   spin_lock_irqsave(&(curr->tasklock),flags);
   // write_lock(&tasklist_lock);

    //First check if a period timer already exists from a previous edition of this syscall.
    //If yes then we cancel it.
    //If this syscall returns 0 or 1 then timer is succesfully cancelled  
    if (((curr -> time_period).tv_sec > 0) || ((curr -> time_period).tv_nsec > 0)) {
	hrtimer_cancel(&(curr->period_timer));
    }
    //If timer is being initialized the first time then
    //Initialize timer , set the callback and set  expiry period to Budget 
    else {
	printk("Initing period timer\n");
	hrtimer_init(&(curr->period_timer),CLOCK_MONOTONIC,HRTIMER_MODE_REL);
	(curr->period_timer).function = &period_timer_callback;
    }
    
    write_lock(&tasklist_lock);
    
    (curr->time_period).tv_sec = period.tv_sec;
    (curr->time_period).tv_nsec = period.tv_nsec;

    //    (curr->budget_time).tv_sec = budget.tv_sec;
    //    (curr->budget_time).tv_nsec = budget.tv_nsec;

    //Setting user_rt_prio to priority given by user and
    //sched policy to SCHED_FIFO
    curr->rt_priority = rt_prio;
    rt_sched_parameters.sched_priority = rt_prio;
    sched_setscheduler(curr,SCHED_FIFO,&rt_sched_parameters);

    write_unlock(&tasklist_lock);
    
    //Traversing over threads in the thread group and setting the values of
    //C, T as well as initializing the budget timer.
    temp = curr;
    do {
	printk("TID :%d TGID = %d\n",temp->pid,temp->tgid);

	//Initializing the budget timer
	if (((temp -> budget_time).tv_sec > 0) || ((temp -> budget_time).tv_nsec > 0)) {
	    hrtimer_cancel(&(temp->budget_timer));
	}
	else {
	    hrtimer_init(&(temp->budget_timer),CLOCK_MONOTONIC,HRTIMER_MODE_REL);
	    (temp->budget_timer).function = &budget_timer_callback;
	}
	write_lock(&tasklist_lock);

	//Setting the budget
	(temp->budget_time).tv_sec = budget.tv_sec;
	(temp->budget_time).tv_nsec = budget.tv_nsec;

	write_unlock(&tasklist_lock);
	//Setting affinity
	if((retval =sched_setaffinity(temp->pid, &cpuset)) < 0){
	    printk("Could not set the affinity for %d with err %d\n", temp->pid, retval);
	}

    }while_each_thread(curr,temp);

    //Start Timer
    p = timespec_to_ktime(period);
    if(hrtimer_start(&(curr->period_timer), p, HRTIMER_MODE_REL) == 1) {
	printk("Could not restart budget timer for task %d", pid);
    }

    printk("User RT Prio for task %d is %d\n",pid,curr->pid);
    //write_unlock(&tasklist_lock);
    //  spin_unlock_irqrestore(&(curr->tasklock),flags);
    return 0;
}

