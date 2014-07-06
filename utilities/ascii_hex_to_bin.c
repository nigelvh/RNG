/* Compile: gcc -Wall -O ascii_hex_to_bin.c -o ascii_hex_to_bin */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

uint8_t read_uint8(int *reached_eof)
{
        int i, c;
        uint8_t value = 0;

        for (i = 0; i < 8; i++) {
                c = fgetc(stdin);
                if (c == EOF) {
                        *reached_eof = 1;
                        break;
                }
                c = (c == '0') ? 0 : 1;
                value = (value << 1) | c;
        }
        return value;
}

unsigned char hexval(unsigned char c)
{
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

int main(int argc, char *argv[])
{
	signed char byte1;
	signed char byte2;

	while(1){
		byte1 = fgetc(stdin);
		if(byte1 == EOF) break;
		byte2 = fgetc(stdin);
		if(byte2 == EOF) break;

		unsigned char num = (hexval(byte1) << 4) + hexval(byte2);

		fwrite(&num, sizeof(num), 1, stdout);
	}

        return 0;
}
