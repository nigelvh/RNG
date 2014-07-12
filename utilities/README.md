=======
K7NVH Random Number Generator Utilities
=======

These utilities are designed to help test various aspects of the K7NVH RNG.

ascii_hex_to_bin.c
__________________

This utility reads a stream of pairs of ASCII hex characters (0-9,A-F) from stdin, and 
prints to stdout the equivalent raw binary data.

	echo -n "4B374E564820524E47" | ./ascii_hex_to_bin

Which should output:

	K7NVH RNG

ascii_to_bin.c
______________

This utility reads a stream of ASCII binary characters ('1' and '0') from stdin, and 
prints to stdout the equivalent raw binary data.

	echo -n "0100100001001001" | ./ascii_to_bin

Which should output:

	HI

deskewing.c
___________

This utility reads a stream of ASCII binary characters ('1' and '0' from stdin, and 
prints to stdout the whitened ASCII binary data.

Whitening is the simple, but effective von Neumann method.

	echo -n "01000001010111010010101010111010" | ./deskewing
	
Which should output: 

	00000111111
	
distribution.c
______________

This utility reads a stream of raw binary data from a file provided as the first argument 
to the command, and counts the occurrences of each binary value (from 0 to 255), then 
outputs a space separated list of the count for each bin. This can be used to plot the 
relative occurrence of each binary value.

	./distribution test_raw_binary_file.bin
	
Output from this program is too long to reasonably include here.

read_serial_port.c
__________________

This utility reads ASCII binary from a serial port provided as the first argument to the 
command, performs whitening on the captured data, and outputs the whitened ASCII binary 
to stdout.

	./read_serial_port /dev/cu.usbmodemfa12111
	
Output from this program is both too long to reasonably include here, and will vary based 
on input from the serial device, but should be composed of only ASCII '1' and '0' 
characters.