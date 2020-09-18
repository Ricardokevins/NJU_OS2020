#include "lib.h"
#include "types.h"

int seed=39;
int myrand()
{
	int a=114514;
	int c=13277;
	int d=73;
	seed=(a*seed+c)%d;
	return seed;
}
int uEntry(void) {
	char ch;
	printf("Input: 1 for bounded_buffer\n       2 for philosopher\n       3 for reader_writer\n");
	scanf("%c", &ch);
	printf("%c",ch);
	switch (ch) {
		case '1':
			exec("/usr/bounded_buffer", 0);
			break;
		case '2':
			exec("/usr/philosopher", 0);
			break;
		case '3':
			exec("/usr/reader_writer", 0);
			break;
		default:
			break;
	}
	exit();
	return 0;
}

/*
int uEntry(void) 
{
	int dec = 0;
int hex = 0;
char str[6];
char cha = 0;
int ret = 0;
while(1){
printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
ret = scanf("Test %c Test %6s %d %x", &cha, str, &dec, &hex);
printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
if (ret == 4)
break;
}
return 0;
}



int uEntry(void) 
{

	int data = 2020;
	int data1 = 1000;
	int i = 4;
	int ret = fork();
	if (ret == 0) 
	{
		while (i != 0) 
		{
			i--;
			printf("Child Process: %d, %d\n", data, data1);
			write(SH_MEM, (uint8_t *)&data, 4, 0); // define SH_MEM 3
			data += data1;
			sleep(128);
		}
		exit();
	} 	
	else 
		if (ret != -1) 
		{
			while (i != 0) 
			{
				i--;
				read(SH_MEM, (uint8_t *)&data1, 4, 0);
				printf("Father Process: %d, %d\n", data, data1);
				sleep(128);
			}
			exit();
		}
	return 0;
}


int uEntry(void) {
int i = 4;
int ret = 0;
int value = 2;
sem_t sem;
printf("Father Process: Semaphore Initializing.\n");
ret = sem_init(&sem, value);
if (ret == -1) {
printf("Father Process: Semaphore Initializing Failed.\n");
exit();
}
ret = fork();
if (ret == 0) {
while( i != 0) {
i --;
printf("Child Process: Semaphore Waiting.\n");
sem_wait(&sem);
printf("Child Process: In Critical Area.\n");
}
printf("Child Process: Semaphore Destroying.\n");
sem_destroy(&sem);
exit();
}
else if (ret != -1) {
while( i != 0) {
i --;
printf("Father Process: Sleeping.\n");
sleep(128);
printf("Father Process: Semaphore Posting.\n");
sem_post(&sem);
}
printf("Father Process: Semaphore Destroying.\n");
sem_destroy(&sem);
exit();
}
return 0;
}
*/

