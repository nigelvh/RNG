/* Compile: gcc -Wall -O ascii_to_bin.c -o ascii_to_bin */

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


int main(int argc, char *argv[])
{
        uint8_t byte;
        int reached_eof = 0;

        while (1) {
                byte = read_uint8(&reached_eof);
                if (reached_eof)
                        break;

                fwrite(&byte, sizeof(byte), 1, stdout);
        }

        return 0;
}
