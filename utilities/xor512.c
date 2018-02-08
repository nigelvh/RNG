/* Compile: gcc -Wall -O xor512.c -o xor512 -std=gnu99 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

int buf [512] = {0};
int eof, working;

int main(int argc, char *argv[]){
	while(1){
		for(int i = 0; i < 512; i++){
			eof = read(STDIN_FILENO, &working, 1);
			if(eof == 0) return 0;
			buf[i] = working;
		}
	
		for(int i = 0; i < 512; i++){
			eof = read(STDIN_FILENO, &working, 1);
			if(eof == 0) return 0;
			fputc((buf[i] ^ working), stdout);
		}
	}

	return 1;
}
