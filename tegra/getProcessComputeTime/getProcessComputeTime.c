/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/threads.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

asmlinkage int sys_getProcessComputeTime(pid_t pid) {

    struct task_struct * curr = NULL;

    /* If pid is negative or greater than max supported
       tasks then we return invalid argument */
    if ((pid < 0) || (pid >= PID_MAX_LIMIT)) {
<<<<<<< HEAD
	printk("PID not in bounds\n");
	return -EINVAL;
=======
	   return -EINVAL;
>>>>>>> a7b3eeb1f0a8d2ef597002b687a2f6a385e9ee55
    }

    /* Finding the taks given its pid */
    curr = (struct task_struct *)find_task_by_vpid(pid);

    /* If current task is not found then
       this Process does not exist */
    if(curr == NULL){
<<<<<<< HEAD
	printk("Did not find current task\n");
	return -ESRCH;
    }

    printk("The computation time of task %d is %ld secs %ld nsecs\n",
	    pid,(curr->compute_time).tv_sec, (curr->compute_time).tv_nsec);
=======
	    return -ESRCH;
    }

    printk("The computation time of task %d is %ld secs %ld nsecs\n",
	pid,(curr->exec_time).tv_sec, (curr->exec_time).tv_nsec);
>>>>>>> a7b3eeb1f0a8d2ef597002b687a2f6a385e9ee55

    return 0;
}
