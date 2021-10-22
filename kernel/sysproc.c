#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
# include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// 新加的 kernel trace 函数
uint64
sys_trace(void)
{
  int mask;

  //观察上面的代码可以知道 a0 存放了第一个参数
  if (argint(0, &mask) < 0) // 读第一个参数（从内核的 a0 寄存器读）
    return -1;

  myproc()->trace_mask = mask; // 读完，放到 proc 里的 trace_mask，需要先声明在 proc.h
  return 0;
}

// 新加的 kernel sysinfo 函数
uint64
sys_sysinfo(void)
{
  uint64 addr; // 存放 sysinfo 用户态传进来的参数（一个 info 结构体的 64 位地址）

  if (argaddr(0, &addr) < 0)  // 从寄存器 a0，获取 64 位地址
    return -1;

  struct proc *p = myproc();
  struct sysinfo info;  // kernel 中也申请一个结构体 info，最后通过 copyout 送回用户态

  info.freemem = kfreemem_count();  // 调用自己写的计算 freemem 的函数，函数名写在 defs.h
  info.nproc = nproc_count();       // 调用自己写的计算 nproc   的函数，函数名写在 defs.h
  info.freefd = freefd_count();    // 调用自己写的计算 freefd   的函数，函数名写在 defs.h

  // Copy sysinfo back to user space
  if (copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0) 
    return -1;
  return 0;
}