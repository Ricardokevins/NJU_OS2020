#include "lib.h"
#include "types.h"


int product=0;
void producer(int id, sem_t* mutex, sem_t* fullbuffer ,sem_t* emptybuffer) 
{
    int pid = getpid();
    for (int k = 1; k <= 8; ++k) 
	{
        
        sem_wait(emptybuffer);
         sleep(128);
        
        sem_wait(mutex);
         sleep(128);
        printf("Producer %d: produce\n",pid);
        sleep(128);
        
        sem_post(mutex);
         sleep(128);
       
        sem_post(fullbuffer);
         sleep(128);
    }
}

void consumer(int id, sem_t* mutex, sem_t* fullbuffer ,sem_t* emptybuffer) 
{
    int pid = getpid();
    for (int k = 1; k <= 4; ++k) 
	{
        
        sem_wait(fullbuffer);
         sleep(128);
        
        sem_wait(mutex);
         sleep(128);
        printf("Consumer : consume\n",pid);
        sleep(128);
         sleep(128);
        
        sem_post(mutex);
         sleep(128);
        
        sem_post(emptybuffer);
         sleep(128);
    }
}


int main(void) {
	// TODO in lab4
	printf("bounded_buffer\n");
	sem_t mutex;
    sem_t emptybuffer;
    sem_t fullbuffer;
	sem_init(&mutex, 1);
    sem_init(&fullbuffer, 0);
    sem_init(&emptybuffer, 3);
	for (int i = 0; i < 5; ++i) 
	{
        if (fork() == 0) 
		{
            if (i < 1)
				consumer(i + 1, &mutex, &fullbuffer, &emptybuffer);          
            else
                producer(i, &mutex, &fullbuffer, &emptybuffer);
            break;
        }    
    }
   
	exit();
    sem_destroy(&mutex);
    sem_destroy(&emptybuffer);
    sem_destroy(&fullbuffer);
	return 0;
}
