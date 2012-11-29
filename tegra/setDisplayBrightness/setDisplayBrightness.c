/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <linux/backlight.h>

int set_brightness(int brightness);

asmlinkage int sys_setDisplayBrightness(int value) {

    if ((value < 10) || (value > 255)) {
	printk("ERR : Invalid Brightness Value\n");
	return -EINVAL;
    }

    set_brightness(value);

    printk("Calling setDisplayBrightness with value  %d\n",value);
    return 0;
}
