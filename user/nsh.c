#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXARGS 10
#define MAXWORD 30
#define MAXLINE 100 

void run_cmd(char* args_p[], int args_n);

char whitespace[] = " \t\r\n\v";
char args[MAXARGS][MAXWORD];        // store each word in a cmd 

int get_cmd(char *buf, int n_buf)   /* ref: user/sh.c getcmd*/
{
    fprintf(2, "@ ");       // shell input sign @
    memset(buf, 0, n_buf);  
    gets(buf, n_buf);       // get user input cmd
    if(buf[0] == 0)         // EOF
        return -1;
    return 0;
}

void mycrash(char *info)  /* ref: user/sh.c panic*/
{
    fprintf(2, "%s\n", info);   // system call return error
    exit(-1);
}

int my_fork()   /* ref: user/sh.c fork1 */
{
    int pid = fork();
    if (pid < 0)    // fail to fork
        mycrash("fork");
    return pid;
}

void split_args(char *buf, char* args_p[], int* args_n)
{
    for (int i = 0; i < MAXARGS; i ++)
        args_p[i] = &args[i][0];        // args_p: a pointer to first position of each word
    int i = 0, j = 0;
    while(buf[j] != '\n' && buf[j] != '\0')
    {
        while(strchr(whitespace, buf[j]))
            j++;        // skip whitespace
        args_p[i++] = buf + j;
        if (i >= MAXARGS)
            mycrash("Too many args");
        while(strchr(whitespace, buf[j]) == 0)
            j++;        // skip to the word-end
        buf[j] = '\0';
        j++;
    }
    args_p[i] = 0;      // end of cmd
    *args_n = i;        // length of cmd
}


void run_pipe(char* args_p[], int args_n)
{
    int i;
    for (i = 0; i < args_n; i ++)
        if (!strcmp(args_p[i], "|")){
            // find the first '|' 
            args_p[i] = 0;
            break;
        }
    
    int p[2];
    if (pipe(p) < 0){
        mycrash("pipe");
    }
    if (my_fork() == 0){
        // do the left cmd
        close(1);       
        dup(p[1]);      // lead pipe input side to stdin
        close(p[0]);
        close(p[1]);    // close child pipe in & out
        run_cmd(args_p, i);
    }
    else {
        // parent, do the right cmd
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        run_cmd(args_p+i+1, args_n-i-1);
    }
}

void run_cmd(char* args_p[], int args_n)
{
    for (int i = 1; i < args_n; i ++)
        if (!strcmp(args_p[i], "|"))    // do with pipe
            run_pipe(args_p, args_n);
    // do with redirect
    for (int i = 1; i < args_n; i ++)
    {
        if (!strcmp(args_p[i], ">"))    
        {
            // close stdout, redirect to args
            close(1);
            open(args_p[i+1], O_CREATE|O_WRONLY);
            args_p[i] = 0;    
            
        }
        if (!strcmp(args_p[i], "<"))
        {
            // close stdin, redirect to args
            close(0);
            open(args_p[i+1], O_RDONLY);
            args_p[i] = 0; 
            
        }
    }
    exec(args_p[0], args_p);
}

int main()
{
    static char buf[MAXLINE];

    while (get_cmd(buf, sizeof(buf)) >= 0)
    {
        if (my_fork()==0){  // child
            char* args_p[MAXARGS];
            int args_n = -1;         // num of words in a cmd
            split_args(buf, args_p, &args_n);   // split words in cmd
            run_cmd(args_p, args_n);
        } 
        else {              // parent
            wait(0);
        }
    }
    exit(0);
}