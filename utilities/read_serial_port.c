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

uint8_t read_uint8(char c){
        int i;
        uint8_t value = 0;

		

        for (i = 0; i < 8; i++) {
                c = fgetc(stdin);
                c = (c == '0') ? 0 : 1;
                value = (value << 1) | c;
        }
        return value;
}

unsigned char hexval(char c){
        if('0' <= c && c <= '9'){
                return c - '0';
        }else if('A' <= c && c <= 'F'){
                return c - 'A' + 10;
        }else if('a' <= c && c <= 'f'){
                return c - 'a' + 10;
        }else{
                printf("Error: %d", c);
                abort();
        }
}

char deskew(char *c1, char *c2){
        if (*c1 == '0' && *c2 == '1')
        	return 0;
        else if (*c1 == '1' && *c2 == '0')
            return 1;
        else
			return -1;
}

int main(int argc, char *argv[]){
	// Check to ensure we've got an argument for the device we want to read from.
	if(argc > 1){
		//printf("%s\r\n", argv[1]);
	}else{
		printf("Please provide the tty device as an argument. EX: %s /dev/tty.usbSerial0\r\n", argv[0]);
		return 1;
	}

	// Try opening the device, and error if we can't.
	int fd = open(argv[1], O_RDONLY);
	if(fd < 0){
		printf("Could not open the tty device, %s. Error %d: %s\r\n", argv[1], errno, strerror(errno));
		return 2;
	}

	// Set up a couple variables
	char buf [128];
	unsigned char num = 0;
	unsigned char c1 = 0;
	unsigned char c2 = 0;
	char result = 0;
	int num_bytes = 0;

	// The device is now open, lets try reading from it.
	while(1){
		// Reset our variables.
		num = 0;
		c1 = 0;
		c2 = 0;
		result = -1;
		num_bytes = 0;

		// Check on how many bytes are available
		ioctl(fd, FIONREAD, &num_bytes);
		if(num_bytes >= 128){
			// Read two bytes from the serial port
			int n = read(fd, buf, sizeof(buf));
		
			// Check for an error
			if(n < 0){
				printf("Error reading from tty device. Error %d: %s\r\n", errno, strerror(errno));
				return 3;
			}
			if(n < 128){
				printf("Error reading from tty device. Fewer bytes than expected. Num bytes: %d", n);
				return 4;
			}
			
			// Read through the bytes
			for(unsigned char i = 1; i < 128; i+=2){
				if(buf[i-1] == EOF || buf[i] == EOF) return 0;
				
				result = deskew(&buf[i-1], &buf[i]);
				if(result >= 0){ result += '0'; fwrite(&result, sizeof(result), 1, stdout); }
				
				/*
				num = (hexval(buf[i-1]) << 4) + hexval(buf[i]);
				
				// Deskew and write out as ascii binary
				c1 = (num & 0b10000000) >> 7;
				c2 = (num & 0b01000000) >> 6;
				result = deskew(&c1, &c2);
				if(result >= 0){ result += '0'; fwrite(&result, sizeof(result), 1, stdout); }

				c1 = (num & 0b00100000) >> 5;
            	c2 = (num & 0b00010000) >> 4;
            	result = deskew(&c1, &c2);
            	if(result >= 0){ result += '0'; fwrite(&result, sizeof(result), 1, stdout); }

            	c1 = (num & 0b00001000) >> 3;
            	c2 = (num & 0b00000100) >> 2;
            	result = deskew(&c1, &c2);
            	if(result >= 0){ result += '0'; fwrite(&result, sizeof(result), 1, stdout); }

            	c1 = (num & 0b00000010) >> 1;
            	c2 = (num & 0b00000001);
            	result = deskew(&c1, &c2);
            	if(result >= 0){ result += '0'; fwrite(&result, sizeof(result), 1, stdout); }	
            	*/			
			}
		}else{
			usleep(100*1000); // Sleep for 100ms
		}
	}

	// Close the file descriptor.
	close(fd);

    return 0;
}
