#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[]){
    if(argc != 1) {
        printf("pingpong needs no argument!\n");
    }
    // 创建两个管道
    int pipe1[2], pipe2[2], pid;
    char buf[5];
    pipe(pipe1); // pipe1: parent -> child (ping)
    pipe(pipe2); // pipe2: child -> parent (pong)

    // Child
    if ((pid = fork()) == 0) {
        close(pipe1[1]);
        close(pipe2[0]);
        int childPid = getpid();
        if (read(pipe1[0], buf, 5) > 0) { // Child 读 ping
            fprintf(1, "%d: received ping\n", childPid);
            write(pipe2[1], "pong", 5); // Child 写 pong
        }
        close(pipe1[0]);
        close(pipe2[1]);
    } 
    else // Father
    {
        close(pipe1[0]);
        close(pipe2[1]);
        write(pipe1[1], "ping", 5); // Parent 写 ping
        int parentPid = getpid();
        if (read(pipe2[0], buf, 5) > 0) { // Parent 读 pong
            fprintf(1, "%d: received pong\n", parentPid);            
        }
        close(pipe1[1]);
        close(pipe2[0]);
    }
    exit(0);
}