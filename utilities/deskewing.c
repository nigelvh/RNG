/* Compile: gcc -Wall -O deskewing.c -o deskewing */

#include <stdio.h>

int main(int argc, char *argv[])
{
        int c1, c2;

        while (1) {
                c1 = fgetc(stdin);
                c2 = fgetc(stdin);

                if (c1 == EOF || c2 == EOF)
                        break;

                if (c1 == '0' && c2 == '1')
                        printf("0");
                else if (c1 == '1' && c2 == '0')
                        printf("1");
        }

        return 0;
}

