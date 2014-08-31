K7NVH Random Number Generator Daemon
=======

This daemon will run in the background, collecting data from the RNG device, whiten it, 
and push the data into the kernel entropy pool. Optionally dumping to a file instead.

The daemon will periodically run estimates of the received entropy to help the user 
ensure that quality of the entropy source is being maintained.

k7nvh_rng.c
------------------

Compile the daemon with:
	
	gcc -lm -Wall -O k7nvh_rng.c -o k7nvh_rng
	
After which, you can run the created executable, with the path to the RNG device being 
the first argument.

	sudo ./k7nvh_rng /dev/tty.usbmodemfd1311
	
The software will daemonize, and will create/make use of three files on your system.
	
	/var/log/k7nvh_rng.log
	/tmp/k7nvh_rng.pid
	/tmp/k7nvh_rng_out.ascii
	
These should look pretty familiar, but for clarity:

/var/log/k7nvh_rng.log
	
	This is the log containing all messages from the daemon. Look here for information 
	about what it's doing, and if it's having issues.
	
/tmp/k7nvh_rng.pid

	This is the pid/lock file for the daemon to ensure only one daemon runs at a time. 
	The daemon will check if the file exists at start up, if it does not, it will be 
	created. If it does exist, it will be opened, locked, and have the PID written to it.
	
/tmp/k7nvh_rng_out.ascii

	This is the optional file that will have random data written to it in lieu of the 
	kernel entropy pool.