/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

extern int power_scheme;

asmlinkage int sys_setPMScheme(int pm_scheme) {

    if ((pm_scheme < 0) || (pm_scheme > 2)) {
	printk("ERR : Invalid PM Scheme\n");
	return -EINVAL;
    }

    power_scheme = pm_scheme;
    printk("Power scheme set to : %d\n",power_scheme);
    return 0;
}


