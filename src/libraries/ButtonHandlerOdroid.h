#ifndef __BUTTONHANDLERO_H__
#define __BUTTONHANDLERO_H__
#include "./ButtonEventHandlerOdroid.h"
#include "./BasicStatemachineOdroid.h"
#include <Arduino.h>
#include "./VirtualPinOdroid.h"
#if defined(ESP32)
#include <vector>
#endif
#define BUTTON_DEBOUNCE       25
#define BUTTON_2PRESSWINDOW 500
#define BUTTON_FIRSTLONGPRESSTIME 250 
#define BUTTON_LONGPRESSTIME 25

class ButtonHandler : public BasicStatemachine {
public:
  ButtonHandler(VirtualPin* pin, ButtonEventHandler * handler = NULL, 
				uint16_t firstLongpressTime = BUTTON_FIRSTLONGPRESSTIME, uint16_t longpressTime = BUTTON_LONGPRESSTIME, uint16_t debounce = BUTTON_DEBOUNCE);  
  ~ButtonHandler();
  ButtonHandler& addEventHandler(ButtonEventHandler *handler);
  operator bool() {return _switchState;};
  int analogRead() {return _pin->analogRead();}
  ButtonHandler& onToggle(CALLBACK_SIGNATURE_BOOL);
  ButtonHandler& onPressed(CALLBACK_SIGNATURE_VOID);
  ButtonHandler& on2Pressed(CALLBACK_SIGNATURE_VOID);
  ButtonHandler& onLongPressed(CALLBACK_SIGNATURE_UINT16_T);
  ButtonHandler& onLongReleased(CALLBACK_SIGNATURE_VOID);
  ButtonHandler& onLongPressed2(CALLBACK_SIGNATURE_UINT16_T);
  ButtonHandler& onLongReleased2(CALLBACK_SIGNATURE_VOID);
protected:
  void runState(int16_t stateNb);
  void onEnter(int16_t stateNew, int16_t stateOld);
private:
  VirtualPin *_pin;	
  uint16_t _repeatCount;
  uint32_t _lastPressed = 0;
  uint16_t _debounce, _firstLongpressTime, _longpressTime;
  bool _switchState;
  CallbackButtonEventHandler* _myCallbacks;
private:
  std::vector<ButtonEventHandler *> _handlers ; 
};


#ifdef OLD

#define BUTTON_DEBOUNCE       25
#define BUTTON_2PRESSWINDOW 500
#define BUTTON_FIRSTLONGPRESSTIME 250 
#define BUTTON_LONGPRESSTIME  25
#define MAX_HANDLERNUM			   4

class ButtonNotify {
public:
  virtual bool onToggle(bool isOn) {return false;};	
  virtual void onPressed() {};
  virtual void on2Pressed() {};
//  virtual void onLongPressed(bool first) {};			// obsolete. Please use onLonPressed(uint16_t);
  virtual void onLongPressed(uint16_t repeatCount) {};
  virtual void onLongReleased() {};
  virtual void begin() {};
//  virtual void onLongPressed2(bool first) {onLongPressed(first);}; // obsolete. Please use onLonPressed2(uint16_t);
  virtual void onLongPressed2(uint16_t repeatCount) {onLongPressed(repeatCount);};
  virtual void onLongReleased2() {onLongReleased();};

  void setSilence(bool silence) {_silenced = false;};
  bool getSilence() {return _silenced;};
protected:
	bool _silenced;
};



class ButtonHandler : public BasicStatemachine {
public:
  ButtonHandler(uint8_t pin, ButtonNotify * handler, 
				uint16_t firstLongpressTime = BUTTON_FIRSTLONGPRESSTIME, uint16_t longpressTime = BUTTON_LONGPRESSTIME, uint16_t dPressWindow = BUTTON_2PRESSWINDOW);  
  ButtonHandler(uint8_t pin, ButtonNotify * handler, ButtonNotify * handler1, 
				uint16_t firstLongpressTime = BUTTON_FIRSTLONGPRESSTIME, uint16_t longpressTime = BUTTON_LONGPRESSTIME, uint16_t dPressWindow = BUTTON_2PRESSWINDOW);  
  ButtonHandler(uint8_t pin, ButtonNotify * handler, ButtonNotify * handler1, ButtonNotify * handler2, 
				uint16_t firstLongpressTime = BUTTON_FIRSTLONGPRESSTIME, uint16_t longpressTime = BUTTON_LONGPRESSTIME, uint16_t dPressWindow = BUTTON_2PRESSWINDOW);
  ButtonHandler(uint8_t pin, ButtonNotify * handler, ButtonNotify * handler1, ButtonNotify * handler2, ButtonNotify *handler3, 
				uint16_t firstLongpressTime = BUTTON_FIRSTLONGPRESSTIME, uint16_t longpressTime = BUTTON_LONGPRESSTIME, uint16_t dPressWindow = BUTTON_2PRESSWINDOW);
  ButtonHandler(VirtualPin* pin, ButtonNotify * handler, 
				uint16_t firstLongpressTime = BUTTON_FIRSTLONGPRESSTIME, uint16_t longpressTime = BUTTON_LONGPRESSTIME, uint16_t dPressWindow = BUTTON_2PRESSWINDOW);  
  ButtonHandler(VirtualPin* pin, ButtonNotify * handler, ButtonNotify * handler1, 
				uint16_t firstLongpressTime = BUTTON_FIRSTLONGPRESSTIME, uint16_t longpressTime = BUTTON_LONGPRESSTIME, uint16_t dPressWindow = BUTTON_2PRESSWINDOW);  
  ButtonHandler(VirtualPin* pin, ButtonNotify * handler, ButtonNotify * handler1, ButtonNotify * handler2, 
				uint16_t firstLongpressTime = BUTTON_FIRSTLONGPRESSTIME, uint16_t longpressTime = BUTTON_LONGPRESSTIME, uint16_t dPressWindow = BUTTON_2PRESSWINDOW);
  ButtonHandler(VirtualPin* pin, ButtonNotify * handler, ButtonNotify * handler1, ButtonNotify * handler2, ButtonNotify *handler3, 
				uint16_t firstLongpressTime = BUTTON_FIRSTLONGPRESSTIME, uint16_t longpressTime = BUTTON_LONGPRESSTIME, uint16_t dPressWindow = BUTTON_2PRESSWINDOW);

  ~ButtonHandler();
  operator bool() {return _switchState;};
  int analogRead() {return _pin->analogRead();}
protected:
  void runState(int16_t stateNb);
  void onEnter(int16_t stateNew, int16_t stateOld);
private:
//  uint8_t _pin; 
  bool _selfMadePin;
  VirtualPin *_pin;	
  uint8_t _handlers;
  uint16_t _repeatCount;
  uint32_t _lastPressed = 0;
  uint16_t _debounce, _firstLongpressTime, _longpressTime, _2PressWindow;
  bool _switchState;
  ButtonNotify * _handler[MAX_HANDLERNUM];
};

#endif
#endif
