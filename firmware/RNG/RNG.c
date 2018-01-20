/* (c) 2017 Nigel Vander Houwen */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#include "Descriptors.h"

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

char BYTE_IN = 0;
uint8_t interactive = 0;

volatile char str [128];
volatile uint8_t str_ready = 0;
volatile uint8_t loop_byte = 0;
volatile uint8_t loop_bit = 0;

volatile unsigned int TCNT1_Reset = 64000;

#ifdef __AVR_ATmega16U2__
	void (*bootloader)(void) = 0x1800;
#endif

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */ 
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
		.Config = {
				.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
				.DataINEndpoint           = {
						.Address          = CDC_TX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint = {
						.Address          = CDC_RX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.NotificationEndpoint = {
						.Address          = CDC_NOTIFICATION_EPADDR,
						.Size             = CDC_NOTIFICATION_EPSIZE,
						.Banks            = 1,
					},
			},
	};

// Standard file stream for the CDC interface when set up, so that the virtual CDC COM port can be used like any regular character stream in the C APIs.
static FILE USBSerialStream;

// Sampling Interrupt
ISR(TIMER1_OVF_vect){
  TCNT1 = TCNT1_Reset;
  
  if(loop_bit == 0) str[loop_byte] = 0;
  
  // Grab sample
  uint8_t working = ((PINC & 0b00000100) >> 2);

  //fputc((working + '0'), &USBSerialStream);
  
  str[loop_byte] |= working << (7 - loop_bit);
  
  // Update position
  if(loop_bit < 7){
  	loop_bit++;
  }else{
  	loop_bit = 0;
  	loop_byte++;
  	
  	if(loop_byte > 127){
  	  loop_byte = 0;
  	  str_ready = 1;
  	}
  }
}

// Main program entry point.
int main(void){
  // Disable watchdog if enabled by bootloader/fuses
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  // Disable clock division
  clock_prescale_set(clock_div_1);

  // Set up timer 1
  TCCR1A = 0b00000000; // No pin changes on compare match
  TCCR1B = 0b00000001; // Clock /1
  TCCR1C = 0b00000000; // No forced output compare
  TCNT1 = 0;
  TIMSK1 = 0b00000001; // Enable interrupts on the A compare match

  // Set the LED pins on Port D as output, and default the LED pins to LOW (off)
  DDRD = 0b00110000;
  PORTD = 0b00000000;

  // Set Entropy pin to input
  DDRC = DDRC & 0b11111011;

  // Hardware Initialization
  USB_Init();

  // Create a regular character stream for the interface so that it can be used with the stdio.h functions
  CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

  // Enable interrupts
  GlobalInterruptEnable();

  for (;;){  
	if(str_ready && !interactive){
	  str_ready = 0;
	  //TIMSK1 = 0b00000000; // DEBUG
	  
	  // Blink LED1 as we gather entropy
	  PORTD = PORTD | 0b00010000;

	  for (uint16_t i = 0; i < 128; i++) {
	  	fputc(str[i], &USBSerialStream);
	  }
  	  // Send the string out the USB Serial stream.
	  //fputs(str, &USBSerialStream);

      // Un-blink LED1
  	  PORTD = PORTD & 0b11101111;
  	}

	// Read a byte from the USB serial stream
	BYTE_IN = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

	// USB Serial stream will return <0 if no bytes are available.
	if (BYTE_IN >= 0) {
	  if (BYTE_IN == 30) {
		// Ctrl-^ jump into the bootloader
		bootloader();
		break; // We should never get here...
	  }
	  
	  if (BYTE_IN == '0') { // DEBUG
	  	TIMSK1 = 0b00000001;
	  }
	  
	  // '?' toggles random data output to help with interactive sessions
	  if (BYTE_IN == '?') {
	  	if (interactive) {
	  	  interactive = 0;
	  	}else{
	  	  interactive = 1;
	  	}
	  }
	  
	  // '+' and '-' bump up and down the TCNT1_Reset value, speeding up and down the sampling
	  if (BYTE_IN == '+') {
	  	TCNT1_Reset += 100;
	  	fprintf(&USBSerialStream, "\r\n\r\n%d\r\n\r\n", TCNT1_Reset);
	  }
	  if (BYTE_IN == '-') {
	  	TCNT1_Reset -= 100;
	  	fprintf(&USBSerialStream, "\r\n\r\n%d\r\n\r\n", TCNT1_Reset);
	  }
	}

	// Run the LUFA stuff
    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    USB_USBTask();
  }
}

// Event handler for the library USB Connection event.
void EVENT_USB_Device_Connect(void){
  // Turn on LED2 to indicate we're enumerated.
  PORTD = PORTD | 0b00100000;
}

// Event handler for the library USB Disconnection event.
void EVENT_USB_Device_Disconnect(void){
  // Turn off the first LED to indicate we're not enumerated.
  PORTD = PORTD & 0b11011111;
}

// Event handler for the library USB Configuration Changed event.
void EVENT_USB_Device_ConfigurationChanged(void){
  bool ConfigSuccess = true;
  ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

  // Set the second LED to indicate USB is ready or not.
  if(ConfigSuccess){

  }else{

  }
}

// Event handler for the library USB Control Request reception event.
void EVENT_USB_Device_ControlRequest(void){
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}