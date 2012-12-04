/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <linux/backlight.h>
#include <linux/cpufreq.h>

extern int is_bin_packing_set ;
extern int global_sysclock ;
extern struct list_head per_cpu_list[];
extern int per_cpu_list_size[];
void clear_cpu_list(void);
int set_rt_priorities_per_cpu(int cpu);
void make_task_ct_struct_per_cpu(struct task_ct_struct ** list,int cpu);
long sysclock_per_cpu(unsigned long max_frequency, struct task_ct_struct * list, int size);
unsigned int apply_sysclock(unsigned long frequency, unsigned long max_frequency);

asmlinkage int sys_startScheduling( void ) {

    int i;
    struct list_head * temp;
    struct task_struct * temp2;
    struct task_struct * curr;
    ktime_t p;
    unsigned long sysclock_freq = 0;
    struct task_ct_struct * list;
    unsigned long max_sysclock_freq = 0;
    struct cpufreq_policy * lastcpupolicy = cpufreq_cpu_get(0);
    unsigned long max_frequency = (lastcpupolicy->cpuinfo).max_freq / 1000 ; //Getting MAX frequency in MHz
    unsigned int ret_freq,temp_freq;
    int ret_val;

    printk("******START SCHEDULING*******\n");

    write_lock(&tasklist_lock);

    if (global_sysclock == 1) {
	//Iterating over all cpu lists and finding the MAX sysclock frequency
	//across all cores . Then we set that across all tasks 
	for(i = 0 ; i < 4 ; i++){
	    if(per_cpu_list_size[i] > 0) {

		//Setting rt-priorites and budget flag per cpu
		list_for_each(temp, &(per_cpu_list[i])){
		    curr = container_of(temp, struct task_struct, per_cpu_task);
		    temp2 = curr;
		    //setting the is_budget set flag for each child thread of the process
		    do {
			temp2->is_budget_set = 1;
		    }while_each_thread(curr,temp2);

		}

		set_rt_priorities_per_cpu(i);  //TODO : Why is this commented
		//Getting max sysclock freq
		make_task_ct_struct_per_cpu(&list,i);
		sysclock_freq = sysclock_per_cpu(max_frequency, list, per_cpu_list_size[i]);
		if (sysclock_freq > max_sysclock_freq) {
		    max_sysclock_freq = sysclock_freq;
		}
		kfree(list);
	    }
	}

	printk("Global Sysclock freq = %lu\n",max_sysclock_freq);
	if((ret_freq = apply_sysclock(max_sysclock_freq, (lastcpupolicy->cpuinfo).max_freq)) < 0){
	    printk("Failed to set the cpu feq after sysclock calculations \n");
	}

	write_unlock(&tasklist_lock);

	temp_freq = cpufreq_get(0);
	printk("Global Sysclock : Current cpu freq before setting %d \n",temp_freq);
	for(i = 0; i < 4; i++){
	    //Setting CPU freq
	    lastcpupolicy = cpufreq_cpu_get(i);
	    lastcpupolicy->max = 1300000;  //TODO : Should this be in for loop after line 81 ?
	    lastcpupolicy->min = 51000;
	    if((ret_val = cpufreq_driver_target(lastcpupolicy,ret_freq,CPUFREQ_RELATION_L))<0){
		printk("Error :Processor frequency wasn't set\n"); 
	    }
	}
	temp_freq = cpufreq_get(0);
	printk("Global Sysclock : Cpu freq after setting %d min freq is %d\n",temp_freq,lastcpupolicy->min);
    }
    else {
	//Iteratin over all cpu lists and starting their timers
	for(i = 0 ; i < 4 ; i++){
	    if(per_cpu_list_size[i] > 0){
		list_for_each(temp, &(per_cpu_list[i])){
		    curr = container_of(temp, struct task_struct, per_cpu_task);
		    temp2 = curr;
		    //setting the is_budget set flag for each child thread of the process
		    do {
			temp2->is_budget_set = 1;
		    }while_each_thread(curr,temp2);
		    p = timespec_to_ktime(curr->time_period);
		    if(hrtimer_start(&(curr->period_timer), p, HRTIMER_MODE_REL) == 1) {	
			printk("Could not restart period timer for task %d\n", curr->pid);
		    }
		}
		set_rt_priorities_per_cpu(i);
	    }
	}

	write_unlock(&tasklist_lock);
    }

    printk("****** EXITING START*******\n");

    return 0;
}
