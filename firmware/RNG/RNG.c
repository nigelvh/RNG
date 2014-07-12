/* (c) 2014 Nigel Vander Houwen */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#include "Descriptors.h"

#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

unsigned char count = 0;
unsigned char buffer = 0;

char str [4];

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

// Main program entry point.
int main(void){
  // Disable watchdog if enabled by bootloader/fuses
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  // Disable clock division
  clock_prescale_set(clock_div_1);

  // Set the LED pins on Port D as output, and default the LED pins to HIGH (off)
  DDRD = 0b00110011;
  PORTD = 0b00110011;

  // Set Entropy pin to input
  DDRC = DDRC & 0b11111011;

  // Hardware Initialization
  USB_Init();

  // Create a regular character stream for the interface so that it can be used with the stdio.h functions
  CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

  // Enable interrupts
  GlobalInterruptEnable();

  for (;;){
	// Blink the third LED as we gather entropy
	PORTD = PORTD & 0b11101111;
  
  	// Read a bit, and dump it out the USB Serial stream.
	fputc(((PINC & 0b00000100) >> 2) + '0', &USBSerialStream);
	
    // Un-blink the third LED
    PORTD = PORTD | 0b00010000;

    // Must throw away unused bytes from the host, or it will lock up while waiting for the device
    CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

	// Run the LUFA stuff
    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    USB_USBTask();
  }
}

// Event handler for the library USB Connection event.
void EVENT_USB_Device_Connect(void){
  // Turn on the first LED to indicate we're enumerated.
  PORTD = PORTD & 0b11111110;
}

// Event handler for the library USB Disconnection event.
void EVENT_USB_Device_Disconnect(void){
  // Turn off the first LED to indicate we're not enumerated.
  PORTD = PORTD | 0b00000001;
}

// Event handler for the library USB Configuration Changed event.
void EVENT_USB_Device_ConfigurationChanged(void){
  bool ConfigSuccess = true;
  ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

  // Set the second LED to indicate USB is ready or not.
  if(ConfigSuccess){
    PORTD = PORTD & 0b11111101;
  }else{
    PORTD = PORTD | 0b00000010;
  }
}

// Event handler for the library USB Control Request reception event.
void EVENT_USB_Device_ControlRequest(void){
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

