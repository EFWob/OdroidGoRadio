#ifndef __VirtualPinOdroid_h__
#define __VirtualPinOdroid_h__
#include <Arduino.h>

class VirtualPin {
public:
  VirtualPin(bool inverted = false);
  virtual void pinMode(uint8_t mode);
  void digitalWrite(uint8_t val);
  int digitalRead();
  bool inverted() {return _inverted;}
  virtual int analogRead();
  virtual void analogWrite(int val);
  virtual void togglePin();
  virtual void begin();
protected:
	virtual int doDigitalRead();
	virtual void doDigitalWrite(uint8_t x);
	bool _inverted = false;
	uint8_t _modeVP = INPUT;
	int _valueVP = 0;
};

//#include <VirtualTILTPin.h>

class VirtualGPIO : public VirtualPin {
public:
  VirtualGPIO(uint8_t pin, bool inverted = false, uint8_t pMode = INPUT);
  virtual void pinMode(uint8_t mode);
  int analogRead();
  virtual void begin();
  virtual void analogWrite(int val);
protected:
  virtual void doDigitalWrite(uint8_t val);
  virtual int doDigitalRead();
  uint8_t _pin;	
  uint32_t _lastReadTime = 0;
  int _lastRead = 0;
};

#if defined(ESP32)
#define ANALOGREAD_MAX     4095
#else
#define ANALOGREAD_MAX     1023
#endif
#define ANALOGREAD_DEBOUNCE  3

class VirtualPinAnalog2Digital : public VirtualPin {
public: 
	VirtualPinAnalog2Digital(VirtualPin * pin, int minHigh = ANALOGREAD_MAX / 2, int maxHigh = ANALOGREAD_MAX, uint8_t debounceCount = ANALOGREAD_DEBOUNCE):
		VirtualPin(), _minHigh(minHigh), _maxHigh(maxHigh), _pin(pin), _debounceCount(debounceCount), _lastRead(LOW), _debounceCounter(0) {_inverted = pin->inverted();if (_inverted) _lastRead = HIGH;};
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


#endif