#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

int main(){
   int ret, fd;

   printf("Test wbinvd\n");
   fd = open("/dev/global_fence", O_RDWR);
   if (fd < 0){
      perror("Failed to flush");
      return errno;
   }
   
   
   close(fd);
   return 0;
}
