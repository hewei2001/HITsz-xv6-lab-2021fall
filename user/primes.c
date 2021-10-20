#include "kernel/types.h"
#include "user.h"

void Childs(int p1[]);

int main(int argc,char* argv[]){
    if(argc != 1) {
        printf("Primes needs no argument!\n");
    }
    int p[2];
    pipe(p);

    if (fork() != 0){
        close(p[0]);
        for (int i = 2; i <= 35; i++){
            write(p[1], &i, sizeof(int));           // 写 2-35 到管道
        }
        close(p[1]);                                // 写完关闭 p写端
        wait(0);                                    // 等待子进程结束
    }
    else {
        Childs(p);                                  // 子进程中筛选
    }    
    exit(0);
}

void Childs(int p1[]){
    int prime, buf;
    close(p1[1]);                                   // 关闭 p1写端，让读出端可以判断传输的结束
    if (read(p1[0], &prime, sizeof(int)) > 0) {
        fprintf(1, "prime %d\n", prime);            // 读出第一个数，必为质数
        int p2[2];
        pipe(p2);                                   // 新的管道
        if(fork() != 0) {
            close(p2[0]);                           // 关闭 p2读端，写入数据
            while(read(p1[0], &buf, sizeof(int)) > 0){
                if(buf % prime) write(p2[1], &buf, sizeof(int));
            }                                       // 先读 p1 再写 p2
            close(p1[0]);
            close(p2[1]);                           // 读完关闭 p1读端和 p2写端
            wait(0);                                // 等待子进程结束
        }
        else {        
            Childs(p2);
        }
    }   
    exit(0);
}