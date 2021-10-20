#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* fmtname(char *path) {
    static char buf[DIRSIZ+1];
    char *p;
    // 指向最后一个 / 后的文件名
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;
    // 返回路径截取后的值
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    buf[strlen(p)] = '\0';
    return buf;
}

void find(char* path, char* file) 
{
    char buf[512], *p;
    int fd;
    struct dirent de;   // 目录项结构体
    struct stat st;     // 文件的统计信息

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
    case T_FILE:    // 对文件操作
        if (strcmp(fmtname(path), file) == 0) {
            printf("%s\n", path);
        }
        break;
    case T_DIR:     // 对目录操作
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("find: path too long\n");    // 解决缓冲区溢出
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);  // 将指针指向buf末尾
        *p++ = '/';             // 将 \0 改为 /
        while(read(fd, &de, sizeof de) == sizeof de) {
            if (de.inum == 0)   
                continue;       // 此文件夹无文件
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                continue;       // 不递归进入 . 和 ..
            }
            memmove(p, de.name, DIRSIZ);    // 将文件名移动到指针
            p[DIRSIZ] = 0;      // 在文件末尾加上0
            find(buf, file);    // 递归进入子目录
        }
        break;
    default:
        break;
    }
    close(fd);
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(2, "usage: find [path] [filename]\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}