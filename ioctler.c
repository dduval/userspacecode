#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

int main(int argc, char *argv[]) {

	int myioctl = 0;
	int myfd;
	sscanf (argv[2], "%d", &myioctl);
	myfd=open(argv[1],O_RDWR);
        printf("%u\n",ioctl(myfd, myioctl, NULL));
	return 0;
}
