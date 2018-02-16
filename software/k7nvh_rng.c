/* Compile: gcc -Wall -O k7nvh_rng.c -o k7nvh_rng -lm -std=gnu99 */

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
#ifdef __linux
	#include <linux/random.h>
#endif

// Enable Debug Logging
//#define debug

// Output to file instead of kernel entropy pool
#define file_output

// Main loop sleep time (uSeconds)
// Saves some CPU time instead of pegging a core checking if we've got new serial data
#define SLEEP_TIME 1000

// Define AMLS info
#define amls_array_len 512

// Define some directories
#define pidfile "/tmp/k7nvh_rng.pid"
#define logfile "/var/log/k7nvh_rng.log"
#define outfile "/tmp/k7nvh_rng_out"
#define rndfile "/dev/random"
#define rundir  "/"

// File handles
int pidFileHandle;
int logFileHandle;
#ifdef file_output
int outFileHandle;
#else
int rndFileHandle;
#endif

// Log message working space
char logmessage[1000];

// Buffer and state regarding reading from serial device
char buf [512] = {0};
int buf_pos = 0;
int num_bytes = 0;

// Buffer and state regarding whitening data
unsigned char buf_whitened [1024] = {0};
int buf_whitened_pos = 0;
int buf_whitened_bit = 0;

// Entropy estimation variables
int ent_est_interval = 60;
int test_ent = 0;
int num_ones = 0;
int num_zeros = 0;
float average = 0.0;
float est_ent = 0.0;
int ent_count = 0;

// Close out file handles before exiting
void exit_cleanup(int value){
	// Close file handles before exit
	if(pidFileHandle > 0) close(pidFileHandle);
    if(logFileHandle > 0) close(logFileHandle);
#ifdef file_output
    if(outFileHandle > 0) close(outFileHandle);
#else
    if(rndFileHandle > 0) close(rndFileHandle);
#endif
	
    // Actually exit
    exit(value);
}

// Takes in string to be logged into logFileHandle
void log_message(char *message){
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char timestr[30];
	strftime(timestr, 30, "%F %T", timeinfo);
	
	char logentry[1024];
	sprintf(logentry, "%s K7NVH_RNG[%d]: %s\r\n", timestr, getpid(), message);
	long m = write(logFileHandle, &logentry, strlen(logentry));
	if (m < 0) {
		sprintf(logmessage, "<ERROR> Error writing to log file. Error %d: %s", errno, strerror(errno));
		log_message(logmessage);
		exit_cleanup(-12);
	}
}

