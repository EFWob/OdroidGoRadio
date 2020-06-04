#include <stdint.h>
#include <Arduino.h>
#include "./BasicStatemachineOdroid.h"



BasicStatemachine::BasicStatemachine() {
    resetStateTime();
    onEnter(STATE_INIT_NONE, STATE_INIT_NONE);  
}

bool BasicStatemachine :: setState(int16_t newStateNb) {
bool ret = false;  
  if (!_isHalted)
	if (ret = transitionAllowed(_currentStateNumber, newStateNb))    {
//      if (newStateNb != _currentStateNumber) {
        int16_t oldStateNb = _currentStateNumber;
//        onLeave(_currentStateNumber, newStateNb);
        _currentStateNumber = newStateNb;
        onEnter(_currentStateNumber, oldStateNb);
        resetStateTime();
	} 
//	  else
//		resetStateTime();	
   return ret;
}

void BasicStatemachine::run() {
	if (!_isHalted) 
		runState(_currentStateNumber);
}

StatemachineLooperClass::StatemachineLooperClass() :
	BasicStatemachine()
	{
	}

void StatemachineLooperClass::runState(int16_t stateNb)	{
	for(int i = 0;i < _machines.size();i++)
		_machines[i]->run();		
}


bool StatemachineLooperClass::add(BasicStatemachine * newMachineToLoop) {
	if (newMachineToLoop)
		_machines.push_back(newMachineToLoop);
}

bool StatemachineLooperClass::add(BASIC_LOOP_SIGNATURE) {
	if (loopcallback)
		add(new BasicStatemachineLooper(loopcallback));
}


void StatemachineLooperClass::shutdown() {
	for(int i = 0;i < _machines.size();i++)
		_machines[i]->shutdown();

}

void StatemachineLooperClass::restart() {
	this->shutdown();
#if defined(ESP32) || defined(ESP8266)
	ESP.restart();
#endif
	while (1) ;
}	

BasicStatemachineLooper::BasicStatemachineLooper(BASIC_LOOP_SIGNATURE) {
	this->loopcallback = loopcallback;
}

void BasicStatemachineLooper::runState(int16_t stateNb) {
	if (loopcallback)
		loopcallback();
}


StatemachineLooperClass StatemachineLooper;