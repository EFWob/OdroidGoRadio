#ifndef __BASICSTATEMACHINEO_H__
#define __BASICSTATEMACHINEO_H__
#if defined(ESP32)
#include <vector>
#endif
#include <stdint.h>
#include <Arduino.h>
#define STATE_INIT_NONE         (int16_t)0
#if defined(ESP8266) || defined(ESP32)
#include <functional>
#define BASIC_LOOP_SIGNATURE std::function<void(void)> loopcallback
#else
#define BASIC_LOOP_SIGNATURE void (*loopcallback)(char*, uint8_t*, unsigned int)
#endif
class BasicStatemachine {
  public:
    BasicStatemachine();
//    ~BasicStatemachine() {/*onLeave(_currentStateNumber, STATE_INIT_NONE);*/};
    bool setState(int16_t newState);
	int16_t getState() {return _currentStateNumber;}
//    void loop() {if (!_isHalted) runState(_currentStateNumber);}
    void run();
	void hold() {if (!_isHalted) {onHold(_currentStateNumber);_isHalted = true;}}
    void resume() {if (_isHalted) {_isHalted = false;onResume(_currentStateNumber);}}
	virtual void shutdown() {};
  protected:
    virtual void onEnter(int16_t currentStateNb, int16_t oldStateNb) {};  
//    virtual void onLeave(int16_t currentStateNb, int16_t nextStateNb) {}; // Obsolete-better use onEnter() only
    virtual void onHold(int16_t currentStateNb) {};
    virtual void onResume(int16_t currentStateNb) {};
    virtual void runState(int16_t stateNb) {};
    virtual bool transitionAllowed(int16_t currentStateNb, int16_t nextStateNb) {return currentStateNb != nextStateNb;};
	void resetStateTime(uint32_t offset = 0) {_stateTime = millis() - offset;}
    uint32_t getStateStartTime() {return _stateTime;}
	uint32_t getStateTime() {return millis() - _stateTime;}
  private:  
    int16_t   _currentStateNumber = STATE_INIT_NONE;
    uint32_t _stateTime = 0;
    bool _isHalted = false;	
};

class BasicStatemachineLooper : public BasicStatemachine {
public:
	BasicStatemachineLooper(BASIC_LOOP_SIGNATURE);
protected:
	BASIC_LOOP_SIGNATURE;
	void runState(int16_t stateNb);
};

#define MACHINELOOPER_SIZE		50

class StatemachineLooperClass : public BasicStatemachine {
	public:
		StatemachineLooperClass();
		bool add(BasicStatemachine * newMachineToLoop);
		bool add(BASIC_LOOP_SIGNATURE);

		void shutdown();
		void restart();
	protected:
		    virtual void runState(int16_t stateNb);
	private:
		std::vector<BasicStatemachine *> _machines ;                      		
};



extern StatemachineLooperClass StatemachineLooper;
#endif
