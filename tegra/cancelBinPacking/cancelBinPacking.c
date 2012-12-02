/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

extern int is_bin_packing_set;

asmlinkage int sys_cancelBinPacking( void ) {

    is_bin_packing_set = 0;
    return 0;
}


