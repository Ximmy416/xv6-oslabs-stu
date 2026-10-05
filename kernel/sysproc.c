#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64 sys_exit(void) {
  int n;
  if (argint(0, &n) < 0) return -1;
  exit(n);
  return 0;  // not reached
}

uint64 sys_getpid(void) { return myproc()->pid; }

uint64 sys_fork(void) { return fork(); }

uint64 sys_wait(void) {
  uint64 p;
  int f;
  if (argaddr(0, &p) < 0) return -1;
  if (argint(1, &f) < 0) return -1;
  return wait(p,f);
}

uint64 sys_sbrk(void) {
  int addr;
  int n;

  if (argint(0, &n) < 0) return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0) return -1;
  return addr;
}

uint64 sys_sleep(void) {
  int n;
  uint ticks0;

  if (argint(0, &n) < 0) return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (myproc()->killed) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64 sys_kill(void) {
  int pid;

  if (argint(0, &pid) < 0) return -1;
  return kill(pid);
}

void sys_yield(void) {
  return yield();
}

// return how many clock tick interrupts have occurred
// since start.
uint64 sys_uptime(void) {
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_rename(void) {
  char name[16];
  int len = argstr(0, name, MAXPATH);
  if (len < 0) {
    return -1;
  }
  struct proc *p = myproc();
  memmove(p->name, name, len);
  p->name[len] = '\0';
  return 0;
}

uint64 sys_seccomp_ctl(void)
{
  int op;
  uint64 arg;
  struct proc *p = myproc();
  if(argint(0, &op) < 0)
    return -1;
  // 第2个参数是64位，用argaddr读进uint64变量arg
  if(argaddr(1, &arg) < 0)
    return -1;
  if(op == 0){
    p->seccomp_mask = arg;
    return 0;
  }else if(op == 1){
    p->max_children = arg;
    return 0;
  }
  return -1;
}

uint64 sys_seccomp_getlog(void)
{
  uint64 buf;
  uint64 len_addr;
  struct proc *p = myproc();
  // 参数0：用户缓冲区地址，uint64
  if(argaddr(0, &buf) < 0) return -1;
  // 参数1：存放长度的用户地址，uint64
  if(argaddr(1, &len_addr) < 0) return -1;

  for(int i=0; i < p->audit_log_len; i++){
    if(copyout(p->pagetable, buf + i*sizeof(uint64), (char *)&p->audit_log[i], sizeof(uint64)) <0){
      return -1;
    }
  }
  if(copyout(p->pagetable, len_addr, (char *)&p->audit_log_len, sizeof(int)) <0){
    return -1;
  }
  return 0;
}