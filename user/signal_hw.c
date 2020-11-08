#include <stdio.h> 
#include <sys/types.h> 
#include <signal.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <sys/wait.h>
int N = 4;

void homework_wait() {
    pid_t pid[N];
    int i, child_status;    
    for(i=0;i<N;i++){
        if ((pid[i] = fork()) == 0) { 
            exit(100+i); /* Child */
        } 
    }
    printf("hello!\n"); 
    for(i=0;i<N;i++){/*Parent*/
        pid_t wpid = waitpid(pid[i], &child_status, 0);
        //pid_t wpid = wait(&child_status);
        if (WIFEXITED(child_status))
            printf("Child %d terminated with exit status %d\n", wpid, WEXITSTATUS(child_status));
        else
            printf("Child %d terminate abnormally\n", wpid);
    } 
}

int main() 
{
    int child_pid; 
    if((child_pid=fork())==0) 
    {
        while(1);
        printf("forbidden zone\n"); 
        exit(0);
    } 
    else 
    {
        while(getc(stdin))
        {          
            kill(child_pid, SIGKILL);       //杀死子进程
            printf("received a signal\n");
            wait(0);
            homework_wait();
            exit(0);
        }
    } 
    return 0;
}