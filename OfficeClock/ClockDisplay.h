/*-------------------------------------------------------------------------
This source file is a part of Office Clock

For the latest info, see http://www.marrin.org/

Copyright (c) 2017, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
      
    - Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
      
    - Neither the name of the <ORGANIZATION> nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.
      
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Ticker.h>
#include <Max72xxPanel.h>
#include "OfficeClock_8x8_Font8pt.h"
#include <time.h>

class ClockDisplay
{
public:
	ClockDisplay();
	
	virtual void scrollDone() = 0;

	void clear();
	void setBrightness(float level);
	void setString(const String& string, bool colon = false, bool pm = false);
	void scrollString(const String& s, uint32_t scrollRate);
	void setTime(uint32_t currentTime);

private:	
	void scroll();
	
	static void _scroll(ClockDisplay* self) { self->scroll(); }
	
private:
	Max72xxPanel _matrix;
	Ticker _scrollTimer;
	String _scrollString;
	int32_t _scrollOffset;
};
