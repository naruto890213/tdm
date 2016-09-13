#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	sleep(5);
	printf("start to exec system cmd\n");
	system("echo zrway.com | sudo ./tdmRestart.sh");
	sleep(5);
	printf("after to exec system cmd\n");

	return 0;
}
