#ifndef __lib_h__
#define __lib_h__

#include "types.h"

#define SYS_WRITE 0
#define SYS_FORK 1
#define SYS_EXEC 2
#define SYS_SLEEP 3
#define SYS_EXIT 4
#define SYS_READ 5
#define SYS_SEM 6
#define SYS_GETPID 7
#define SYS_OPEN 8
#define SYS_LSEEK 9
#define SYS_CLOSE 10
#define SYS_REMOVE 11
#define SYS_LS 12

#define STD_OUT 0
#define STD_IN 1
#define SH_MEM 3

#define SEM_INIT 0
#define SEM_WAIT 1
#define SEM_POST 2
#define SEM_DESTROY 3

#define O_WRITE 0x01
#define O_READ 0x02
#define O_CREATE 0x04
#define O_DIRECTORY 0x08

#define MAX_BUFFER_SIZE 256

int printf(const char *format,...);

int scanf(const char *format,...);

pid_t fork();

int exec(const char *filename, char * const argv[]);

int sleep(uint32_t time);

int exit();

int open(char *path, int flags);

int lseek(int fd, int offset, int whence);

int close(int fd);

int remove (char *path);

int write(int fd, uint8_t *buffer, int size, ...);

int read(int fd, uint8_t *buffer, int size, ...);

int do_ls(char *path, char *res);

int sem_init(sem_t *sem, uint32_t value);

int sem_wait(sem_t *sem);

int sem_post(sem_t *sem);

int sem_destroy(sem_t *sem);

int getpid();

#endif
