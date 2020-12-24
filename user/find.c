#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


//将路径格式化为文件名
char* fmtname(char *path){
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash. 找到最后一个'/'后面的文件名
  for(p=path+strlen(path); p >= path && *p != '/'; p--);
  p++;
  memmove(buf, p, strlen(p)+1);
  return buf;
}

int match(char *re, char *text){
    return strcmp(re, text);
}

void find(char *path, char *re)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;     // 记录文件/文件夹信息

    if((fd = open(path, 0)) < 0){
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
        case T_FILE:    //该路径是个文件
            if(!match(re, fmtname(path)))    //文件名匹配
                printf("%s\n", path);
            break;

        case T_DIR:     //该路径是个目录
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);    // p指向buf的最后
            *p++ = '/';             // 最后加上'/'
            while(read(fd, &de, sizeof(de)) == sizeof(de)){ // 该目录下的所有文件/文件夹
                if(de.inum == 0)
                continue;
                memmove(p, de.name, DIRSIZ);    // 拼上该路径
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                //Don't recurse into "." and ".."
                if(strlen(de.name) == 1 && de.name[0] == '.')       
                    continue;
                if(strlen(de.name) == 2 && de.name[0] == '.' && de.name[1] == '.')
                    continue;
                find(buf, re);      //递归找该子目录下的文件
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if(argc <= 2){
        fprintf(2, "find: not enough params provided");
    }
    else if (argc == 3){
        find(argv[1], argv[2]);
    }
    else {
        fprintf(2, "find: only two params allowed");
    }
    exit();
}