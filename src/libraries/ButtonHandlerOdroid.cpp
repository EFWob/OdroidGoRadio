#include "./ButtonHandlerOdroid.h"


#define STATE_BUTTON_OFF                1
#define STATE_BUTTON_QUALIFY_ON         2
#define STATE_BUTTON_ON                 3
#define STATE_BUTTON_LONGON             4
#define STATE_BUTTON_OFF1				5
#define STATE_BUTTON_ON2				6
#define STATE_BUTTON_LONGON2			7

ButtonHandler::ButtonHandler(VirtualPin *pin, ButtonEventHandler * handler,
				uint16_t firstLongpressTime, uint16_t longpressTime, uint16_t debounce):
  BasicStatemachine(),
  _pin(pin),
  _lastPressed(0),
  _switchState(false),
  _debounce(debounce),
  _firstLongpressTime(firstLongpressTime),
  _longpressTime(longpressTime)
   {
  _myCallbacks = new CallbackButtonEventHandler();
   addEventHandler(_myCallbacks); 
   addEventHandler(handler); 
	StatemachineLooper.add(this);
   }

ButtonHandler& ButtonHandler::addEventHandler(ButtonEventHandler *handler) {   
	if (handler)
		_handlers.push_back(handler);
	return *this;
}

ButtonHandler::~ButtonHandler() {
	delete _myCallbacks;
}

ButtonHandler& ButtonHandler::onToggle(CALLBACK_SIGNATURE_BOOL) {_myCallbacks->onToggle(callback);return *this;};
ButtonHandler& ButtonHandler::onPressed(CALLBACK_SIGNATURE_VOID) {_myCallbacks->onPressed(callback);return *this;};
ButtonHandler& ButtonHandler::on2Pressed(CALLBACK_SIGNATURE_VOID) {_myCallbacks->on2Pressed(callback);return *this;};
ButtonHandler& ButtonHandler::onLongPressed(CALLBACK_SIGNATURE_UINT16_T) {_myCallbacks->onLongPressed(callback);return *this;};
ButtonHandler& ButtonHandler::onLongReleased(CALLBACK_SIGNATURE_VOID) {_myCallbacks->onLongReleased(callback);return *this;};
ButtonHandler& ButtonHandler::onLongPressed2(CALLBACK_SIGNATURE_UINT16_T) {_myCallbacks->onLongPressed2(callback);return *this;};
ButtonHandler& ButtonHandler::onLongReleased2(CALLBACK_SIGNATURE_VOID) {_myCallbacks->onLongReleased2(callback);return *this;};  

void ButtonHandler::onEnter(int16_t stateNew, int16_t stateOld) {
	_debounce = BUTTON_DEBOUNCE;
}

