/* get_current_processes.c 
 * 
 * Our syscall which prints all
 * the current processes in the system.
 */

#include <linux/linkage.h>
#include <linux/kernel.h>

#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h> /* For copy_to_user */

#include <linux/rcupdate.h>
#include <linux/sched.h> /* For task_struct */
#include <linux/spinlock.h>
#include <linux/slab.h>

extern rwlock_t tasklist_lock;

#define BUF_LEN 80  /* Max length of message from device */

asmlinkage int sys_get_current_processes( char * buf, int n ){
	
	char temp[BUF_LEN];
	char *my_msg = NULL;

	/* To iterate over task list in sched.c */
	struct task_struct *curr ;        

	/* Offset used to manipulate write location kernel malloced buffer */
	int msg_offset = 0;

	/* Used to estimate total number of bytes required to be
	   printed out in user space. */
	int total_len = 0, length, return_buf_len;   
	
        /* Return with error in case of bad conditions */
        if( buf == NULL || n <= 0 ){
		return -1;
	}

        /* Check for malicious pointers */
	if (!access_ok(char * , buf , n)) {
		return -EFAULT;
	} 

	/* 1st run through list to estimate how big a buffer we need
	   to malloc to display all task info

	 * We also lock the list so that no new tasks are added */

	read_lock(&tasklist_lock);
	for_each_process(curr) {
		sprintf(temp, "Pid = [%d] Prio = [%d] Name = [%s]\n",curr->pid,curr->normal_prio,curr->comm);
		/* We don't count NULL ends in all strings 
		   as they will be co-catenated as one and sent
		   to userspace */
		total_len += strlen(temp);
	}

	/* Mallocing buffer in kernel */
	my_msg = kmalloc(total_len +1, GFP_KERNEL);

	/* 2nd run through list to show the tasks */
	for_each_process(curr) {

		sprintf(my_msg + msg_offset, "Pid = [%d] Prio = [%d] Name = [%s]\n",curr->pid,curr->normal_prio,curr->comm);
		msg_offset = strlen(my_msg);

	}
	read_unlock(&tasklist_lock);

	/* Checking for smaller buffer and in such a case giving incomplete output
	 */
	length = n < (total_len + 1) ? (n - 1) : (total_len + 1);
	my_msg[length] = '\0';

	/* Here we calculate strlength of final msg being sent to the user */
	return_buf_len = strlen(my_msg);

	if (copy_to_user(buf,my_msg,(return_buf_len + 1)) != 0) {
		kfree(my_msg);
		return -EFAULT;
	}

	kfree(my_msg);
	return (total_len - return_buf_len);
}
