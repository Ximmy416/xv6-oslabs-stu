# xv6-oslabs-stu

哈工大（深圳）2026 秋操作系统课程实验项目，基于 xv6-riscv 教学操作系统，在 RISC-V QEMU 模拟器上完成。

## 实验环境

- **操作系统**：WSL2 Ubuntu 24.04
- **模拟器**：QEMU（RISC-V 64）
- **编译器**：riscv64-unknown-elf-gcc
- **调试器**：riscv64-unknown-elf-gdb + gdb-dashboard

## 分支说明

| 分支 | 实验内容 | 状态 |
|------|---------|------|
| `util` | 实验一：XV6 与 Unix 实用程序（sleep、pingpong、find + 启动流程 GDB 调试） | ✅ 已完成 |
| `syscall` | 实验二：进程与系统调用（exit 退出信息、wait 非阻塞选项、yield 系统调用） | ✅ 已完成 |
| `lock` | 实验三：锁机制的应用（物理页分配器与 buffer cache 的锁竞争优化） | ✅ 已完成 |
| `pgtbl` | 实验四：页表（vmprint、每进程独立内核页表、用户映射并入内核页表） | ✅ 已完成 |

## 实验一（util）内容

### 用户程序

- **sleep**：接收一个 tick 参数，调用 sleep() 系统调用让进程休眠指定 tick 数
- **pingpong**：通过两条管道和 fork() 实现父子进程双向通信（ping/pong）
- **find**：递归遍历目录树，查找指定名称的文件或目录

### 启动流程调试

使用 GDB 跟踪 xv6 从 _entry 到第一个用户进程 init 的完整启动过程，通过断点观察进程名从 "initcode" 到 "init" 的切换，理解 exec() 系统调用不创建新进程、仅替换当前进程程序映像的原理。

## 实验二（syscall）内容

### 任务一：exit 打印父子进程信息

在 exit 的执行流程中，用课程提供的 exit_info 输出当前进程的父进程信息，以及每个子进程的编号、pid、名称和状态；据此可以观察父进程先退出后，其子进程被重新交给 init 进程托管的完整过程。测试程序为 exittest。

### 任务二：wait 增加非阻塞选项

把 wait 接口改为 wait(int *status, int flags)：flags 为 0 时保持原有阻塞等待；flags 为 1 时若当前没有可回收的僵尸子进程就不睡眠，直接返回 -1。内核态相应修改 wait 的查找与睡眠逻辑，用户态接口由课程提供。测试程序为 waittest。

### 任务三：实现 yield 系统调用

新增 yield 系统调用，让当前进程主动让出 CPU，而不是退出进程。同时在调度路径上打印三类信息：当前进程内核线程上下文的保存地址范围、当前运行进程的 pid 与其陷入内核前的用户态 pc、下一个可运行进程的 pid 与用户态 pc。因输出与调度顺序相关，测试时用 make qemu CPUS=1 启动单核环境，测试程序为 yieldtest。

## 实验三（lock）内容

### 物理页分配器

原实现由单一 kmem 锁保护一条全局空闲页链表，多核并发分配时锁竞争严重。改为每个 CPU 各维护一份空闲页链表与独立的锁，本 CPU 链表为空时再向其他 CPU 借用，把集中在一把锁上的竞争分散到多把锁上。验证指标为 kalloctest 中的 tot（要求 tot < 10）。

### buffer cache

原实现由单一 bcache.lock 保护整张缓存链表，查找、分配和释放都要抢同一把锁。改为按 blockno 哈希分桶，每个桶一条链表、一把锁：访问落在不同桶的块时互不干扰，把锁竞争分散到多把桶锁上，同时保持"同一磁盘块在缓存中只有一份副本"的约束。验证为 bcachetest 的 tot 指标与 usertests 全部通过。

## 实验四（pgtbl）内容

### 任务一：vmprint

实现 vmprint 与递归辅助函数，按三级页表（L2/L1/L0）逐层打印进程页表的每一项：索引、物理地址与权限位，叶子项额外打印所映射的虚拟地址；只打印有效表项，并在 exec 装载第一个用户进程时输出。测试为 make grade 的 pte printout 项。

### 任务二：每进程独立内核页表

为每个进程建立独立的内核页表，映射内核代码与数据、该进程的内核栈、UART、PLIC、VIRTIO 与 TRAMPOLINE；调度器在 swtch 前切换到进程的 k_pagetable 并刷新 TLB，进程让出 CPU 后切回全局内核页表。进程退出时用专门的回收函数释放页表结构，叶子映射只清表项、不释放其指向的物理页。测试为 kvmtest（test_pagetable 输出 1）与 usertests 全部通过。

### 任务三：用户映射并入内核页表

在 userinit、fork、growproc、exec 四个位置把用户页表的映射同步进进程的内核页表，使内核在自身页表下也能访问用户内存；同时为 copyin 与 copyinstr 加上 SSTATUS_SUM 开关并改走 vmcopyin.c 中的 copyin_new / copyinstr_new，统计两类拷贝的调用次数。验证为 stats stats 输出非零计数、usertests 全部通过、make grade 的 count copyin 与 kernel pagetable test 项。

## 快速开始

    git clone https://github.com/Ximmy416/xv6-oslabs-stu.git
    cd xv6-oslabs-stu

    # 选择要查看的实验分支
    git checkout pgtbl      # 实验四，也可换成 util / syscall / lock

    make qemu

## 技术栈

RISC-V 汇编 · C · xv6 操作系统 · QEMU 虚拟化 · GDB 调试 · 系统调用 · 进程管理与调度 · 页表与虚拟内存 · 自旋锁与并发 · 文件系统与 buffer cache
