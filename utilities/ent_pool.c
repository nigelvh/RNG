/* (c) 2014 Nigel Vander Houwen */
/* On OSX hosts, will test reading a byte from /dev/random, and then writing that back to /dev/random. */
/* On Linux hosts, will test reading the size of the entropy pool, and pushing data in, via the ioctl interface. */
/* Compile: gcc -Wall -O ent_pool.c -o ent_pool */

#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __APPLE__
	// Do something
#elif __linux__
	#include <linux/random.h>
#endif

int main(int argc, char *argv[]){
	int fd = open("/dev/random", O_RDWR);
	if(fd < 0){
		printf("Could not open the /dev/random device. Error %d: %s\r\n", errno, strerror(errno));
		return 1;
	}
	
	// If we're on OSX, we don't have an ioctl interface, just push data into /dev/random
	#ifdef __APPLE__
		char buf [1];
		int n = read(fd, buf, sizeof(buf));
		
		// Check for an error
		if(n < 0){
			printf("Error reading from /dev/random device. Error %d: %s\r\n", errno, strerror(errno));
			return 3;
		}
		if(n < sizeof(buf)){
			printf("Error reading from /dev/random device. Fewer bytes than expected. Num bytes: %d", n);
			return 4;
		}
		
		printf("Read %d bytes from /dev/random.\r\n", n);
		
		int m = write(fd, buf, sizeof(buf));
		
		// Check for an error
		if(m < 0){
			printf("Error writing to /dev/random device. Error %d: %s\r\n", errno, strerror(errno));
			return 3;
		}
		if(m < sizeof(buf)){
			printf("Error writing to /dev/random device. Fewer bytes than expected. Num bytes: %d", m);
			return 4;
		}
		
		printf("Wrote %d bytes to /dev/random.\r\n", m);
	#endif
	
	// If we're on linux, read the current count from the entropy pool, and push new stuff in via ioctl
	#ifdef __linux__
		struct {
			int ent_count;
			int size;
			unsigned char data[1024];
		} entropy;	

		entropy.data[0] = 42;
		entropy.size = 1;
		entropy.ent_count = 8;

		int count = 0;
		if( ioctl(fd, RNDGETENTCNT, &count) < 0 ){
			printf("Could not read the entropy count. Error %d: %s\r\n", errno, strerror(errno));
			return 2;
		}
		printf("Entropy Pool Size: %d\r\n", count);
		if( ioctl(fd, RNDADDENTROPY, &entropy) < 0){
			printf("Could not add entropy to the pool. Error %d: %s\r\n", errno, strerror(errno));
			return 5;
		}
		if( ioctl(fd, RNDGETENTCNT, &count) < 0 ){
			printf("Could not read the entropy count. Error %d: %s\r\n", errno, strerror(errno));
			return 2;
		}
		printf("Entropy Pool Size: %d\r\n", count);
	#endif
	
	// Close the file descriptor.
	close(fd);
	
    return 0;
}