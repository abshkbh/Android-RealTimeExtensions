/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

asmlinkage int sys_setGlobalSysClock() {

    printk("********* CALLING SETGLOBALSYSCLOCK*********\n");
    return 0;
}
