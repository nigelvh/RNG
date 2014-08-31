K7NVH Random Number Generator
=======

The K7NVH Random Number Generator (https://digitalnigel.com/wordpress/?p=1892) is a 
device designed in a small USB stick format to generate true, quantum random numbers 
and make that data available to the linux kernel entropy pool to supplement the built in
PRNG (Pseudo-Random Number Generator).

The randomness source is a reverse biased PN junction in a NPN transistor. The resulting 
noise is amplified and sampled to create a stream of binary data.

The device is available to the host system as a USB serial device, which when accessed 
transfers an endless (but at this point, biased) stream of ASCII '1' and '0' characters. 

The stream of ASCII characters is then processed to whiten them, and converted to raw 
binary data, which can be fed into the kernel entropy pool.

firmware
--------

This is the source code to be compiled by AVR-GCC for installation on the device itself.
The device should already have this installed, and is only needed if you want to make 
modifications.

software
--------

This is the source code to be compiled on your linux or OSX system for the entropy 
processing daemon that will read from the RNG device, whiten the received entropy, and by 
default, push the data into the kernel entropy pool for use by other applications.

utilities
---------

This is the source code for a number of utilities and test programs created to flesh out 
ideas or test functioning of the RNG device.