#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
    int n;
    char buf[512];
    char line[32][32];
    char *cmd[32];      //参数指针

    for (int i = 0; i < 32; i++)
        cmd[i] = line[i];

    for (int i = 1; i < argc; i++)
        strcpy(line[i - 1], argv[i]);    //第一行的参数

    while((n = read(0, buf, sizeof(buf))) > 0) {
        int pos = argc - 1;
        char *line_buf = line[pos];         //当前行首地址指针 
        for (char *c = buf; *c; c++) {
            if (*c == ' ' || *c == '\n') {
                *line_buf = '\0';        //该行字符串末尾\0
                pos++;                   //读完该行参数，pos++到下一行
                line_buf = line[pos];    //指向下一行首地址指针
            } else
                *line_buf++ = *c;        //缓存该行参数每个字符到line
        }
        pos++;               
        *line_buf = '\0';    
        cmd[pos] = 0;      //命令末尾

        if (fork() == 0) {  //子进程执行该命令
            exec(cmd[0], cmd);
        } else {
            wait();         //父进程等待
        } 
    }
    if(n < 0){
        printf("xargs: read error\n");
        exit();
    }
    exit();
}