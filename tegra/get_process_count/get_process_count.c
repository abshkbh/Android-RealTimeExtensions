/* my_GetProcessCount.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>


#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h> /* For copy_to_user */

#include <linux/rcupdate.h>
#include <linux/sched.h> /* For task_struct */
#include <linux/slab.h>
#include<linux/spinlock.h> 

extern rwlock_t tasklist_lock;

asmlinkage int sys_get_process_count(void) {
	/* To keep count of tasks */
	int counter;	
	
        /* To iterate over task list in sched.c */
	struct task_struct *c ;   
	printk("My GPCount system call\n");
	
        counter = 0;	

	/* Lock list and iterate over processes */  
	read_lock(&tasklist_lock);
	for_each_process(c) {
		counter++;	
	}
	read_unlock(&tasklist_lock);
	/* Unlock list */

	return counter;

}

