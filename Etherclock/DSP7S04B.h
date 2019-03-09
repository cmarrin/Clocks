/*

Copyright (c) 2016, Embedded Adventures, www.embeddedadventures.com
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
 
- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

- Neither the name of Embedded Adventures nor the names of its contributors
  may be used to endorse or promote products derived from this software
  without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
THE POSSIBILITY OF SUCH DAMAGE.

Contact us at source [at] embeddedadventures.com

*/

//
// A library for accessing the DSP-7204H 7 segment 4 digit display (with colon) from Embedded Adventures
// This version works over i2c
//

#pragma once 

#include <Wire.h>

#define EA_DSP7S04_ADDR_DEFAULT 0x32

#define MODE_ON 		1
#define MODE_OFF 		0
#define CMD_DOT 		0x03
#define CMD_COLON 		0x04
#define CMD_PRINT 		0x01
#define CMD_TEST_MODE 	0x00
#define CMD_RAW 		0x02
#define CMD_CLEAR 		0x05
#define CMD_LEVEL 		0x06

class DSP7S04B
{
public:
	DSP7S04B(uint8_t addr = EA_DSP7S04_ADDR_DEFAULT) : _addr(addr) { }
	
  	void print(const char* str);				// print a string on the display
  	void setRaw(uint8_t pos, uint16_t data);	// set segments directly
  	void setDot(uint8_t pos, bool on);			// turn a dot (0-3) on or off
  	void setColon(bool on);						// turn colon on or off
  	void setTestMode(bool on);					// turn on or off test mode (pass MODE_ON or MODE_OFF)
  	void clearDisplay(void);					// clear entire display
  	void setBrightness(uint8_t level);			// set brightness 0-dull 255-bright

private: 
 	 uint8_t _addr;
 
};
