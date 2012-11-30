#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/cpufreq.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>

#define SCALE 1000

enum hrtimer_restart budget_timer_callback(struct hrtimer * timer);
enum hrtimer_restart period_timer_callback(struct hrtimer * timer);
int check_admission(struct timespec budget, struct timespec period);
void add_periodic_task(struct list_head * new);
void del_periodic_task(struct list_head * entry);
long sysclock(unsigned long max_frequency, struct task_ct_struct * list);
unsigned int apply_sysclock(unsigned long frequency, unsigned long max_frequency);
int set_rt_priorities(void);
void make_task_ct_struct(struct task_ct_struct ** list);
int apply_pmclock(unsigned long max_frequency);
void pmclock(struct task_ct_struct * list);

extern int power_scheme;

asmlinkage int sys_setProcessBudget(pid_t pid, unsigned long budget, struct timespec period) {

    struct task_struct * curr;    
    struct task_struct * temp;
    unsigned long temp_time;
    struct cpufreq_policy * lastcpupolicy = cpufreq_cpu_get(0);
    unsigned long max_frequency = (lastcpupolicy->cpuinfo).max_freq / 1000 ; //Getting MAX frequency in MHz
    unsigned long sysclock_freq = 0;
    struct timespec task_budget;
    unsigned int ret_freq = 0, temp_freq;
    int ret_val;
    struct task_ct_struct * list;

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
    temp_time = (budget / (max_frequency)) * 1000;
    task_budget = ns_to_timespec(temp_time);

    printk("Time in ns = %lu and frequency = %lu Mhz\n",temp_time,max_frequency);
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
	return -ESRCH;
    }

    if(curr->is_budget_set == 1){
	del_periodic_task(&(curr->periodic_task));
    }

    printk("DEBUG: Default policy is %d\n",curr->policy);

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
    (curr->min_budget_time).tv_sec = task_budget.tv_sec;
    (curr->min_budget_time).tv_nsec = task_budget.tv_nsec;

    temp = curr;
    do {
	//Setting flag
	temp->is_budget_set = 1;
    }while_each_thread(curr,temp);

    //Adding the task in our periodic linked list
    add_periodic_task(&(curr->periodic_task));

    //Setting the rt proirities 
    if (set_rt_priorities() < 0) {
	printk("Error setting rt priorities\n");
    }

    printk("DEBUG: Updated policy is %d\n",curr->policy);

    switch(power_scheme){
	case 0:
	    printk("SETPROCESSBUDGET: No PM Scheme selected\n");
	    break;
	case 1:
	    printk("SETPROCESSBUDGET: SysClock is selected\n");
	    //Getting the sys clock frequency
	    make_task_ct_struct(&list);
	    sysclock_freq = sysclock(max_frequency, list);
	    printk("Sysclock frequency is %lu KHz\n", sysclock_freq);
	    if((ret_freq = apply_sysclock(sysclock_freq, (lastcpupolicy->cpuinfo).max_freq)) < 0){
		printk("Failed to set the cpu feq after sysclock calculations \n");
	    } 
	    kfree(list);
	    break;
	case 2:
	    //Getting the PM clock frequency
	    printk("SETPROCESSBUDGET: PMClock is selected\n");
	    make_task_ct_struct(&list);
	    sysclock_freq = sysclock(max_frequency, list);
	    printk("Sysclock frequency is %lu KHz\n", sysclock_freq);
	    pmclock(list);
	    apply_pmclock((lastcpupolicy->cpuinfo).max_freq);
	    kfree(list);
	    break;
    }

    printk("User RT Prio for task %d is %d\n",pid,curr->rt_priority);

    //Unlocking spin lock
    write_unlock(&tasklist_lock);

    printk("leaving the task_list lock \n");

    //Setting the frequency only for SYSCLOCK
    if(power_scheme == 1){
	lastcpupolicy->max = 1300000;
	lastcpupolicy->min = 51000;
	temp_freq = cpufreq_get(0);
	printk("Current cpu freq before setting %d \n",temp_freq);
	if((ret_val = cpufreq_driver_target(lastcpupolicy,ret_freq,CPUFREQ_RELATION_L))<0){
	    printk("Error :Processor frequency wasn't set\n"); 
	}
	temp_freq = cpufreq_get(0);
	printk("Cpu freq after setting %d min freq is %d\n",temp_freq,lastcpupolicy->min);
    }

    printk("Returning from setProcessBudget \n");
    return 0;
}
