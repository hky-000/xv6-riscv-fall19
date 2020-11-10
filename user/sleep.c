#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int sleepNum = atoi(argv[1]);

	if(argc != 2){
		fprintf(2, "usage: must 1 argument for sleep\n");
		exit();
	}
	
	printf("(nothing happens for a little while)\n");
	sleep(sleepNum);
	exit();
}

