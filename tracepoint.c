#include "tracepoint.h"

#include <linux/version.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 20)
#include <linux/kobject.h>
#include <trace/sched.h>
#include "ppm_syscall.h"
#include <trace/syscall.h>
#else
#include <asm/syscall.h>
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37))
#include <asm/atomic.h>
#else
#include <linux/atomic.h>
#endif
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
#include <linux/sched.h>
#else
#include <linux/sched/signal.h>
#include <linux/sched/cputime.h>
#endif
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/tracepoint.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26))
#include <linux/file.h>
#else
#include <linux/fdtable.h>
#endif
#include <net/sock.h>
#include <asm/unistd.h>

#ifndef __NR_syscall_max
#define __NR_syscall_max (336)
#endif

#ifndef NR_syscalls
#define NR_syscalls (__NR_syscall_max + 1)
#endif

#ifdef CONFIG_MIPS
  #define SYSCALL_TABLE_ID0 __NR_Linux
#elif defined CONFIG_ARM
  #define SYSCALL_TABLE_ID0 __NR_SYSCALL_BASE
#elif defined CONFIG_X86 || defined CONFIG_SUPERH
  #define SYSCALL_TABLE_ID0 0
#elif defined CONFIG_PPC64
  #define SYSCALL_TABLE_ID0 0
#elif defined CONFIG_S390
  #define SYSCALL_TABLE_ID0 0
#else
  #define SYSCALL_TABLE_ID0 0
#endif

/**
 * @brief The tracepoints_table struct
 */
struct tracepoints_table
{
    char              *name;
    void              *callback;
    struct tracepoint *tp;
    ulong              init;
};

// 只支持内核版本大于2.6.20的系统
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 20)
#error Unsupported kernel version!
#endif

// 高低内核版本tracepoint注册接口兼容
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
    #define TRACEPOINT_PROBE_REGISTER(p1, p2) tracepoint_probe_register(p1, p2)
    #define TRACEPOINT_PROBE_UNREGISTER(p1, p2) tracepoint_probe_unregister(p1, p2)
    #define TRACEPOINT_PROBE(probe, args...) static void probe(args)
#else
    #define TRACEPOINT_PROBE_REGISTER(p1, p2) tracepoint_probe_register(p1, p2, NULL)
    #define TRACEPOINT_PROBE_UNREGISTER(p1, p2) tracepoint_probe_unregister(p1, p2, NULL)
    #define TRACEPOINT_PROBE(probe, args...) static void probe(void *__data, args)
#endif

// syscall 前置回调
TRACEPOINT_PROBE(syscall_enter_probe, struct pt_regs *regs, long id);
// syscall 后置回调
TRACEPOINT_PROBE(syscall_exit_probe, struct pt_regs *regs, long ret);

// 初始化tracepoints hook列表
static struct tracepoints_table g_tracepoints_tables[] =
{
    {.name = "sys_enter", .callback = (void *)syscall_enter_probe, .tp = NULL, .init = 0},
    {.name = "sys_exit", .callback = (void *)syscall_exit_probe, .tp = NULL, .init = 0},
};

// syscall回调列表
static struct tracepoint_syscall *g_syscall_tables[NR_syscalls];
static spinlock_t g_tracepoint_lock;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0))
/* compat tracepoint functions */
static void __visit_tracepoint(struct tracepoint *tp, void *priv)
{
    if (NULL == tp || NULL== priv)
    {
        return;
    }
    if (0 == strcmp(tp->name, ((struct tracepoints_table *)priv)->name))
    {
        ((struct tracepoints_table *)priv)->tp = tp;
        ((struct tracepoints_table *)priv)->init = 1;
    }
}
#endif

static int compat_register_trace(struct tracepoints_table *tp_info)
{
    if (NULL == tp_info)
    {
        return -1;
    }
    printk("tracepoint probe register %s\n", tp_info->name);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 15, 0))
    tp_info->init = 1;
    return TRACEPOINT_PROBE_REGISTER(tp_info->name, tp_info->callback);
#else
    for_each_kernel_tracepoint(__visit_tracepoint, tp_info);
    if (NULL == tp_info->tp)
    {
        return -1;
    }
    return tracepoint_probe_register(tp_info->tp, tp_info->callback, NULL);
#endif
    return -1;
}

