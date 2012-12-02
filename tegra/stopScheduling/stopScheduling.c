/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

extern int is_bin_packing_set;
extern int periodic_tasks_size;
extern struct list_head periodic_task_head;

void clear_cpu_lists(void);

asmlinkage int sys_stopScheduling( void ) {

    struct list_head * temp;
    struct task_struct * curr;
    struct task_struct * temp2;
    
    printk("******STOPING SCHEDULING*******\n");
   
    is_bin_packing_set = 1;

    if(periodic_tasks_size == 0){
	printk("No RT Tasks to stop\n");
	return 0;
    }

    //This loop cancels the timers for all the rt tasks and resets the
    // compute time to zero . It also wakes up all the threads and now
    // all the tasks run as non-rt tasks .
    write_lock(&tasklist_lock);
    list_for_each(temp, &periodic_task_head){
	curr = container_of(temp, struct task_struct, periodic_task);

	//Cancelling the budget timer of all tasks that were running 
	if(hrtimer_try_to_cancel(&(curr->budget_timer)) == -1){ 
	    printk("Budget timer for %u in apply_sysclock()\n", curr->pid);
	}

	//Cancelling the period timer for all tasks
	if(hrtimer_try_to_cancel(&(curr->period_timer)) == -1){ 
	    printk("Period timer for %u in apply_sysclock()\n", curr->pid);
	}

	//setting the compute times to zero
	curr->compute_time.tv_sec = 0;
	curr->compute_time.tv_nsec = 0;

	//We should also wake-up all threads in a process
	temp2 = curr;
	do {
	    if(wake_up_process(temp2)){
		printk("Process was sleeping %d\n",temp2->pid);
	    }
	}while_each_thread(curr,temp2);

    }

    //Clearing all the cpu the list
    clear_cpu_lists();
    write_unlock(&tasklist_lock);

    printk("******EXITING STOP*******\n");

    return 0;
}
