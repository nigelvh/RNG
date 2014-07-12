/* (c) 2014 Nigel Vander Houwen */
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
	if(argc < 2){
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
	char result = 0;
	int num_bytes = 0;

	// The device is now open, lets try reading from it.
	while(1){
		// Reset our variables.
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
			for(unsigned char i = 1; i < n; i+=2){
				// If for some reason we get an EOF, quit.
				if(buf[i-1] == EOF || buf[i] == EOF) return 0;
				
				// Deskew (whiten) the data by reading two bytes
				result = deskew(&buf[i-1], &buf[i]);
				
				// If whitening threw away the bits, we'll get a -1, if we didn't lets output it.
				if(result >= 0){
					result += '0';
					fwrite(&result, sizeof(result), 1, stdout);
				}		
			}
		}else{
			// Sleep for 100ms (non-blocking wait, lowers CPU time dramatically)
			usleep(100*1000);
		}
	}

	// Close the file descriptor.
	close(fd);

    return 0;
}
