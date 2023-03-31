#ifndef TRACEPIONT_H
#define TRACEPIONT_H

#define DEF_TRACEPOINT_NAME_MAX 64
struct pt_regs;
typedef int (*Syscallback)(struct pt_regs *, long);

/**
 * @brief The tracepoint_syscall struct 系统调用syscall hook信息
 */
struct tracepoint_syscall
{
    unsigned long id;
    char          name[DEF_TRACEPOINT_NAME_MAX];
    Syscallback   enter;
    Syscallback   exit;
};

/**
 * @brief register_syscall_tracepoiont 通过NR ID注册syscall回调
 * @param info
 * @return
 */
int register_syscall_tracepoint(struct tracepoint_syscall *info);
int register_syscall_tracepoints(long len, struct tracepoint_syscall *info[]);

/**
 * @brief register_syscall_tracepoiont 通过NR ID注销syscall回调
 * @param info
 * @return
 */
void unregister_syscall_tracepoint(struct tracepoint_syscall *info);
void unregister_syscall_tracepoints(long len, struct tracepoint_syscall *info[]);

/**
 * @brief tracepoint_init 初始化tracepoint hook列表(sys_enter/sys_exit)
 * @brief tracepoint_fini 注销tracepoint hook列表
 * @return
 */
int tracepoint_init(void);
void tracepoint_fini(void);

#endif // TRACEPIONT_H
