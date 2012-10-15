/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

asmlinkage long sys_setProcessBudget(pid_t pid, struct timespec budget) {

	struct task_struct * curr;
	
	if (!access_ok(struct timespec *,&budget,sizeof(struct timespec))) {
		return -EFAULT;
	}
	
	curr = (struct task_struct *) find_task_by_vpid(pid);
	
	if(curr == NULL){
		return -ESRCH;
	}
	(curr->budget_time).tv_sec = budget.tv_sec;
	(curr->budget_time).tv_nsec = budget.tv_nsec;

	printk("Budget for task %d set\n ",pid);
	
	return 0;
}
