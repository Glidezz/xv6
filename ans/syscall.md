# syscall

## System call tracing

首先在[user.h](../user/user.h)，和[user.pl](../user/usys.pl)，中添加函数原型。

然后在[syscall.h](../kernel/syscall.h)，[syscall.c](../kernel/syscall.c)中实现系统调用号，以及系统调用。

为正确打印系统调用的名称，这里选择使用数组的方式（类似保存系统调用的函数的方式）保存系统调用的名称，这样获得相关名称的方式与系统调用的方式即可保持一致。

```c
// syscall.h
#define SYS_trace  22
#define SYS_sysinfo 23
```

```c
//syscall.c
extern uint64 sys_close(void);
extern uint64 sys_trace(void);
extern uint64 sys_sysinfo(void);

static uint64 (*syscalls[])(void) = {
 //...
 [SYS_close]   sys_close,
 [SYS_trace]   sys_trace,
 [SYS_sysinfo] sys_sysinfo,
};

char *trace_name[] = {
[SYS_fork] "fork",
[SYS_exit] "exit",
[SYS_wait] "wait",
[SYS_pipe] "pipe",
[SYS_read] "read",
[SYS_kill] "kill",
[SYS_exec] "exec",
[SYS_fstat] "fstat",
[SYS_chdir] "chdir",
[SYS_dup] "dup",
[SYS_getpid] "getpid",
[SYS_sbrk] "sbrk",
[SYS_sleep] "sleep",
[SYS_uptime] "uptime",
[SYS_open] "open",
[SYS_write] "write",
[SYS_mknod] "mknod",
[SYS_unlink] "unlink",
[SYS_link] "link",
[SYS_mkdir] "mkdir",
[SYS_close] "close",
[SYS_trace] "trace",
[SYS_sysinfo] "sysinfo",
};
```

为实现打印系统调用的名称，需要在proc中增加系统调用的标记位
```c
struct proc {
  struct spinlock lock;
  //...
  char name[16]; // Process name (debugging)
  uint64 mark;                  // 标记trace系统调用的标记位
};
```

在初始化进程时，要初始化mask标记位，在父进程fork子进程时，要继承mask标记位。

```c
//proc.h
static struct proc*
allocproc(void)
{
    found:
   p->pid = allocpid();
   p->state = USED;
   p->mark = 0;
}
//...
int
fork(void)
{
    np->mark = p->mark;  // 拷贝trace标识位
    pid = np->pid;
}
```

在调用trace时，要将mask保存进当前进程的mask中。
```c
//sysproc.c
 uint64
 sys_trace(void) {
     uint64 mark;
     argaddr(0, &mark);

     struct proc *p = myproc();
     p->mark = mark;
     return 0;
 }
```

最后在syscall.c中实现打印系统调用。这里利用位运算的方式简单完成。

```c
//syscall.c
void
syscall(void)
{
    //...
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
     // Use num to lookup the system call function for num, call it,
     // and store its return value in p->trapframe->a0
     p->trapframe->a0 = syscalls[num]();
     if (1 << num & p->mark) {
             printf("%d: syscall %s -> %d\n", p->pid, trace_name[num], p->trapframe->a0);
     }
     //...
```

## Sysinfo

注册系统调用以及添加系统调用号过程与上面一致。

主要实现统计空闲页表的数量以及统计unused的进程数量，前者可参照freerange的实现，后者统计空闲进程可以通过遍历proc数组实现，

不要忘记在def.c中添加函数声明。

```c
//kalloc.c
 uint64 kmemory(void) {
     struct run *r;
     uint64 ans = 0;
     acquire(&kmem.lock);
     for (r = kmem.freelist; r != 0; r = r->next)
         ans += PGSIZE;
     release(&kmem.lock);
     return ans;
 }
```
```c
//proc.h
 uint64 kproc(void) {
     uint64 ans = 0;
     struct proc *p;
     for (p = proc; p < &proc[NPROC]; p++) {
         acquire(&p->lock);
         if (p->state != UNUSED)
             ans++;
         release(&p->lock);
     }
     return ans;
 } 
```

最后利用copyout将得到的结果从内核态拷贝到用户态。

```c
//sysproc.c
 uint64
 sys_sysinfo(void) {
     uint64 addr;
     argaddr(0, &addr);

     struct sysinfo info;
     info.freemem = kmemory();
     info.nproc = kproc();
     struct proc *p = myproc();
     if (copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
         return -1;

     return 0;
 } 
```
