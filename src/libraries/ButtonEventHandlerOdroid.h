#ifndef __BUTTONEVENTHANDLERODROID_H__
#define __BUTTONEVENTHANDLERODROID_H__
#if defined(ESP8266) || defined(ESP32)
#include <functional>
#define CALLBACK_SIGNATURE_VOID std::function<void(void)> callback
#define CALLBACK_SIGNATURE_BOOL std::function<void(bool)> callback
#define CALLBACK_SIGNATURE_UINT16_T std::function<void(uint16_t)> callback
#else
#define CALLBACK_SIGNATURE_VOID void (*callback)(void)
#define CALLBACK_SIGNATURE_BOOL void (*callback)(bool)
#define CALLBACK_SIGNATURE_UINT16_T void (*callback)(uint16_t)
#endif



class ButtonEventHandler {
public:
  virtual bool isActive() {return true;};
  virtual void onToggle(bool isOn) {};	
  virtual void onPressed() {};
  virtual void on2Pressed() {};
  virtual void onLongPressed(uint16_t repeatCount) {};
  virtual void onLongReleased() {};
  virtual void onLongPressed2(uint16_t repeatCount) {onLongPressed(repeatCount);};
  virtual void onLongReleased2() {onLongReleased();};
  virtual void begin() {};
protected:
};

class CallbackButtonEventHandler: public ButtonEventHandler {
public:
  virtual void begin() {};
  virtual void onToggle(bool isOn) {if (_onToggle_cb) _onToggle_cb(isOn);};	
  virtual void onPressed() {if (_onPressed_cb) _onPressed_cb();};
  virtual void on2Pressed() {if (_on2Pressed_cb) _on2Pressed_cb();};
  virtual void onLongPressed(uint16_t repeatCount) {if (_onLongPressed_cb) _onLongPressed_cb(repeatCount);};
  virtual void onLongReleased() {if (_onLongReleased_cb) _onLongReleased_cb();};
  virtual void onLongPressed2(uint16_t repeatCount) {if (_onLongPressed2_cb) _onLongPressed2_cb(repeatCount);};
  virtual void onLongReleased2() {if (_onLongReleased2_cb) _onLongReleased2_cb();};
	void onToggle(CALLBACK_SIGNATURE_BOOL) {_onToggle_cb = callback;};
	void onPressed(CALLBACK_SIGNATURE_VOID) {_onPressed_cb = callback;};
	void on2Pressed(CALLBACK_SIGNATURE_VOID) {_on2Pressed_cb = callback;};
	void onLongPressed(CALLBACK_SIGNATURE_UINT16_T) {_onLongPressed_cb = callback;};
	void onLongReleased(CALLBACK_SIGNATURE_VOID) {_onLongReleased_cb = callback;};
	void onLongPressed2(CALLBACK_SIGNATURE_UINT16_T) {_onLongPressed2_cb = callback;};
	void onLongReleased2(CALLBACK_SIGNATURE_VOID) {_onLongReleased2_cb = callback;};
protected:
#if defined(ESP8266) || defined(ESP32)
	std::function<void(bool)> _onToggle_cb = NULL;
	std::function<void(void)> _onPressed_cb = NULL;
	std::function<void(void)> _on2Pressed_cb = NULL;
	std::function<void(uint16_t)> _onLongPressed_cb = NULL;
	std::function<void(void)> _onLongReleased_cb = NULL;
	std::function<void(uint16_t)> _onLongPressed2_cb = NULL;
	std::function<void(void)> _onLongReleased2_cb = NULL;
#else
	void (*_onToggle_cb)(bool) = NULL;
	void (*_onPressed_cb)(void) = NULL;
	void (*_on2Pressed_cb)(void) = NULL;
	void (*_onLongPressed_cb)(uint16_t) = NULL;
	void (*_onLongReleased_cb)(void) = NULL;
	void (*_onLongPressed2_cb)(uint16_t) = NULL;
	void (*_onLongReleased2_cb)(void) = NULL;
#endif	
};

#endif
