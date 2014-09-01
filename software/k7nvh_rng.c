#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <math.h>

// Debug
int debug = 0;

// Define some directories
#define pidfile "/tmp/k7nvh_rng.pid"
#define logfile "/var/log/k7nvh_rng.log"
#define outfile "/tmp/k7nvh_rng_out.ascii"
#define rundir "/"

// File handles
int pidFileHandle;
int logFileHandle;
int entFileHandle;
int outFileHandle;

// Buffer and state regarding reading from serial device
char buf [128] = {0};
int result = 0;
int num_bytes = 0;

// Buffer and state regarding whitening data
char buf_whitened [512] = {0};
int buf_whitened_pos = 0;

// Entropy estimation variables
int ent_est_interval = 300;
int test_ent = 0;
int num_ones = 0;
int num_zeros = 0;
float average = 0.0;
float est_ent = 0.0;
int ent_count = 0;

void exit_cleanup(int value){
	// Close file handles before exit
	if(pidFileHandle > 0) close(pidFileHandle);
    if(logFileHandle > 0) close(logFileHandle);
    if(entFileHandle > 0) close(entFileHandle);
    
    // Actually exit
    exit(value);
}

void log_message(char *message){
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char timestr[30];
	strftime(timestr, 30, "%F %T", timeinfo);
	
	char logentry[1024];
	sprintf(logentry, "%s K7NVH_RNG[%d]: %s\r\n", timestr, getpid(), message);
	write(logFileHandle, &logentry, strlen(logentry));
}

void signal_handler(int sig){
	char logmessage[200];
	signal(sig, signal_handler);

    switch(sig){
    	case SIGALRM:
    		if(debug > 0) log_message("<DEBUG> Received SIGALRM signal.");
    		test_ent = 1;
    		break;
        case SIGHUP:
            log_message("<NOTICE> Received SIGHUP signal.");
            break;
        case SIGINT:
        case SIGTERM:
            log_message("<NOTICE> Received SIGTERM. Exiting.");
            exit_cleanup(0);
            break;
        default:
            sprintf(logmessage, "<WARNING> Unhandled signal %s", strsignal(sig));
            log_message(logmessage);
            break;
    }
}

int deskew(char *c1, char *c2){
        if (*c1 == '0' && *c2 == '1')
        	return 0;
        else if (*c1 == '1' && *c2 == '0')
            return 1;
        else
			return -1;
}

