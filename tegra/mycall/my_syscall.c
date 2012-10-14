/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

asmlinkage int sys_mycall(int num1,int num2, long long * sum) {
	
        
	long long local_sum = 0;

        printk("My first system call\n");

	/* Check for sum == null */
	if(sum == NULL){
		return -1;
	}

	/* Check for bad address */
	if (!access_ok(long long *,sum,sizeof(long long))) {
		return -EFAULT;
	}

	local_sum = (long long)num1 + (long long)num2;

	if (copy_to_user(sum , &local_sum ,sizeof(long long)) != 0) {
		return -1;
         }

	//    (*sum) = num1 + num2; Example of Bad Code

	return 0;
}
