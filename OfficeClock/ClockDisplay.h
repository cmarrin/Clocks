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

class ClockDisplay
{
public:
	ClockDisplay()
		: _matrix(SS, 4, 1)
	{
		pinMode(SS, OUTPUT);
		digitalWrite(SS, LOW);

		_matrix.setFont(&OfficeClock_8x8_Font8pt);
		_matrix.setTextWrap(false);
    
		_matrix.setIntensity(0); // Use a value between 0 and 15 for brightness

		_matrix.setPosition(0, 0, 0); // The first display is at <0, 0>
		_matrix.setPosition(1, 1, 0); // The second display is at <1, 0>
		_matrix.setPosition(2, 2, 0); // The third display is at <2, 0>
		_matrix.setPosition(3, 3, 0); // And the last display is at <3, 0>

		_matrix.setRotation(0, 1);    // The first display is position upside down
		_matrix.setRotation(1, 1);    // The first display is position upside down
		_matrix.setRotation(2, 1);    // The first display is position upside down
		_matrix.setRotation(3, 1);    // The same hold for the last display

		clear();
	}

	void clear()
	{
		_matrix.fillScreen(LOW);
		_matrix.write(); // Send bitmap to display
	}
  
	void setBrightness(float level)
	{
		level *= 15;
		if (level > 15) {
			level = 15;
		} else if (level < 0) {
			level = 0;
		}
		_matrix.setIntensity(level);
	}
		
	void setString(const String& string, bool colon = false, bool pm = false)
	{
		String s = string;
		if (string.length() >= 2 && colon) {
			s = s.substring(0, 2) + ":" + s.substring(2);
		}
		s.trim();  
    
		// center the string
		int16_t x1, y1;
		uint16_t w, h;
		_matrix.getTextBounds((char*) s.c_str(), 0, 0, &x1, &y1, &w, &h);
    
		_matrix.setCursor((_matrix.width() - w) / 2, _matrix.height() - (h + y1));
		_matrix.fillScreen(LOW);
		_matrix.print(s);
		_matrix.write(); // Send bitmap to display
	}
	
	void scrollString(const String& s, uint32_t scrollRate, std::function<void()> doneCallback);
	
private:	
	void scrollThunk()
	{
		int16_t x1, y1;
		uint16_t w, h;
		_matrix.getTextBounds((char*) _scrollString.c_str(), 0, 0, &x1, &y1, &w, &h);
		if (_scrollOffset++ >= w) {
			_scrollTimer.detach();
			_scrollDoneCallback();
		}

		_matrix.fillScreen(LOW);

		_matrix.setCursor(-_scrollOffset, _matrix.height() - (h + y1));
		_matrix.print(_scrollString);
		_matrix.write(); // Send bitmap to display
	}
	
	static void _scroll(ClockDisplay* self)
	{
		self->scrollThunk();
	}
	
private:
	Max72xxPanel _matrix;
	Ticker _scrollTimer;
	String _scrollString;
	int32_t _scrollOffset;
	std::function<void()> _scrollDoneCallback;
};

void ClockDisplay::scrollString(const String& s, uint32_t scrollRate, std::function<void()> doneCallback)
{
	_scrollString = s;
	_scrollOffset = -_matrix.width(); // Make scroll start offscreen
	_scrollDoneCallback = doneCallback;
	_scrollTimer.attach_ms(scrollRate, _scroll, this);
}
