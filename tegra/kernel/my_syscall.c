/* my_syscall.c - Our own syscall */

#include <linux/linkage.h>

asmlinkage int sys_mycall(int num1,int num2) {
 printk("My first system call\n");
 return (num1 + num2);
 }
