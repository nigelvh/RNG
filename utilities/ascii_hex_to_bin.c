/* (c) 2014 Nigel Vander Houwen */
/* Compile: gcc -Wall -O ascii_hex_to_bin.c -o ascii_hex_to_bin */

#include <stdlib.h>
#include <stdio.h>

// Takes in a char 0-9,A-F,a-f and returns the int value 0-15
unsigned char hexval(unsigned char c){
	if('0' <= c && c <= '9'){
		return c - '0';
	}else if('A' <= c && c <= 'F'){
		return c - 'A' + 10;
	}else if('a' <= c && c <= 'f'){
		return c - 'a' + 10;
	}else{
		printf("Error: %c", c);
		abort();
	}
}

int main(int argc, char *argv[]){
	signed char byte1;
	signed char byte2;

	while(1){
		// Grab two bytes from STDIN, and make sure they're not EOFs
		byte1 = fgetc(stdin);
		if(byte1 == EOF) break;
		byte2 = fgetc(stdin);
		if(byte2 == EOF) break;

		// Grab the int value of each of the two chars, and add them together to get a byte's worth of data
		unsigned char num = (hexval(byte1) << 4) + hexval(byte2);

		// Write the raw byte out to stdout
		fwrite(&num, sizeof(num), 1, stdout);
	}

    return 0;
}
