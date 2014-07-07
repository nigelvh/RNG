/* Compile: gcc -Wall -O distribution.c -o distribution */
/* Takes in a file as an argument, reads through the file, and outputs a space */
/* separated list of the total count of each character value from 0-255        */
/* Nigel VH                                                                    */

#include <stdio.h>

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
