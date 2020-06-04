#include "./VirtualPinOdroid.h"


  VirtualPin::VirtualPin(bool inverted): _inverted(inverted), _modeVP(INPUT), _valueVP(0) {};
  void VirtualPin::pinMode(uint8_t mode) {_modeVP = mode;};
  void VirtualPin::digitalWrite(uint8_t val) {if (_inverted) val = (LOW==val)?HIGH:LOW;doDigitalWrite(val);};
  int VirtualPin::digitalRead() {int x = doDigitalRead(); if (_inverted) x = (x==LOW)?HIGH:LOW;return x;};
  int VirtualPin::analogRead() {return _valueVP;};
  void VirtualPin::analogWrite(int val) {_valueVP = val;};
  void VirtualPin::togglePin() {digitalWrite(!digitalRead());};
  void VirtualPin::begin() {
	//  Serial.println("VirtualPin.begin(). Hier sollten wir nie landen!!!!!");
	};
  int VirtualPin::doDigitalRead() {int x = _valueVP;if (_inverted) x = (x == LOW)?HIGH:LOW;return x;};
  void VirtualPin::doDigitalWrite(uint8_t x) { _valueVP = x;};



  VirtualGPIO::VirtualGPIO(uint8_t pin, bool inverted, uint8_t pMode) :_pin(pin), VirtualPin(inverted) {
	  ::pinMode(pin, pMode);
  };
  void VirtualGPIO::pinMode(uint8_t mode) {
	  ::pinMode(_pin, mode);
      };
  int VirtualGPIO::analogRead() {
// The following hack is required as ESP8266 will fail if analogRead() is called too often.
// Every 20 msec seems to be safe. 	  
//	    static int lastRead = 0;
//		static uint32_t lastReadTime = 0;
		uint32_t x = millis();
		if (x - _lastReadTime >= 20) {
			_lastReadTime = x;
			_lastRead = ::analogRead(_pin);
		}
		return _lastRead;
	};
  void VirtualGPIO::begin() {
	  
  };

  
  
  void VirtualGPIO::analogWrite(int val) {
#if !defined(ESP32)
	  ::analogWrite(_pin, val);
#endif	  
	  };
  void VirtualGPIO::doDigitalWrite(uint8_t val) {
	  ::digitalWrite(_pin, val);
  };
  int VirtualGPIO::doDigitalRead() {
	  return ::digitalRead(_pin);
  };

  
#ifdef OLD
  
#if defined(ESP32)
#define ANALOGREAD_MAX     4095
#else
#define ANALOGREAD_MAX     1023
#endif
#define ANALOGREAD_DEBOUNCE  3

class VirtualPinAnalog2Digital : public VirtualPin {
public: 
	VirtualPinAnalog2Digital(VirtualPin * pin, int minHigh = ANALOGREAD_MAX / 2, int maxHigh = ANALOGREAD_MAX, uint8_t debounceCount = ANALOGREAD_DEBOUNCE):
		VirtualPin(), _minHigh(minHigh), _maxHigh(maxHigh), _pin(pin), _debounceCount(debounceCount), _lastRead(LOW), _debounceCounter(0) {};
    int analogRead() {return _pin->analogRead();};
protected:
	virtual int doDigitalRead() {
		int x = analogRead();
		if ((x >= _minHigh) && (x <= _maxHigh)) {
			if (_debounceCounter >= _debounceCount)
					return (_lastRead = HIGH);
			++_debounceCounter;
			return _lastRead;
		}
		if (0 == _debounceCounter)
				return (_lastRead = LOW);
		_debounceCounter--;
		return _lastRead;
	};
	int _minHigh, _maxHigh, _debounceCount, _debounceCounter, _lastRead;
	VirtualPin *_pin;
};

#if defined(ESP32)
#define TOUCH_LEVEL 40
#define TOUCH_DEBOUNCE 3
class VirtualTouchPin: public VirtualGPIO {
	VirtualTouchPin(uint8_t pin, bool inverted = false, uint8_t highLevel = TOUCH_LEVEL, uint8_t debounceCount = TOUCH_DEBOUNCE) :
		VirtualGPIO(pin, inverted, INPUT),
		_lastRead(LOW), _debounceCounter(0), _debounceCount(debounceCount), _highLevel(highLevel) {
		};
    int analogRead() {return touchRead(_pin);};
protected:
	virtual int doDigitalRead() {
		int x = analogRead();
		if (x <= _highLevel) {
			if (_debounceCounter >= _debounceCount)
					return (_lastRead = HIGH);
			++_debounceCounter;
			return _lastRead;
		}
		if (0 == _debounceCounter)
				return (_lastRead = LOW);
		_debounceCounter--;
		return _lastRead;
	};
	int _highLevel, _debounceCount, _debounceCounter, _lastRead;
};
#endif

#if defined(ESP8266) || defined(ESP32)
#include <TasmotaHandler.h>
class VirtualTasmotaPin : public VirtualPin , public TasmotaHandlerClass {
public: 
  VirtualTasmotaPin(char *host) : TasmotaHandlerClass(host) {};
  void togglePin() {TasmotaHandlerClass::setPowerState(2);};
protected:
		virtual int doDigitalRead() {return TasmotaHandlerClass::getPowerState()?HIGH:LOW;};
		virtual void doDigitalWrite(uint8_t val) {TasmotaHandlerClass::setPowerState(LOW==val?0:1);};	
};
#endif
#endif  //#ifdef OLD
