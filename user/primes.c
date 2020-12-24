#include "kernel/types.h"
#include "user/user.h"

void func(int *input, int num){
	if(num == 1){					// 剩的最后一个数是质数
		printf("prime %d\n", *input);
		return;
	}
	int p[2],i;
	int prime = *input;
	int temp;
	printf("prime %d\n", prime);	// 每次进入管道的第一个是质数
	pipe(p);						// 新建一个管道
    if(fork() == 0){				// 子进程写入管道当前还剩的所有数
        for(i = 0; i < num; i++){
            temp = *(input + i);
			write(p[1], (char *)(&temp), 4);
		}
        exit();
    }
	close(p[1]);
	if(fork() == 0){
		int counter = 0;
		char buffer[4];
		while(read(p[0], buffer, 4) != 0){ // 依次从管道中读一个数
			temp = *((int *)buffer);
			if(temp % prime != 0){			// 不是第一个数(质数)的倍数
				*input = temp;				// input记录要输出给下一个管道的数
				input += 1;
				counter++;
			}
		}
		func(input - counter, counter);		// 下一个管道一共有counter个数输入
		exit();
    }
    wait();
    wait();
}

int main(){
    int input[34];
	for(int i = 0; i < 34; i++){
		input[i] = i+2;
	}
	func(input, 34);
    exit();
}