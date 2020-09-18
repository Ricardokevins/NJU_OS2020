
#include "lib.h"
#include "types.h"
int ls(char *destFilePath) 
{

	printf("ls %s\n", destFilePath);
	char info[1024];
	do_ls(destFilePath, info);
	printf("%s\n", info);
	return 0;
}

int cat(char *destFilePath) 
{
    printf("cat %s\n", destFilePath);
	char buf[1024]={0};
	int fd = open(destFilePath, O_READ);
	
	read(fd, (uint8_t *)buf, 1024);
	printf("%s", buf);
	printf("\n");
	close(fd);
	return 0;
}


int main(void) 
{
	int fd = 0;
	int i = 0;
	char tmp = 0;
	printf("Start Test !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	ls("/");
	ls("/boot/");
	ls("/dev/");
	ls("/usr/");
	fd = open("/usr/test", O_WRITE | O_READ | O_CREATE);
	for (i = 0; i < 26; i ++) 
    {
		tmp = (char)(i % 26 + 'A');
		write(fd, (uint8_t*)&tmp, 1);
	}
	close(fd);
	ls("/usr/");
	cat("/usr/test");
	printf("test end\n");
    exit();
    return 1;
}

