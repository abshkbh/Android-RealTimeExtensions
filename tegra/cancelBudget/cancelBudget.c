/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <linux/cpufreq.h>

void del_periodic_task(struct list_head * entry);
void del_task_cpu(struct list_head * entry, int cpu);
long sysclock(unsigned long max_frequency, struct task_ct_struct * list);
unsigned int apply_sysclock(unsigned long frequency, unsigned long max_frequency);
int set_rt_priorities(void);
void make_task_ct_struct(struct task_ct_struct ** list);
void pmclock(struct task_ct_struct * list);
int set_rt_priorities_per_cpu(int cpu);

extern int power_scheme;
extern int is_bin_packing_set;

asmlinkage int sys_cancelBudget(pid_t pid) {

    struct task_struct * curr;
    struct task_struct * temp;
    struct cpufreq_policy * lastcpupolicy = cpufreq_cpu_get(0);
    unsigned int max_frequency = (lastcpupolicy->cpuinfo).max_freq / 1000 ; //Getting MAX frequency in MHz
    unsigned int ret_freq, temp_freq;
    unsigned long sysclock_freq = 0;
    int ret_val;
    struct sched_param rt_sched_parameters;
    struct task_ct_struct * list;

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
    memset(&rt_sched_parameters, '0', sizeof(struct sched_param));
    do {
	temp->is_budget_set = 0;
	rt_sched_parameters.sched_priority = 0;
	printk(" DEBUG : Policy before cancel budget is %d\n" , temp->policy);
	ret_val = sched_setscheduler_nocheck(temp, SCHED_NORMAL, &rt_sched_parameters);
	
	printk(" DEBUG : Policy after cancel budget is %d\n" , temp->policy);
	printk("Retval = %d\n",ret_val);
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

    if(is_bin_packing_set == 1){
	del_task_cpu(&(curr->per_cpu_task), curr->cpu_no);
	set_rt_priorities_per_cpu(curr->cpu_no);
    }

    //Waking up the process in case if it was sleeping
    temp = curr;
    do {
	if(!wake_up_process(temp)){
	    printk("Process already running %d\n",temp->pid);
	}
    }while_each_thread(curr,temp);

    printk("Budget cancelled\n");

    //Setting the new frequency for the system

    //Setting the rt proirities
    if(is_bin_packing_set == 0){
	if (set_rt_priorities() < 0) {
	    printk("Error setting rt priorities\n");
	}
    }

    if(is_bin_packing_set == 0 ){
	if(power_scheme == 1){
	    //Getting the sys clock frequency
	    make_task_ct_struct(&list);
	    sysclock_freq = sysclock(max_frequency, list);
	    printk("Sysclock frequency is %lu kHz\n", sysclock_freq);
	} else if(power_scheme == 2){
	    //Getting the sys clock frequency
	    make_task_ct_struct(&list);
	    sysclock_freq = sysclock(max_frequency, list);
	    printk("Sysclock frequency is %lu kHz\n", sysclock_freq);
	    pmclock(list);
	    kfree(list);
	}

	if((ret_freq = apply_sysclock(sysclock_freq, (lastcpupolicy->cpuinfo).max_freq)) < 0){
	    printk("Failed to set the cpu feq after sysclock calculations \n");
	}
    }
    write_unlock(&tasklist_lock);

    if(power_scheme == 1 && is_bin_packing_set == 0){
	temp_freq = cpufreq_get(0);
	printk("Current cpu freq before setting %d \n",temp_freq);
	if((ret_val = cpufreq_driver_target(lastcpupolicy,ret_freq,CPUFREQ_RELATION_L))<0){
	    printk("Error :Processor frequency wasn't set\n"); 
	}
	temp_freq = cpufreq_get(0);
	printk("Cpu freq after setting %d min freq is %d\n",temp_freq,lastcpupolicy->min);
	kfree(list);
    }

    return 0;
}
