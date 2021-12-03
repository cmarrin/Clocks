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

#include <mil.h>
#include "DSP7S04B.h"

void DSP7S04B::setDot(uint8_t position, bool on)
{
    Wire.beginTransmission(_addr);
    Wire.write(CMD_DOT);
    Wire.write(position);
    Wire.write(on ? MODE_ON : MODE_OFF);
    Wire.endTransmission();
}

void DSP7S04B::setColon(bool on) {
    Wire.beginTransmission(_addr);
    Wire.write(CMD_COLON);
    Wire.write(on ? MODE_ON : MODE_OFF);
    Wire.endTransmission();

}	

void DSP7S04B::print(const char* str)
{
	mil::cout << "*** print '" << str << "', addr=" << _addr << "\n";
	uint8_t count;

    Wire.beginTransmission(_addr);
    Wire.write(CMD_PRINT);

    for (count = 0; count < 4; count++) {
      if (str[count] == '\0') {
        break;
      }
      Wire.write(str[count]);
    }
    Wire.endTransmission();
}
	
void DSP7S04B::setRaw(uint8_t pos, uint16_t data)
{
    Wire.beginTransmission(_addr);
    Wire.write(CMD_RAW);
    Wire.write(pos);
    Wire.write(data >> 8);
    Wire.write(data & 0xff);
    Wire.endTransmission();
}

void DSP7S04B::setTestMode(bool on)
{
    Wire.beginTransmission(_addr);
    Wire.write(CMD_TEST_MODE);
    Wire.write(on);
    Wire.endTransmission();
}
		
void DSP7S04B::clearDisplay()
{
    Wire.beginTransmission(_addr);
    Wire.write(CMD_CLEAR);
    Wire.endTransmission();
}	

void DSP7S04B::setBrightness(uint8_t level)
{
    Wire.beginTransmission(_addr);
    Wire.write(CMD_LEVEL);
    Wire.write(level);
    Wire.endTransmission();
}
