/* Compile: gcc -Wall -O vonneumann.c -o vonneumann -std=gnu99 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

int main(int argc, char *argv[]){
	int input, c1, c2, output=0, count=0, eof;

	while (1) {
		eof = read(STDIN_FILENO, &input, 1);
		if(eof == 0) return 0;

		for(int i = 0; i < 4; i++){
			c1 = (input >> (7 - (i * 2))) & 0b00000001;
			c2 = (input >> (6 - (i * 2))) & 0b00000001;

			if(c1 == 0 && c2 == 1){
				count++;
			}
			if(c1 == 1 && c2 == 0){
				output |= (1 << (7 - count));
				count++;
			}

			if(count >= 8){
				count = 0;
				fputc(output, stdout);
				output = 0;
			}
		}
	}

	return 0;
}
