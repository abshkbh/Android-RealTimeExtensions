/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>

extern int bin_packing_scheme;
extern int power_scheme;
int bin_packing(int scheme);
void clear_cpu_lists( void );

asmlinkage int sys_setBinPacking(int scheme) {

    int ret_val;
    
    printk("******START BINPACKING*******\n");

    if ((scheme < 0) || (scheme > 4)) {
	printk("ERR : Invalid Bin Packing Scheme\n");
	return -EINVAL;
    }

    bin_packing_scheme = scheme;
    printk("Bin Packing Scheme set to : %d\n",bin_packing_scheme);
    power_scheme = 0;
    if((ret_val = bin_packing(scheme)) == 0){
	printk("DEBUG: Bin Packing Failed\n");
	clear_cpu_lists();
	printk("DEBUG: CPU Lists Cleared\n");
	return -EINVAL;
    }
    printk("******EXIT BINPACKING*******\n");
    return ret_val;
}


