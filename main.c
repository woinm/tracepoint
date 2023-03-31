#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched.h>

#include <linux/syscalls.h>

#include <linux/binfmts.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "tracepoint.h"
//

///////////////////////////////////////////////////////////////////////
///
/// @brief main.c EDR驱动，启动入口                                   ///
///
///////////////////////////////////////////////////////////////////////

int __tracepoint_enter(struct pt_regs *regs, long id) {
  printk("%s[%d] => enter: %d\n", current->comm, current->tgid, id);
  return 0;
}

int __tracepoint_exit(struct pt_regs *regs, long ret) {
  printk("%s[%d] => exit: %d\n", current->comm, current->tgid, ret);
  return 0;
}

struct tracepoint_syscall execve_probe = {.id = __NR_mmap,
                                          .name = "execve",
                                          .enter = __tracepoint_enter,
                                          .exit = __tracepoint_exit};

static int __init insmod_nerv(void) {
  int ret = 0;
  ret = tracepoint_init();
  if (0 != ret) {
    printk("tracepoint init failed\n");
    return ret;
  }
  printk("tracepoint init success\n");
  register_syscall_tracepoint(&execve_probe);
  return 0;
}

static void __exit rmmod_nerv(void) {
  printk("tracepoint-probe[%d] received rmmod request, begin to uninstall the "
         "module!!!\n",
         current->pid);
  unregister_syscall_tracepoint(&execve_probe);
  tracepoint_fini();
  printk("tracepoint-probe[%d] uninstall success!!!\n", current->pid);
}

module_init(insmod_nerv) module_exit(rmmod_nerv)

    MODULE_LICENSE("GPL");
// 声明模块的作者
// MODULE_AUTHOR ();
// 声明模块的描述
MODULE_DESCRIPTION(NERV_DESCRIPTION);
// 声明模块的版本
MODULE_VERSION(NERV_VERSION);
// 声明模块的设备表
// MODULE_DEVICE_TABLE("table_info");
// 别名
// MODULE_ALIAS("alternate_name");