void ButtonHandler::runState(int16_t state) 
   {
    uint32_t x;
    int any = 0;
      switch(state)
        {
          case STATE_INIT_NONE:
//			Serial.println("Calling Handlers.begin (SX1509 init sollte folgen!");
			for(int i = 0;i < _handlers.size();i++)
				_handlers[i]->begin();
		    setState(STATE_BUTTON_OFF);
			break;
          case STATE_BUTTON_OFF:
              if (LOW == (_pin->digitalRead()))
                {
                  setState(STATE_BUTTON_QUALIFY_ON);
                }
            break;
          case STATE_BUTTON_QUALIFY_ON:
            if (HIGH == (_pin->digitalRead()))
              {
                setState(STATE_BUTTON_OFF);
              }
            else if ((x = getStateTime())> 0) 
			  {
              if (x > _debounce)
                x = _debounce;
              if (0 == (_debounce = _debounce - x))
			   { 
				_switchState = true;	
//				Serial.println("Button switch true!");
				for(int i = _handlers.size();i > 0;i--)  
					if (_handlers[i-1]->isActive()) 
						(_handlers[i-1]->onToggle(true));
                setState(STATE_BUTTON_ON); 
			   }
              resetStateTime();               
              }
            break;      
          case STATE_BUTTON_ON:
            if (getStateTime() > BUTTON_DEBOUNCE)
              {
              if (HIGH == (_pin->digitalRead())) 
                {
                  if (0 == _debounce--) 
                    {
					setState(STATE_BUTTON_OFF1);
                    }
                }
              else 
                {
				  _debounce = BUTTON_DEBOUNCE;    	
                  if (_firstLongpressTime < getStateTime()) 
                    {
					_repeatCount = 0;	
					for(int i = 0;i < _handlers.size();i++)  
						if (_handlers[i]->isActive()) {
							_handlers[i]->onLongPressed(_repeatCount);
						}
                    setState(STATE_BUTTON_LONGON);
                    }   
                }
			  }	
            break;
          case STATE_BUTTON_LONGON:
            if (HIGH == (_pin->digitalRead())) 
              {
                if (0 == _debounce--) 
                  {
					_switchState = false;  

					for(int i = _handlers.size();i > 0;i--)  
						if (_handlers[i-1]->isActive()) {
							_handlers[i-1]->onLongReleased();
							_handlers[i-1]->onToggle(false);
						}									
                    setState(STATE_BUTTON_OFF);        
                  }
              }
            else
              {					
              _debounce = BUTTON_DEBOUNCE;                
                if (getStateTime() > _longpressTime)
				  {	
				  resetStateTime();
				  ++_repeatCount;
				for(int i = 0;i < _handlers.size();i++)  
					if (_handlers[i]->isActive()) {
						_handlers[i]->onLongPressed(_repeatCount);
					}							
                  }
			  }	  
            break;
        case STATE_BUTTON_OFF1:
			if (_debounce < getStateTime()) {
              if (LOW == (_pin->digitalRead())) {
                  setState(STATE_BUTTON_ON2);
                }
			  else if (getStateTime() > _firstLongpressTime / 2) {
					setState(STATE_BUTTON_OFF);
					_switchState = false;	
					for(int i = _handlers.size();i > 0;i--)  
						if (_handlers[i-1]->isActive()) {
							_handlers[i-1]->onPressed();
							_handlers[i-1]->onToggle(false);
						}							
				}
			  }
            break;
        case STATE_BUTTON_ON2:
			if (_debounce < getStateTime()) {
              if (HIGH == (_pin->digitalRead())) {
					_switchState = false;	
					for(int i =  _handlers.size();i > 0;i--)  
						if (_handlers[i-1]->isActive()) {
							_handlers[i-1]->on2Pressed();
							_handlers[i-1]->onToggle(false);
						}
					setState(STATE_BUTTON_OFF);
                }
			  else if (getStateTime() > _firstLongpressTime) {
				    _repeatCount = 0;
					for(int i = 0;i < _handlers.size();i++)  
						if (_handlers[i]->isActive()) {
							_handlers[i]->onLongPressed2(_repeatCount);
						}
                    setState(STATE_BUTTON_LONGON2);   
				}
			} //if(debounce)	
            break;
          case STATE_BUTTON_LONGON2:
            if (HIGH == (_pin->digitalRead())) 
              {
                if (0 == _debounce--) 
                  {
					_switchState = false;  
					for(int i =  _handlers.size();i > 0;i--)  
						if (_handlers[i-1]->isActive()) {
							_handlers[i-1]->onLongReleased2();
							_handlers[i-1]->onToggle(false);
						}
                    setState(STATE_BUTTON_OFF);        
                  }
              }
            else
              {					
              _debounce = BUTTON_DEBOUNCE;                
                if (getStateTime() > _longpressTime)
				  {	
				  resetStateTime();
				  ++_repeatCount;
				  for(int i = 0;i < _handlers.size();i++)  
					if (_handlers[i]->isActive()) {
						_handlers[i]->onLongPressed2(_repeatCount);
					}							
                  }
			  }	  
            break;
			
        }
}

