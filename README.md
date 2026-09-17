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
| `syscall` | 实验二：系统调用（跟踪系统调用、添加新系统调用） | 待开始 |
| `lazy` | 实验三：懒分配与写时复制 | 待开始 |

## 实验一（util）内容

### 用户程序

- **sleep**：接收一个 tick 参数，调用 sleep() 系统调用让进程休眠指定 tick 数
- **pingpong**：通过两条管道和 fork() 实现父子进程双向通信（ping/pong）
- **find**：递归遍历目录树，查找指定名称的文件或目录

### 启动流程调试

使用 GDB 跟踪 xv6 从 _entry 到第一个用户进程 init 的完整启动过程，通过断点观察进程名从 "initcode" 到 "init" 的切换，理解 exec() 系统调用不创建新进程、仅替换当前进程程序映像的原理。

## 快速开始

    git clone https://github.com/Ximmy416/xv6-oslabs-stu.git
    cd xv6-oslabs-stu
    git checkout util
    make qemu

## 技术栈

RISC-V 汇编 · C · xv6 操作系统 · QEMU 虚拟化 · GDB 调试 · 系统调用 · 进程管理 · 文件系统