int main(int argc, char *argv[]) {
    // Check to ensure we've got an argument for the device we want to read from.
	if(argc < 2){
		printf("Please provide the tty device as an argument. EX: %s /dev/tty.usbSerial0\r\n", argv[0]);
		exit_cleanup(-7);
	}

    // Our process ID and Session ID
    pid_t pid, sid;
        
    // Fork off the parent process
    pid = fork();
    if (pid < 0) {
        exit_cleanup(-2);
    }
    // If we got a good PID, then we can exit the parent process.
    if (pid > 0) {
        exit_cleanup(0);
    }

    // Change the file mode mask
    umask(0);
                
    // Open any logs here
    logFileHandle = open(logfile, O_RDWR|O_CREAT|O_APPEND, 0644);
	if(logFileHandle < 0){
		// Unable to open our log, exit.
		printf("<ERROR> Could not open log file. %s\r\n", strerror(errno));
		exit_cleanup(-1);
	}
    
    log_message("<INFO> Starting entropy gathering daemon...");
                
    // Create a new SID for the child process
    sid = setsid();
    if (sid < 0) {
        log_message("<ERROR> Could not get new SID.");
        exit_cleanup(-3);
    }
        
    // Change the current working directory
    if ((chdir(rundir)) < 0) {
        log_message("<ERROR> Could not change working directory.");
        exit_cleanup(-4);
    }
    
    // Check for an existing PID
    pidFileHandle = open(pidfile, O_RDWR|O_CREAT, 0600);
    if(pidFileHandle < 0){
    	log_message("<ERROR> PID file exists or could not be created.");
    	exit_cleanup(-5);
    }
    
    // Lock the PID file
    if(lockf(pidFileHandle, F_TLOCK, 0) < 0){
    	log_message("<ERROR> PID file could not be locked.");
    	exit_cleanup(-6);
    }
    
    // Write PID to the PID file
    char str[10];
    sprintf(str, "%d\n", getpid());
    write(pidFileHandle, str, strlen(str));
            
    // Close out the standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Set up the signal handlers
	signal(SIGALRM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
    
    // Daemon-specific initialization goes here
	// Try opening the tty device, and error if we can't.
	entFileHandle = open(argv[1], O_RDONLY | O_NONBLOCK | O_NOCTTY);
	if(entFileHandle < 0){
		char logmessage[200];
		sprintf(logmessage, "<ERROR> Could not open the tty device, %s. Error %d: %s", argv[1], errno, strerror(errno));
		log_message(logmessage);
		exit_cleanup(-8);
	}

	// Read serial interface attributes
	struct termios tty;
	if(tcgetattr(entFileHandle, &tty) != 0){
		char logmessage[200];
		sprintf(logmessage, "<ERROR> Could not get the tty device attributes. Error %d: %s", errno, strerror(errno));
		log_message(logmessage);
		exit_cleanup(-13);
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

	// Set the serial interface attributes
	if(tcsetattr(entFileHandle, TCSANOW, &tty) != 0){
		char logmessage[200];
		sprintf(logmessage, "<ERROR> Could not set the tty device attributes. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-14);
	}

	// Assert and then clear DTR
#ifdef __APPLE__
	if(ioctl(entFileHandle, TIOCSDTR) != 0){
		char logmessage[200];
		sprintf(logmessage, "<ERROR> Could not assert DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-16);
	}
	if(ioctl(entFileHandle, TIOCCDTR) != 0){
		char logmessage[200];
		sprintf(logmessage, "<ERROR> Could not clear DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-17);
	}
#elif __linux
	int setdtr = TIOCM_DTR;
	if(ioctl(entFileHandle, TIOCMBIC, &setdtr) != 0){
		char logmessage[200];
		sprintf(logmessage, "<ERROR> Could not assert DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-16);
	}
	if(ioctl(entFileHandle, TIOCMBIS, &setdtr) != 0){
		char logmessage[200];
		sprintf(logmessage, "<ERROR> Could not clear DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-17);
	}
#endif
	
	// Temporarily send data to a file rather than the entropy pool
	outFileHandle = open(outfile, O_RDWR|O_CREAT|O_APPEND, 0644);
	if(outFileHandle < 0){
		char logmessage[200];
		sprintf(logmessage, "<ERROR> Could not open output file. Error %d: %s", errno, strerror(errno));
		log_message(logmessage);
		exit_cleanup(-11);
	}

	// Set up an alarm so we periodically test the quality of our entropy
	alarm(ent_est_interval);
	char logmessage[200];
	sprintf(logmessage, "<INFO> Next estimation of entropy in %d seconds.", ent_est_interval);
	log_message(logmessage);

    log_message("<INFO> Entropy gathering daemon started.");
    
    // The Big Loop
    while (1) {
        // Reset our variables.
		result = -1;
		num_bytes = 0;
		
		if(debug > 0) log_message("<DEBUG> Main loop...");

		// Check on how many bytes are available
		ioctl(entFileHandle, FIONREAD, &num_bytes);
		if(num_bytes >= sizeof(buf)){
			if(debug > 0) log_message("<DEBUG> Reading bytes...");
		
			// Read sizeof(buf) bytes from the serial port
			int n = read(entFileHandle, buf, sizeof(buf));
		
			// Check for an error
			if(n < 0){
				char logmessage[200];
				sprintf(logmessage, "<ERROR> Error reading from tty device. Error %d: %s", errno, strerror(errno));
				log_message(logmessage);
				exit_cleanup(-9);
			}
			if(n < sizeof(buf)){
				char logmessage[200];
				sprintf(logmessage, "<ERROR> Error reading from tty device. Fewer bytes than expected. Num bytes: %d", n);
				log_message(logmessage);
				exit_cleanup(-10);
			}
			
			// Read through the bytes
			int i = 1;
			for(; i < n; i+=2){
				if(debug > 0) log_message("<DEBUG> Processing bytes...");
			
				// If for some reason we get an EOF, quit.
				if(buf[i-1] == EOF || buf[i] == EOF) return 0;
				
				// Deskew (whiten) the data by reading two bytes
				result = deskew(&buf[i-1], &buf[i]);
				
				// If whitening threw away the bits, we'll get a -1, if we didn't lets output it.
				if(result >= 0){
					result += '0';
					
					buf_whitened[buf_whitened_pos] = result;
					buf_whitened_pos++;
					
					if(debug > 0){
						char logmessage[200];
						sprintf(logmessage, "<DEBUG> buf_whitened_pos = %d", buf_whitened_pos);
						log_message(logmessage);
					}
					
					// Check if our whitened buffer is full
					if(buf_whitened_pos >= sizeof(buf_whitened)){
						if(debug > 0){
							char logmessage[600];
							sprintf(logmessage, "<DEBUG> Writing %d bytes of whitened data to outfile.", buf_whitened_pos);
							log_message(logmessage);
						}
					
						// If we're scheduled to check the quality of the entropy, do this stuff
						if(test_ent > 0){
							if(debug > 0) log_message("<DEBUG> Caught condition to test entropy.");
							
							// Count the number of 0's and 1's in our sample
							int n = 0;
							average = 0.0;
							num_zeros = 0;
							num_ones = 0;
							for(; n < sizeof(buf_whitened); n++){
								if(buf_whitened[n] == '1'){
									num_ones++;
								}else{
									num_zeros++;
								}
							}
							
							// Calculate our average before moving onto estimating entropy
							average = (float)num_ones / (float)sizeof(buf_whitened);
							
							// Estimate Shannon's entropy
							est_ent = 0.0;
							est_ent -= (1.0 * num_zeros / sizeof(buf_whitened)) * log2f((1.0 * num_zeros / sizeof(buf_whitened)));
							est_ent -= (1.0 * num_ones / sizeof(buf_whitened)) * log2f((1.0 * num_ones / sizeof(buf_whitened)));
							
							// Print our estimation of entropy
							char logmessage[200];
							sprintf(logmessage, "<INFO> Sample average is: %f, Estimated entropy is: %f bits/bit. %d bytes generated in last %d seconds.", average, est_ent, ent_count, ent_est_interval);
							log_message(logmessage);
							
							// Re-set our alarm so we periodically test the quality of our entropy
							alarm(ent_est_interval);
	
							// Reset our state variable
							test_ent = 0;
							ent_count = 0;
						}else{
							int o = 0; 
							for(; o < sizeof(buf_whitened); o += 8){
								if(debug > 0){
									char logmessage[600];
									sprintf(logmessage, "<DEBUG> Stuffing bits into bytes. Byte %d.", (o/8));
									log_message(logmessage);
								}
								
								// Stuff 8 bits together into a byte
								int p = o;
								uint8_t value = 0;
								for(; p < o+8; p++){
									if(buf_whitened[p] == '0'){
										value = (value << 1) | 0;
									}else{
										value = (value << 1) | 1;
									}
								}
								
								if(debug > 0){
									sprintf(logmessage, "<DEBUG> New byte stuffed. Int value %d.", value);
									log_message(logmessage);
								}
								
								ent_count++;
								
								// Write out our byte
								int m = write(outFileHandle, &value, sizeof(value));
								if(m < 0){
									char logmessage[200];
									sprintf(logmessage, "<ERROR> Error writing to output file. Error %d: %s", errno, strerror(errno));
									log_message(logmessage);
									exit_cleanup(-15);
								}
							}
						}
						// Reset our buffer position variable
						buf_whitened_pos = 0;
					}
				}
			}
		}else{
			// Sleep for 250ms (non-blocking wait, lowers CPU time dramatically)
			if(debug > 0) log_message("<DEBUG> Sleeping...");
			usleep(250*1000);
		}
    }
    
	// Clean up and exit
	log_message("<ERROR> Reached end of program. We shouldn't be here...");
	exit_cleanup(0);
}