/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>

enum hrtimer_restart budget_timer_callback(struct hrtimer * timer);
enum hrtimer_restart period_timer_callback(struct hrtimer * timer);
int check_admission(struct timespec budget, struct timespec period);
void add_periodic_task(struct list_head * new);

asmlinkage int sys_setProcessBudget(pid_t pid, struct timespec budget, struct timespec period, int rt_prio) {

    struct task_struct * curr;
    struct task_struct * temp;
    int retval;
    ktime_t p;
    struct sched_param rt_sched_parameters;

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


    if(timespec_compare(&budget, &period) >= 0){
	printk("Budget >= Period\n");
	return -EINVAL;
    }

    //TODO check locking out here
    //checking admission
    write_lock(&tasklist_lock);
    if(check_admission(budget, period) == 0){
	printk("Cant add task to the taskset\n");
	write_unlock(&tasklist_lock);
	return -EPERM;
    }

    //Finding task struct given its pid
    curr = (struct task_struct *) find_task_by_vpid(pid);
    if(curr == NULL){
	printk("Couldn't find task\n");
	write_unlock(&tasklist_lock);
	return -ESRCH;
    }


    //First check if a period timer already exists from a previous edition of this syscall.
    //If yes then we cancel it.
    //If this syscall returns 0 or 1 then timer is succesfully cancelled  
    if (((curr -> time_period).tv_sec > 0) || ((curr -> time_period).tv_nsec > 0)) {
	hrtimer_cancel(&(curr->period_timer));
    }
    //If timer is being initialized the first time then
    //Initialize timer , set the callback and set  expiry period to Budget 
    else {
	hrtimer_init(&(curr->period_timer),CLOCK_MONOTONIC,HRTIMER_MODE_REL);
	(curr->period_timer).function = &period_timer_callback;
    }

    //First check if a budget timer already exists from a previous edition of this syscall.
    //If yes then we cancel it.
    // If this syscall returns 0 or 1 then timer is succesfully cancelled  
    if (((curr -> budget_time).tv_sec > 0) || ((curr -> budget_time).tv_nsec > 0)) {
	hrtimer_cancel(&(curr->budget_timer));
    }
    //If timer is being initialized the first time then
    //Initialize timer , set the callback and set  expiry period to Budget 
    else {
	hrtimer_init(&(curr->budget_timer),CLOCK_MONOTONIC,HRTIMER_MODE_REL);
	(curr->budget_timer).function = &budget_timer_callback;
    }
    

    //Setting flag
    curr->is_budget_set = 1;

    //Setting periods and budgets
    (curr->time_period).tv_sec = period.tv_sec;
    (curr->time_period).tv_nsec = period.tv_nsec;
    
    (curr->budget_time).tv_sec = budget.tv_sec;
    (curr->budget_time).tv_nsec = budget.tv_nsec;


    //Setting user_rt_prio to priority given by user and
    //sched policy to SCHED_FIFO
    temp = curr;
    do {
	//Setting flag
	temp->is_budget_set = 1;
	temp->rt_priority = rt_prio;
	rt_sched_parameters.sched_priority = rt_prio;
	retval = sched_setscheduler(temp,SCHED_FIFO,&rt_sched_parameters);
	printk("Retval = %d\n",retval);
	set_tsk_need_resched(temp);
    }while_each_thread(curr,temp);


    //Adding the task in our periodic linked list
    add_periodic_task(&(curr->periodic_task));

    //Start Timer
    p = timespec_to_ktime(period);
    if(hrtimer_start(&(curr->period_timer), p, HRTIMER_MODE_REL) == 1) {	
	printk("Could not restart budget timer for task %d", pid);
    }

    printk("User RT Prio for task %d is %d\n",pid,curr->rt_priority);

    //Unlocking spin lock
    write_unlock(&tasklist_lock);

    return 0;
}