static void compat_unregister_trace(struct tracepoints_table *tp_info)
{
    if (NULL == tp_info)
    {
        return;
    }
    printk("tracepoint probe unregister %s\n", tp_info->name);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 15, 0))
    TRACEPOINT_PROBE_UNREGISTER(tp_info->name, tp_info->callback);
#else
    tracepoint_probe_unregister(tp_info->tp, tp_info->callback, NULL);
#endif
    tp_info->tp = NULL;
    tp_info->init = 0;
}

int register_syscall_tracepoint(struct tracepoint_syscall *info)
{
    if (NULL == info || info->id < 0 || info->id >= NR_syscalls)
    {
        return -1;
    }

    spin_lock(&(g_tracepoint_lock));
    g_syscall_tables[info->id] = info;
    spin_unlock(&(g_tracepoint_lock));
    return 0;
}

int register_syscall_tracepoints(long len, struct tracepoint_syscall *infos[])
{
    int ret;
    long index = 0;
    for(; index < len; ++index)
    {
        ret = register_syscall_tracepoint(infos[index]);
        if (0 != ret)
        {
            unregister_syscall_tracepoints(index, infos);
            return ret;
        }
    }
    return 0;
}

void unregister_syscall_tracepoint(struct tracepoint_syscall *info)
{
    if (NULL == info || info->id < 0 || info->id >= NR_syscalls)
    {
        return;
    }
    spin_lock(&(g_tracepoint_lock));
    g_syscall_tables[info->id] = NULL;
    spin_unlock(&(g_tracepoint_lock));
}

void unregister_syscall_tracepoints(long len, struct tracepoint_syscall *infos[])
{
    if (NULL == infos)
    {
        return ;
    }

    long index = 0;
    for(; index < len; ++index)
    {
        unregister_syscall_tracepoint(infos[index]);
    }
}

TRACEPOINT_PROBE(syscall_enter_probe, struct pt_regs *regs, long id)
{
    if (id < 0 || id >= NR_syscalls)
    {
        return;
    }
    Syscallback callback = NULL;
    spin_lock(&(g_tracepoint_lock));
    struct tracepoint_syscall *info = g_syscall_tables[id];
    if (NULL != info)
    {
        callback = info->enter;
    }
    spin_unlock(&(g_tracepoint_lock));

    if (NULL != callback)
    {
        callback(regs, id);
    }
}

TRACEPOINT_PROBE(syscall_exit_probe, struct pt_regs *regs, long ret)
{
    long index;
    long id = syscall_get_nr(current, regs);
    index = id - SYSCALL_TABLE_ID0;

    if (id < 0 || id >= NR_syscalls)
    {
        return;
    }

    Syscallback callback = NULL;
    spin_lock(&(g_tracepoint_lock));
    struct tracepoint_syscall *info = g_syscall_tables[id];
    if (NULL != info)
    {
        callback = info->exit;
    }
    spin_unlock(&(g_tracepoint_lock));

    if (NULL != callback)
    {
        callback(regs, ret);
    }
}

/**
 * @brief __clear_tracepoints 清理注销tracepoints hook列表
 */
void __clear_tracepoints(void)
{
    ulong i;
    for (i = 0; i < sizeof(g_tracepoints_tables) / sizeof(struct tracepoints_table); ++i)
    {
        if (0 != g_tracepoints_tables[i].init)
        {
            compat_unregister_trace(&g_tracepoints_tables[i]);
        }
    }
}

int tracepoint_init(void)
{
    int ret = -1;

    ulong i = 0;
    for(; i < NR_syscalls; ++i)
    {
        g_syscall_tables[i] = NULL;
    }
    spin_lock_init(&g_tracepoint_lock);

    for (i = 0; i < sizeof(g_tracepoints_tables) / sizeof(struct tracepoints_table); ++i)
    {
        ret = compat_register_trace(&g_tracepoints_tables[i]);
        if (0 != ret)
        {
            pr_err("can't create the %s tracepoint\n", g_tracepoints_tables[i].name);
            __clear_tracepoints();
            return ret;
        }
    }
    return ret;
}

void tracepoint_fini(void)
{
    //printk("clear syscalltables.\n");
    // 清理注册的syscall回调
    int i = 0;
    spin_lock(&(g_tracepoint_lock));
    for(; i < NR_syscalls; ++i)
    {
        g_syscall_tables[i] = NULL;
    }
    spin_unlock(&(g_tracepoint_lock));

    //printk("clear tracepoints.\n");
    // 清理注销tracepoints hook列表
    __clear_tracepoints();
}
