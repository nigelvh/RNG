K7NVH Random Number Generator Utilities
=======

These utilities are designed to help test various aspects of the K7NVH RNG or other 
software components.

ascii_hex_to_bin.c
------------------

This utility reads a stream of pairs of ASCII hex characters (0-9,A-F) from stdin, and 
prints to stdout the equivalent raw binary data.

	echo -n "4B374E564820524E47" | ./ascii_hex_to_bin

Which should output:

	K7NVH RNG

ascii_to_bin.c
--------------

This utility reads a stream of ASCII binary characters ('1' and '0') from stdin, and 
prints to stdout the equivalent raw binary data.

	echo -n "0100100001001001" | ./ascii_to_bin

Which should output:

	HI

vonneumann.c
-----------

This utility reads a stream of raw binary data from stdin, and 
prints to stdout the debiased binary data.

Whitening is the simple, but effective von Neumann method.

	echo -n "01000001010111010010101010111010" | ./ascii_to_bin | ./vonneumann

amls.c
------

This utility reads a stream of ASCII '1' and '0' chars from stdin, then
AMLS (Advanced Multi-Level Strategy) debiases them, and outputs ASCII '1' and '0' chars
to stdout.

xor512.c
--------

This utility reads a stream of raw binary data from stdin, XOR mixes 
two 512 byte blocks together, and outputs the resulting data to stdout.

distribution.c
--------------

This utility reads a stream of raw binary data from a file provided as the first argument 
to the command, and counts the occurrences of each binary value (from 0 to 255), then 
outputs a space separated list of the count for each bin. This can be used to plot the 
relative occurrence of each binary value.

	./distribution test_raw_binary_file.bin
	
Output from this program is too long to reasonably include here.

read_serial_port.c
------------------

This utility reads from a serial port provided as the first argument to the 
command, and outputs to stdout.

	./read_serial_port /dev/cu.usbmodemfa12111
	
Output from this program is both too long to reasonably include here, and will vary based 
on input from the serial device, but should be composed of only ASCII '1' and '0' 
characters.

ent_pool.c
----------

This utility is designed to test status and pushing data into the kernel entropy pool. 
In the case of OSX hosts, this is accomplished by opening /dev/random, reading a random 
byte, and then writing that byte back into /dev/random. If that completed, then it worked.
In the case of Linux hosts, this is accomplished via the ioctl interface. We read the 
count of the kernel entropy pool (in bits), write a byte into the entropy pool, and 
re-read the count.

Linux:
	./ent_pool 
	Entropy Pool Size: 163
	Entropy Pool Size: 171
OSX:
	./ent_pool 
	Read 1 bytes from /dev/random.
	Wrote 1 bytes to /dev/random.