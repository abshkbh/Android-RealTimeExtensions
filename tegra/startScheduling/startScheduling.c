/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <linux/backlight.h>

extern int is_bin_packing_set ;
extern struct list_head per_cpu_list[4];
void clear_cpu_list(void);

asmlinkage int sys_startScheduling() {

    int i;
    struct list_head temp;
    struct task_struct * temp2;
    struct task_struct * curr;

    printk("******START SCHEDULING*******\n",value);


    //Iteratin over all cpu lists and starting their timers
    for(i = 0 ; i < 4 ; i++){

	list_for_each(temp, &(per_cpu_list[i])){
	    curr = container_of(temp, struct task_struct, periodic_task);

	    p = timespec_to_ktime(curr->time_period);
	    if(hrtimer_start(&(curr->period_timer), p, HRTIMER_MODE_REL) == 1) {	
		printk("Could not restart period timer for task %d\n", curr->pid);
	    }
	    temp2 = curr;

	    //setting the is_budget set flag for each child thread of the process
	    do {
		temp2->is_budget = 1;

	    }while_each_thread(curr,temp2);
	}
    }


    return 0;
}
