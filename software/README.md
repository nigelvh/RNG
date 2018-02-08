K7NVH Random Number Generator Daemon
=======

This daemon will run in the background, collecting data from the RNG device, whiten it, 
and push the data into the kernel entropy pool. Optionally dumping to a file instead.

The daemon will periodically run estimates of the received entropy to help the user 
ensure that quality of the entropy source is being maintained.

k7nvh_rng.c
------------------

Compile the daemon with:
	
	gcc -Wall -O k7nvh_rng.c -o k7nvh_rng -lm -std=gnu99
	
After which, you can run the created executable, with the path to the RNG device being 
the first argument.

	sudo ./k7nvh_rng /dev/tty.usbmodemfd1311
	
Note that the path to the device will vary based on what type of system the device is 
connected to. You may see it appear as a /dev/ttyACM device on linux distributions or 
/dev/tty.usbmodemfd on OSX based systems. /dev/tty.usbmodemfd1311 is included above as 
an example.
	
The software will daemonize, and will create/make use of three files on your system.
	
	/var/log/k7nvh_rng.log
	/tmp/k7nvh_rng.pid
	/tmp/k7nvh_rng_out
	
These should look pretty familiar, but for clarity:

/var/log/k7nvh_rng.log
	
	This is the log containing all messages from the daemon. Look here for information 
	about what it's doing, and if it's having issues.
	
/tmp/k7nvh_rng.pid

	This is the pid/lock file for the daemon to ensure only one daemon runs at a time. 
	
/tmp/k7nvh_rng_out

	This is the optional file that will have random data written to it in lieu of the 
	kernel entropy pool.

By default, the software is configured to output entropy to the /tmp/k7nvh_rng_out file for 
the user to verify that the captured entropy is of sufficient quality. Use a entropy 
testing software package like ent or dieharder on the resulting file to give things a test.

When you are satisfied that the entropy is of good quality, comment out 
`#define file_output` near the top of the source code to enable pushing entropy into the 
kernel pool rather than the output file.