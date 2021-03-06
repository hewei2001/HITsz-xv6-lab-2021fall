# HITsz-xv6-lab-2021fall

哈工大（深圳）2021 年秋季学期「操作系统」[课程指导书](http://hitsz-lab.gitee.io/os-labs-2021)

原课程 xv6-mit-6.S081-2020 主页：[6.S081: Operating System Engineering](https://pdos.csail.mit.edu/6.S081/2020/index.html)

## 实验内容

每个实验在对应分支下，点击 `commits` 可查看代码修改，四次实验均通过完整测试。

### Lab1：XV6与Unix实用程序 

实现 5 个 Unix 实用程序：sleep、pingpong、primes、find、xargs。

本实验与原版 xv6 实验相同，代码在 `util` 分支下。

### Lab2：系统调用 

分为两个任务：

1. 系统调用信息打印：添加一个名为 trace 的系统调用，来帮助你在之后的实验中进行 debug。

2. 添加系统调用 sysinfo：该系统调用将收集 xv6 运行的一些信息。

本实验任务一中 trace 的打印与原版 xv6 实验**略有不同**，代码在 `syscall` 分支下。

### Lab3：锁机制的应用 

分为两个任务：

1. 内存分配器（Memory allocator）：修改空闲内存列表管理机制，使每个 CPU 核使用独立的列表。

2. 磁盘缓存（Buffer cache）：修改磁盘缓存块列表的管理机制，使得可用多个锁管理缓存块，从而减少缓存块管理的锁争用。

本实验与原版 xv6 实验相同，代码在 `lock` 分支下。

### Lab4：页表

分为三个任务：

1. 打印页表：加入页表打印功能，来帮助你在之后的实验中进行 debug。

2. 独立内核页表：将共享内核页表改成独立内核页表，使得每个进程拥有自己独立的内核页表。

3. 简化软件模拟地址翻译：在独立内核页表加上用户地址空间的映射，同时将函数 copyin()/copyinstr() 中的软件模拟地址翻译改成直接访问。

本实验任务一中页表的打印与原版 xv6 实验**略有不同**，代码在 `pgtbl` 分支下。

## 注意

所有课程实验都需要**独立完成**。你应当主动避免阅读、使用任何人/互联网上的 XV6 实验代码 。在实验中碰到问题很正常，发现问题时需沉着冷静，认真阅读实验指导书，查阅 MIT 6.S081 的官方网站以及 XV6 book，尝试自己调试解决，也可以向同学/助教/老师请教调试的技巧。只有这样，你才能够得到真正的训练与收获！ 
