#include "lib.h"
#include "types.h"

/*
P(WriteMutex);
write;
V(WriteMutex);
*/

/*
P(CountMutex);
if (Rcount == 0)
P(WriteMutex);
++Rcount;
V(CountMutex);
read;
P(CountMutex);
--Rcount;
if (Rcount == 0)
V(WriteMutex);
V(CountMutex);
*/

void Writer(int id,sem_t* WriteMutex)
{

	int pid = getpid();
	while(1)
	{
		//printf("Win %d\n",pid);
		sem_wait(WriteMutex);

		sleep(128);
		printf("Write %d\n",pid);
		sleep(128);

		sem_post(WriteMutex);
		sleep(128);
	}
	
}

//write(SH_MEM, (uint8_t *)&data, 4, 0); 
//read(SH_MEM, (uint8_t *)&data1, 4, 0);

void Reader(int id,sem_t* WriteMutex,sem_t* CountMutex)
{
	int Rcount=0;
	int pid = getpid();
	while(1)
	{
		//printf("Enter %d\n",pid);
		sem_wait(CountMutex);
		//printf("get count key %d\n",pid);
		sleep(128);
		read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		//printf("count %d\n",Rcount);
		if((Rcount)==0)
		{
			sem_wait(WriteMutex);
			sleep(128);
		}
		read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		(Rcount)+=1;
		write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		//printf("aftercount %d\n",Rcount);
		sleep(128);
		sem_post(CountMutex);
		//printf("return count key %d\n",pid);
		sleep(128);
		printf("Read %d total %d \n",pid,Rcount);
		sleep(128);
		//printf("Wait %d\n",pid);
		sem_wait(CountMutex);
		//printf("get count key again %d\n",pid);
		sleep(128);
		read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		(Rcount)--;
		write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		sleep(128);
		read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		if ((Rcount) == 0)
		{
			sem_post(WriteMutex);
			sleep(128);
		}
		sem_post(CountMutex);
		sleep(128);
	}
}
int main(void) {
	// TODO in lab4
	printf("reader_writer\n");
	sem_t WriteMutex;
	sem_t CountMutex;
	sem_init(&WriteMutex,1);
	sem_init(&CountMutex,1);
	int Rcount=0;
	write(SH_MEM, (uint8_t *)&Rcount, 4, 0); 
	for (int i = 0; i < 6; ++i) 
	{
        if (fork() == 0) 
		{
			//int pid = getpid();
			//printf("%d %d\n",pid,i);
			if(i<3)
			{
				Reader(i,&WriteMutex,&CountMutex);
			}
			else
			{
				Writer(i,&WriteMutex);
			}
        }    
    }
	exit();
	sem_destroy(&WriteMutex);
	sem_destroy(&CountMutex);
	return 0;
}