// Handles external signals to the process
void signal_handler(int sig){
	signal(sig, signal_handler);

    switch(sig){
    	case SIGALRM:
#ifdef debug
    		log_message("<DEBUG> Received SIGALRM signal.");
#endif
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

// Advanced Multi-Level Strategy based on http://www.eecs.harvard.edu/~michaelm/coinflipext.pdf
// Expects input array of ints containing values of 0 or 1. Maximum length determined by amls_array_len
// Returns pointer to array of ints containing values of 0 or 1, -1 value will indicate end of data
// Returned array pointer must be freed after use
char * amls(char data[], int data_len) {
	// Set up 'result' array
	char *result;
	result = malloc(amls_array_len * sizeof(char));
	memset(result, -1, amls_array_len * sizeof(char));
	int result_pos = 0;
	
	// Set up 'level_1' array
	char *level_1;
	level_1 = malloc(amls_array_len * sizeof(char));
	memset(result, -1, amls_array_len * sizeof(char));
	int level_1_pos = 0;
	
	// Set up 'level_a' array
	char *level_a;
	level_a = malloc(amls_array_len * sizeof(char));
	memset(result, -1, amls_array_len * sizeof(char));
	int level_a_pos = 0;
	
	// Input bits
	char bit_0 = -1;
	
	// Check that we've got at least two bits to compare
	if (data_len < 2){
		free(level_1);
		free(level_a);
		
		return result;
	}
	
	// Process through the input data
	for (int i = 0; i < data_len; i++) {
		if (data[i] != 0 && data[i] != 1) break; // If we encounter a non-0 or 1 bit, stop
		
		if (bit_0 == -1) { // Skip a cycle to grab two bits at a time
			bit_0 = data[i];
		} else { // We've got two bits, do the processing
			if (bit_0 == data[i]) {
				level_a[level_a_pos] = 0;
				level_a_pos++;
				
				level_1[level_1_pos] = bit_0;
				level_1_pos++;
			} else {
				level_a[level_a_pos] = 1;
				level_a_pos++;
				
				result[result_pos] = bit_0;
				result_pos++;
			}
			
			// Reset bit_0
			bit_0 = -1;
		}
	}
	
	// Run AMLS on the level_a and level_1 arrays, then append their results to the main results array
	char *level_a_result = amls(level_a, level_a_pos);
	char *level_1_result = amls(level_1, level_1_pos);
	for (int i = 0; i < amls_array_len; i++) {
		if (level_a_result[i] == -1) break;
		result[result_pos] = level_a_result[i];
		result_pos++;
	}
	for (int i = 0; i < amls_array_len; i++) {
		if (level_1_result[i] == -1) break;
		result[result_pos] = level_1_result[i];
		result_pos++;
	}
	
	// Free the allocated memory that's not used anymore
	free(level_a_result);
	free(level_1_result);
	free(level_a);
	free(level_1);
	
	return result;
}

int main(int argc, char *argv[]) {
    // Check to ensure we've got an argument for the device we want to read from.
	if(argc < 2){
		printf("Please provide the tty device(s) as the argument(s). EX: %s /dev/tty.usbSerial0\r\n", argv[0]);
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
    long o = write(pidFileHandle, str, strlen(str));
	if (o < 0) {
		sprintf(logmessage, "<ERROR> Error writing to PID file. Error %d: %s", errno, strerror(errno));
		log_message(logmessage);
		exit_cleanup(-13);
	}
	
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
	// Try opening the tty device(s), and error if we can't.
	int entFileHandles [argc];
	unsigned long deviceCounters [argc][2]; // [device][0's] and [1's]
	for (int i = 1; i < argc; i++) {
		deviceCounters[i-1][0] = 0;
		deviceCounters[i-1][1] = 0;
		
		entFileHandles[i-1] = open(argv[i], O_RDONLY | O_NONBLOCK | O_NOCTTY);
		if (entFileHandles[i-1] < 0) {
			sprintf(logmessage, "<ERROR> Could not open the tty device, %s. Error %d: %s", argv[i], errno, strerror(errno));
			log_message(logmessage);
			exit_cleanup(-8);
		}
		
		// Read serial interface attributes
		struct termios tty;
		if(tcgetattr(entFileHandles[i-1], &tty) != 0){
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
		if(tcsetattr(entFileHandles[i-1], TCSANOW, &tty) != 0){
			sprintf(logmessage, "<ERROR> Could not set the tty device attributes. Error %d: %s", errno, strerror(errno));
			log_message(logmessage);
			exit_cleanup(-14);
		}
		
		// Assert and then clear DTR
#ifdef __APPLE__
		if(ioctl(entFileHandles[i-1], TIOCSDTR) != 0){
			sprintf(logmessage, "<ERROR> Could not assert DTR. Error %d: %s", errno, strerror(errno));
			log_message(logmessage);
			exit_cleanup(-16);
		}
		if(ioctl(entFileHandles[i-1], TIOCCDTR) != 0){
			sprintf(logmessage, "<ERROR> Could not clear DTR. Error %d: %s", errno, strerror(errno));
			log_message(logmessage);
			exit_cleanup(-17);
		}
#elif __linux
		int setdtr = TIOCM_DTR;
		if(ioctl(entFileHandles[i-1], TIOCMBIC, &setdtr) != 0){
			sprintf(logmessage, "<ERROR> Could not assert DTR. Error %d: %s", errno, strerror(errno));
			log_message(logmessage);
			exit_cleanup(-16);
		}
		if(ioctl(entFileHandles[i-1], TIOCMBIS, &setdtr) != 0){
			sprintf(logmessage, "<ERROR> Could not clear DTR. Error %d: %s", errno, strerror(errno));
			log_message(logmessage);
			exit_cleanup(-17);
		}
#endif
	}
	
	// Temporarily send data to a file rather than the entropy pool
#ifdef file_output
	sprintf(logmessage, "<INFO> Saving generated entropy to %s in lieu of kernel pool.", outfile);
	log_message(logmessage);
			
	outFileHandle = open(outfile, O_RDWR|O_CREAT|O_APPEND, 0644);
	if(outFileHandle < 0){
		sprintf(logmessage, "<ERROR> Could not open output file. Error %d: %s", errno, strerror(errno));
		log_message(logmessage);
		exit_cleanup(-11);
	}
#else
	log_message("<INFO> File output disabled. Saving generated entropy to kernel pool.");
		
	rndFileHandle = open(rndfile, O_RDWR);
	if(rndFileHandle < 0){
		sprintf(logmessage, "<ERROR> Could not open the %s device. Error %d: %s", rndfile, errno, strerror(errno));
		log_message(logmessage);
		exit_cleanup(-18);
	}
#endif

	// Set up an alarm so we periodically test the quality of our entropy
	alarm(ent_est_interval);
	sprintf(logmessage, "<INFO> Next estimation of entropy in %d seconds.", ent_est_interval);
	log_message(logmessage);

    log_message("<INFO> Entropy gathering daemon started.");
    
    // The Big Loop
    while (1) {
#ifdef debug
		log_message("<DEBUG> Main loop...");
#endif
		
		// Check if we're scheduled to print a statistics test
		if (test_ent) {
#ifdef debug
			log_message("<DEBUG> Scheduled entropy stats.");
#endif
			
			// Calculate the average
			average = (float)num_ones / (float)(num_ones + num_zeros);

			// Estimate Shannon's entropy
			// Estimates the information density of the data. https://en.wikipedia.org/wiki/Shannon%27s_source_coding_theorem
			est_ent = 0.0;
			est_ent -= (1.0 * num_zeros / (num_zeros + num_ones)) * log2((1.0 * num_zeros / (num_zeros + num_ones)));
			est_ent -= (1.0 * num_ones / (num_zeros + num_ones)) * log2((1.0 * num_ones / (num_zeros + num_ones)));
			
			// Log our stats
			sprintf(logmessage, "<INFO> %lu debiased bits, Average: %f. Est. Entropy: %f bits/bit. %d bytes generated in last %d seconds.", (long)(num_zeros + num_ones), average, est_ent, ent_count, ent_est_interval);
			log_message(logmessage);
			
			// Per device stats
			for (int i = 0; i < (argc - 1); i++) {
				float biased_average = (float)deviceCounters[i][1] / (float)(deviceCounters[i][1] + deviceCounters[i][0]);
				sprintf(logmessage, "<INFO> %s: %lu bits. Biased average: %f.", argv[i+1], (long)(deviceCounters[i][0] + deviceCounters[i][1]), biased_average);
				log_message(logmessage);
				
				deviceCounters[i][0] = 0;
				deviceCounters[i][1] = 0;
			}
			
			// If we're on linux, we can also read the state of the entropy pool
#ifdef __linux
#ifndef file_output
			int rndcount = 0;
			if( ioctl(rndFileHandle, RNDGETENTCNT, &rndcount) < 0 ){
				sprintf(logmessage, "Could not read the entropy count. Error %d: %s", errno, strerror(errno));
				log_message(logmessage);
				exit_cleanup(-20);
			}
			sprintf(logmessage, "<INFO> Linux kernel entropy pool has %d bits of entropy available.", rndcount);
			log_message(logmessage);
#endif
#endif
			
			// Re-set our alarm so we periodically test the quality of our entropy
			alarm(ent_est_interval);
			
			// Reset our state variables
			test_ent = 0;
			ent_count = 0;
			num_zeros = 0;
			num_ones = 0;
		}
		
		// Read from each rng device
		for (int i = 0; i < (argc - 1); i++) {
			// Reset our variables.
			num_bytes = 0;
			
			// Check how many bytes are available
			ioctl(entFileHandles[i], FIONREAD, &num_bytes);
			
#ifdef debug
			sprintf(logmessage, "<DEBUG> %s bytes available: %d", argv[i+1], num_bytes);
			log_message(logmessage);
#endif
			
			if (num_bytes >= 128) {
				
#ifdef debug
				sprintf(logmessage, "<DEBUG> Reading data from %s.", argv[i+1]);
				log_message(logmessage);
#endif
				
				// Read bytes from the serial port
				long n = read(entFileHandles[i], &buf[buf_pos], sizeof(buf[0]) * 128);
				if (n != 128) {
					sprintf(logmessage, "<ERROR> Error reading from device: %s. Recieved bytes: %ld. Error %d: %s", argv[i+1], n, errno, strerror(errno));
					log_message(logmessage);
					exit_cleanup(-9);
				}
				
#ifdef debug
				log_message("<DEBUG> Converting ASCII '1'/'0' to ints.");
#endif
				
				// Count up our per device bit stats, check for errors, and convert from ASCII to ints
				for (int j = 0; j < 128; j++) {
					if (buf[buf_pos+j] != '0' && buf[buf_pos+j] != '1') {
						sprintf(logmessage, "<ERROR> Invalid input data from %s. Expect '0' and '1' only. Recieved: %d", argv[i+1], buf[buf_pos+j]);
						log_message(logmessage);
						exit_cleanup(-11);
					}
					
					if (buf[buf_pos+j] == '0') {
						deviceCounters[i][0]++;
						buf[buf_pos+j] = 0;
					} else {
						deviceCounters[i][1]++;
						buf[buf_pos+j] = 1;
					}
				}
				
				// Increment our buffer position
				buf_pos += 128;
				
				// If the buffer is full, do some processing
				if (buf_pos >= 512) {

#ifdef debug
					log_message("<DEBUG> AMLS processing.");
#endif
					
					// AMLS process the buf
					char *amls_result = amls(buf, (sizeof(buf) / sizeof(buf[0])));
					
#ifdef debug
					log_message("<DEBUG> Collecting bits into bytes, mixing data, and writing out.");
#endif
					
					// Convert bits from AMLS into bytes
					for (int k = 0; k < (sizeof(buf) / (sizeof(buf[0]))); k++) {
						if (amls_result[k] != 0 && amls_result[k] != 1) break;
						
						// Update our bit counts
						if (amls_result[k] == 0) { num_zeros++; } else { num_ones++; }
						
						buf_whitened[buf_whitened_pos] |= (amls_result[k] << buf_whitened_bit);
						buf_whitened_bit++;
						if (buf_whitened_bit > 7) {
							buf_whitened_bit = 0;
							buf_whitened_pos++;
						}
						if (buf_whitened_pos >= (sizeof(buf_whitened) / sizeof(buf_whitened[0]))) {
							for (int l = 0; l < ((sizeof(buf_whitened) / sizeof(buf_whitened[0])) / 2); l++) {
								// XOR mix the debiased data
								char value = buf_whitened[l] ^ buf_whitened[l+((sizeof(buf_whitened) / sizeof(buf_whitened[0])) / 2)];
								
								// Update the counter
								ent_count++;
								
								// Output the data
#ifdef file_output
								long m = write(outFileHandle, &value, sizeof(value));
								if (m < 0) {
									sprintf(logmessage, "<ERROR> Error writing to output file. Error %d: %s", errno, strerror(errno));
									log_message(logmessage);
									exit_cleanup(-15);
								}
#else
#ifdef __APPLE__
								long m = write(rndFileHandle, &value, sizeof(value));
								if(m < 0){
									sprintf(logmessage, "Error writing to %s device. Error %d: %s", rndfile, errno, strerror(errno));
									log_message(logmessage);
									exit_cleanup(-21);
								}
#elif __linux
								struct {
									int ent_count;
									int size;
									unsigned char data[1024];
								} entropy;
								
								entropy.data[0] = value;
								entropy.size = 1;
								entropy.ent_count = 8;
								
								if( ioctl(rndFileHandle, RNDADDENTROPY, &entropy) < 0){
									sprintf(logmessage, "<ERROR> Could not add entropy to the pool. Error %d: %s", errno, strerror(errno));
									log_message(logmessage);
									exit_cleanup(-19);
								}
#endif
#endif
							}
							
							// Reset some variables
							buf_whitened_pos = 0;
							memset(buf_whitened, 0, (sizeof(buf_whitened) / sizeof(buf_whitened[0])));
						}
					}
					
					// Free the memory AMLS used for this block of data
					free(amls_result);
					
					buf_pos = 0;
				}
			}
		}
		
		usleep(SLEEP_TIME);
    }
}
