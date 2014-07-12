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