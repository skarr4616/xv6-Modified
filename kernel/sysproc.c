#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
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
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
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

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
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

  argint(0, &pid);
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

// returns if trace mask is successfully set or not
uint64
sys_trace(void)
{
  argint(0, &(myproc()->tracemask));
  return 0;  
}

// sets the ticks and address of handler
uint64
sys_sigalarm(void)
{
  uint64 addr;
  int ticks;

  argint(0, &ticks);
  argaddr(1, &addr);

  myproc()->ticks = ticks;
  myproc()->handler = addr;

  return 0;
}

// restarts execution from where it was left and returns the last value stored at a0 register
uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  memmove(p->trapframe, p->alarm_tf, PGSIZE);

  kfree(p->alarm_tf);
  p->alarm_tf = 0;
  p->cur_ticks = 0;
  return p->trapframe->a0;
}

uint64 sys_set_priority(void)
{
  int new_static_p;
  int pid;

  argint(0, &new_static_p);
  argint(1, &pid);

  return set_priority(new_static_p, pid);
}

uint64
sys_settickets(void)
{
  argint(0, &(myproc()->tickets));
  return 0;  
}

uint64
sys_waitx(void)
{
  uint64 p, raddr, waddr;
  int rtime, wtime;

  argaddr(0, &p);
  argaddr(1, &raddr);
  argaddr(2, &waddr);

  int ret = waitx(p, &rtime, &wtime);
  
  struct proc *proc = myproc();
  if (copyout(proc->pagetable, raddr, (char*)&rtime , sizeof(int)) < 0)
    return -1;
  if (copyout(proc->pagetable, waddr, (char*)&wtime , sizeof(int)) < 0)
    return -1;
    
  return ret;
}