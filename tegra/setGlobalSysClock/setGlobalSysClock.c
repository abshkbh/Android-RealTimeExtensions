/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

extern int global_sysclock;
asmlinkage int sys_setGlobalSysClock( void ) {

    printk("********* CALLING SETGLOBALSYSCLOCK*********\n");
    global_sysclock = 1; 
    return 0;
}
