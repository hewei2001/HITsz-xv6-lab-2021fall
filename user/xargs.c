#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char* argv[]) {
    char buf[128];
    int n;
    // 没有参数，读到什么输出什么
    if (argc == 1) {
        while((n = read(0, buf, sizeof buf)) > 0) {
            write(1, buf, n);           // n为读入的字节数
        }
        exit(0);
    }
    // 有若干参数，将参数存到新的变长数组
    char* args[MAXARG];
    int numArg = argc - 1;              // 传给子命令的参数少一个
    for (int i = 1; i < argc; ++i) {
        args[i-1] = argv[i];            // 传给子命令的参数
    }
    char* p = buf;
    while ((n = read(0, p, 1)) > 0) {   // 逐字节读入
        if (*p != '\n') {               // 换行符作为分隔符
            p++;
        } else {
            *p = 0;                     // 赋值0强制结束字符串
            if (fork() == 0) {          // 创建子进程来执行子命令
                args[numArg] = buf;     // 将输入的一行当作一个参数
                exec(args[0], args);
                exit(0);
            } else {
                wait(0);                // 等待子进程退出
            }
            p = buf;                    // 重新指向开头
        }
    }
    exit(0);
}

