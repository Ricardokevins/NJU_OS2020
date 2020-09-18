#include "lib.h"
#include "types.h"

/*
int seed=39;
int myrand()
{
	int a=114514;
	int c=13277;
	int d=73;
	seed=(a*seed+c)%d;
	return seed;
}
*/



int uEntry(void) 
{
	char ch;
	printf("Input: 1 for Testcase\n       2 enter Shell\n");
	scanf("%c", &ch);
	switch (ch) 
	{
		case '1':
			exec("/usr/testcase", 0);
			break;
		case '2':
			exec("/usr/Shell", 0);
			break;
		default:
			break;
	}
	exit();
	return 0;
}

