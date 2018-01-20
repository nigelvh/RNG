/* (c) 2017 Nigel Vander Houwen */
/* Compile: gcc -Wall -O read_serial_port.c -o read_serial_port */

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>

int entFileHandle;

void exit_cleanup(int value){
	// Close file handles before exit
	if(entFileHandle > 0) close(entFileHandle);

	// Actually exit
	exit(value);
}

int main(int argc, char *argv[]){
	// Check to ensure we've got an argument for the device we want to read from.
	if(argc < 2){
		printf("Please provide the tty device as an argument. EX: %s /dev/tty.usbSerial0\r\n", argv[0]);
		return 1;
	}

	// Try opening the device, and error if we can't.
	entFileHandle = open(argv[1], O_RDONLY | O_NONBLOCK | O_NOCTTY);
	if(entFileHandle < 0){
		printf("<ERROR> Could not open the tty device, %s. Error %d: %s", argv[1], errno, strerror(errno));
		exit_cleanup(-8);
	}
	
	// Set serial interface attributes
	struct termios tty;
	if(tcgetattr(entFileHandle, &tty) != 0){
		printf("Could not get tty device attributes. Error %d: %s\r\n", errno, strerror(errno));
		exit_cleanup(-5);
	}

	// Set what we want the serial interface attributes to be
	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8 bit characters
	tty.c_iflag &= ~IGNBRK; // Ignore Breaks
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable hardware flow control
	tty.c_cflag |= (CLOCAL | CREAD); // Local control, enable reads
	tty.c_cflag &= ~(PARENB | PARODD); // No parity
	tty.c_cflag &= ~CSTOPB; 
	tty.c_cflag &= ~CRTSCTS;

	if(tcsetattr(entFileHandle, TCSANOW, &tty) != 0){
		printf("Could not set tty device attributes. Error %d: %s\r\n", errno, strerror(errno));
		exit_cleanup(-6);
	}

	// Assert and then clear DTR
#ifdef __APPLE__
	if(ioctl(entFileHandle, TIOCSDTR) != 0){
		printf("<ERROR> Could not assert DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-16);
	}
	if(ioctl(entFileHandle, TIOCCDTR) != 0){
		printf("<ERROR> Could not clear DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-17);
	}
#elif __linux
	int setdtr = TIOCM_DTR;
	if(ioctl(entFileHandle, TIOCMBIC, &setdtr) != 0){
		printf("<ERROR> Could not assert DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-16);
	}
	if(ioctl(entFileHandle, TIOCMBIS, &setdtr) != 0){
		printf("<ERROR> Could not clear DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-17);
	}
#endif

	// Set up a couple variables
	char buf [128];
	int result = 0;
	int num_bytes = 0;

	// The device is now open, lets try reading from it.
	while(1){
		// Reset our variables.
		result = -1;
		num_bytes = 0;

		// Check on how many bytes are available
		ioctl(entFileHandle, FIONREAD, &num_bytes);
		if(num_bytes >= 128){
			// Read bytes from the serial port
			int n = read(entFileHandle, buf, sizeof(buf));
		
			// Check for an error
			if(n < 0){
				printf("Error reading from tty device. Error %d: %s\r\n", errno, strerror(errno));
				return 3;
			}
			if(n < sizeof(buf)){
				printf("Error reading from tty device. Fewer bytes than expected. Num bytes: %d", n);
				return 4;
			}
			
			// Read through the bytes
			for(unsigned char i = 0; i < n; i++){
				fwrite(&buf[i], 1, 1, stdout);
			}
		}else{
			// Sleep for 100ms (non-blocking wait, lowers CPU time dramatically)
			usleep(100*1000);
		}
	}

	// Close the file descriptor.
	close(entFileHandle);

    return 0;
}
