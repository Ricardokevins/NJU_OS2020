#include "lib.h"
#include "types.h"

/*
#define N 5 // 哲学家个数
semaphore fork[5]; // 信号量初值为1
void philosopher(int i){ // 哲学家编号：0-4
while(TRUE){
think(); // 哲学家在思考
if(i%2==0){
P(fork[i]); // 去拿左边的叉⼦
P(fork[(i+1)%N]); // 去拿右边的叉⼦
} else {
P(fork[(i+1)%N]); // 去拿右边的叉⼦
P(fork[i]); // 去拿左边的叉⼦
}
eat(); // 吃⾯条
V(fork[i]); // 放下左边的叉⼦
V(fork[(i+1)%N]); // 放下右边的叉⼦
}
}
*/

#define N 5
sem_t Myfork[5];
void philosopher(int i)
{
	int pid = getpid();
	while(1)
	{
		printf("Thinking %d\n",pid);
		sleep(128);
		if(i%2==0)
		{
			sem_wait(&Myfork[i]);
			sleep(128);
			sem_wait(&Myfork[(i+1)%N]);
			sleep(128);
		}
		else
		{
			sem_wait(&Myfork[(i+1)%N]);
			sleep(128);
			sem_wait(&Myfork[i]);
			sleep(128);
		}
		printf("Eating %d\n",pid);
		sleep(128);
		sem_post(&Myfork[i]);
		sleep(128);
		sem_post(&Myfork[(i+1)%N]);
		sleep(128);
	}
}
int main(void) {
	// TODO in lab4
	printf("philosopher\n");
	for(int i=0;i<5;i++)
	{
		sem_init(&Myfork[i],1);
	}
	for (int i = 0; i < 5; ++i) 
	{
        if (fork() == 0) 
		{
            philosopher(i);
        }    
    }
	
	exit();
	for(int i=0;i<5;i++)
	{
		sem_destroy(&Myfork[i]);
	}
	return 0;
}
