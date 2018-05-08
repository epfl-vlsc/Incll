#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include <sys/time.h>

//majority of time is dominated by flush

#define NRUNS 10
unsigned long int timeFlush(struct timeval* tvalBefore, struct timeval* tvalAfter){
	return (((tvalAfter->tv_sec - tvalBefore->tv_sec)*1000000L
				+ tvalAfter->tv_usec) - tvalBefore->tv_usec);
}


int main(){
	int ret, fd;
	char *buf;
	struct timeval tvalBefore, tvalAfter;

	unsigned long int total = 0;

	printf("Test wbinvd\n");
	fd = open("/dev/global_flush", O_RDWR);
	if (fd < 0){
		perror("Failed to connect to device\n");
		return errno;
	}

	for(int i=0;i<NRUNS;++i){
		gettimeofday (&tvalBefore, NULL);
		ret = write(fd, buf, 0);
		if (ret < 0){
			perror("Failed to flush.");
			return errno;
		}
		gettimeofday (&tvalAfter, NULL);
		total += timeFlush(&tvalBefore, &tvalAfter);
	}

	printf("Time spent flushing %lu us\n", total/NRUNS);

	close(fd);
	return 0;
}
