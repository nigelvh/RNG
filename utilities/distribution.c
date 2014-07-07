/* Compile: gcc -Wall -O read_serial_port.c -o read_serial_port */

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	FILE *fp;

	fp  = fopen(argv[1], "rb");

	// Set up a couple variables
	int byte1 = 0;
	unsigned long distribution [256] = {0};
	unsigned char eof_reached = 0;

	// The device is now open, lets try reading from it.
	while(1){
		byte1 = fgetc(fp);

		if(byte1 == EOF){
			eof_reached = 1;
		}else{
			distribution[(unsigned char)byte1]++;
		}

		if(eof_reached){
			for(int i = 0; i < 256; i++){
				printf("%lu ", distribution[i]);
			}
			printf("\r\n");
			break;
		}
	}

	return 0;
}
