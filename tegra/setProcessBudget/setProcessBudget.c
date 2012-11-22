#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/cpufreq.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>

#define SCALE 1000000

enum hrtimer_restart budget_timer_callback(struct hrtimer * timer);
enum hrtimer_restart period_timer_callback(struct hrtimer * timer);
int check_admission(struct timespec budget, struct timespec period);
void add_periodic_task(struct list_head * new);
void del_periodic_task(struct list_head * entry);
long sysclock();

asmlinkage int sys_setProcessBudget(pid_t pid, unsigned long budget, struct timespec period) {

    struct task_struct * curr;    
    struct task_struct * temp;
    unsigned long temp_time;
    unsigned long frequency = cpufreq_quick_get(0) / 1000 ; //Getiing frequency in MHz
    unsigned long sysclock_freq;
    ktime_t p;
    struct timespec task_budget;
    struct sched_param rt_sched_parameters;

    //Error checks for input arguments
    if (!((period.tv_sec > 0) || (period.tv_nsec > 0))) {
	printk("Invalid time period\n");
	return -EINVAL;
    }

    if (pid <= 0) {
	printk("Invalid PID\n");
	return -EINVAL;
    }

    //checking admission
    write_lock(&tasklist_lock);

    //First get budget from cycles in (millions) / current CPU frequency in (Mhz) * 1000 ns
    temp_time = ((budget * SCALE) / (frequency)) * 1000;
    task_budget = ns_to_timespec(temp_time);

    printk("Time in ns = %lu and frequency = %lu Mhz\n",temp_time,frequency);
    printk("Taks struct budget %ldsec and %ldnsec\n",task_budget.tv_sec,task_budget.tv_nsec);
    
    if(timespec_compare(&task_budget, &period) >= 0){
    	printk("Budget >= Period\n");
	write_unlock(&tasklist_lock);
    	return -EINVAL;
    }

    if(check_admission(task_budget, period) == 0){
	 printk("Cant add task to the taskset\n");
	 write_unlock(&tasklist_lock);
	 return -EPERM;
    } 

    //Finding task struct given its pid
    curr = (struct task_struct *) find_task_by_vpid(pid);
    if(curr == NULL){
	printk("Couldn't find task\n");
	write_unlock(&tasklist_lock);
urn -ESRCH;
    }

    if(curr->is_budget_set == 1){
	del_periodic_task(&(curr->periodic_task));
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

    //Setting periods and budgets
    (curr->budget_time).tv_sec = task_budget.tv_sec;
    (curr->budget_time).tv_nsec = task_budget.tv_nsec;

    //Setting user_rt_prio to priority given by user and
    //sched policy to SCHED_FIFO
    temp = curr;
    do {
	//Setting flag
	temp->is_budget_set = 1;
	//temp->rt_priority = rt_prio;
	//rt_sched_parameters.sched_priority = rt_prio;
	//retval = sched_setscheduler(temp,SCHED_FIFO,&rt_sched_parameters);
	//printk("Retval = %d\n",retval);
	//set_tsk_need_resched(temp);
    }while_each_thread(curr,temp);


    //Adding the task in our periodic linked list
    add_periodic_task(&(curr->periodic_task));

    //Getting the sys clock frequency
    sysclock_freq = sysclock();
    printk("Sysclock frequency is %lu\n", sysclock_freq);

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
