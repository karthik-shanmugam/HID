/*
Copyright (c) 2014-2015 NicoHood
See the readme for credit to other people.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

// Include guard
#pragma once

#include <Arduino.h>
#include "PluggableUSB.h"
#include "HID.h"
#include "HID-Settings.h"

// RawHID might never work with multireports, because of OS problems
// therefore we have to make it a single report with no idea. No other HID device will be supported then.
#undef RAWHID_USAGE_PAGE
#define RAWHID_USAGE_PAGE	0xFFC0 // recommended: 0xFF00 to 0xFFFF

#undef RAWHID_USAGE
#define RAWHID_USAGE		0x0C00 // recommended: 0x0100 to 0xFFFF

// Keep one byte offset for the reportID if used
#if (HID_REPORTID_RAWHID)
#define RAWHID_SIZE (USB_EP_SIZE-1)
#else
#define RAWHID_SIZE (USB_EP_SIZE)
#endif

#undef RAWHID_TX_SIZE
#define RAWHID_TX_SIZE RAWHID_SIZE

#undef RAWHID_RX_SIZE
#define RAWHID_RX_SIZE RAWHID_SIZE

typedef union{
	// a RAWHID_TX_SIZE byte buffer for tx
	uint8_t whole8[];
	uint16_t whole16[];
	uint32_t whole32[];
	uint8_t buff[RAWHID_TX_SIZE];
} HID_RawKeyboardTXReport_Data_t;

typedef union{
	// a RAWHID_TX_SIZE byte buffer for rx
	uint8_t whole8[];
	uint16_t whole16[];
	uint32_t whole32[];
	uint8_t buff[RAWHID_RX_SIZE];
} HID_RawKeyboardRXReport_Data_t;

class RawHID_ : public PluggableUSBModule, public Stream
{
public:
	RawHID_(void);

	void begin(void){
		// empty
	}

	void end(void){
		// empty
	}
	
	virtual int available(void){
		return dataLength;
	}
	
	virtual int read(){
		if(dataLength){
			// Get next data byte
			uint8_t data = *(dataTail - dataLength);
			dataLength--;
			
			// Release buffer if its read fully
			if(!dataLength){
				free(dataHead);
			}

			return data;
		}
		return -1;
	}
	
	virtual int peek(){
		if(dataLength){
			return *(dataTail - dataLength);
		}
		return -1;
	}
	
	virtual void flush(void){
		// Delete all incoming bytes
		if(dataLength){
			free(dataHead);
			dataLength = 0;
		}
	}

	using Print::write;
	virtual size_t write(uint8_t b){
		return write(&b, 1);
	}

	virtual size_t write(uint8_t *buffer, size_t size){
		// TODO this will split the data into proper USB_EP_SIZE packets already
		SendReport(buffer, size);
		return size;
	
		size_t bytesleft = size;
		// First work through the buffer thats already there
		while (bytesleft >= RAWHID_TX_SIZE){
			SendReport(&buffer[size - bytesleft], RAWHID_TX_SIZE);
			bytesleft -= RAWHID_TX_SIZE;
		}
		
		// Write down the leftover bytes and fill with zeros
		if (bytesleft){
			SendReport(&buffer[size - bytesleft], bytesleft);
		}
		
		return size;
	}
	
	void SendReport(void* data, int length);
	
protected:
    // Implementation of the PUSBListNode
    int getInterface(uint8_t* interfaceCount);
    int getDescriptor(USBSetup& setup);
    bool setup(USBSetup& setup);
    
    uint8_t epType[1];
    uint8_t protocol;
    uint8_t idle;
    
	// Buffer pointers to hold the received data
	int dataLength;
	uint8_t* dataHead;
	uint8_t* dataTail;
};
extern RawHID_ RawHID;

