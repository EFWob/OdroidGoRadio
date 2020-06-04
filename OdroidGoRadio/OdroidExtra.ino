#include "src/libraries/BasicStatemachineOdroid.h"
#include "src/libraries/VirtualPinOdroid.h"
#include "src/libraries/ButtonHandlerOdroid.h"
#include <odroid_go.h>
#include "OdroidExtra.h"

extern char*       dbgprint( const char* format, ... ) ;
extern bool time_req;
extern String nvsgetstr ( const char* key );

class RadioMenu;
bool nvssetOdroid();





class RadioStatemachine: public BasicStatemachine {
  public:
    RadioStatemachine();
    void resetStateTime();
    void startMenu(uint8_t idx);
    void menuButton(uint8_t button, uint16_t repeats);
    uint32_t getStateTime();
    RadioMenu *_menu[4];

  protected:
    RadioMenu *_currentMenu;
    void onEnter(int16_t currentStatenb, int16_t oldStatenb);
    void setBacklight(int16_t stateNb);
    void runState(int16_t stateNb);
} radioStatemachine;

#define PRESET_SHOWTIME          (1000ul * (uint32_t)odroidRadioConfig.showtime.preset) 
#define VOLUME_SHOWTIME          (1000ul * (uint32_t)odroidRadioConfig.showtime.volume + 550)
#define MENU_SHOWTIME             (1000ul * (uint32_t)odroidRadioConfig.showtime.menu)
#define LIST_SHOWTIME             (1000ul * (uint32_t)odroidRadioConfig.showtime.chanList)
#define BRIGHTNESS_SHOWTIME       (1000ul * (uint32_t)odroidRadioConfig.showtime.brightness)
#define BRIGHTNESS_RUNUPTIME      (50ul * (uint32_t)odroidRadioConfig.showtime.brightnessUp)
#define FAVORITE_GROUPS           2       // 2 Groups, linked to Buttons (A) (B)
#define FAVORITE_BANKS            (odroidRadioConfig.misc.favBanks)       // with 3 Banks (of 4 Favorites each) 
#define KEY_SPEED                 (5 - odroidRadioConfig.misc.keySpeed * 2)


#define MENU_A                    0     // A pressed while in menu-state
#define MENU_B                    1     // B pressed in Menu state
#define MENU_UP                   2     // UP pressed in Menu state
#define MENU_DN                   3     // Down pressed in Menu state
#define MENU_LEFT                 4     // Left
#define MENU_RIGHT                5     // Right

struct {
  struct {
    int16_t min = 50;
    int16_t max = 100;
    int16_t start = 80;
    int16_t useStart = 1;
    int16_t life = 0;
  } volume;
  struct {
    int16_t preset = 3;
    int16_t volume = 1;
    int16_t chanList = 15;
    int16_t menu = 15;
    int16_t brightness = 4;
    int16_t brightnessUp = 5; 
  } showtime;

  struct {
    int16_t time = 6;           // 6 * 5 = 30 min
    int16_t volume = 80;        // initial volume
    int16_t offAfter1Min = 1;   // display off after 1 minute idle time in sleep mode?
  } sleep;
  struct {
    int16_t max = 255;            // maximum Backlight value
    int16_t min = 4;              // minimum backlight value
  } backlight;

  struct {
    int16_t closeListOnSelect = 1;
    int16_t favBanks = 3;
    int16_t ignoreSD = 1;
    int16_t keySpeed = 1;
    int16_t debug = 1;
  } misc;
  struct {
    int16_t preset = 1;
  } start;
} odroidRadioConfig;



bool ignoreSD() {
  dbgprint("IgnoreSD-Flag: %d", odroidRadioConfig.misc.ignoreSD);
  return odroidRadioConfig.misc.ignoreSD;
}




int favoriteIndex[FAVORITE_GROUPS];
int favoriteGroup;
//uint32_t favoriteShowtime;

VirtualGPIO playX(34, true);
VirtualGPIO playY(35, true);


VirtualPinAnalog2Digital xLeft(&playX, 1700, 1900);
VirtualPinAnalog2Digital xRight(&playX, 4000, 4095);
VirtualPinAnalog2Digital yUp(&playY, 1700, 1900);
VirtualPinAnalog2Digital yDown(&playY, 4000, 4095);

ButtonHandler volUp(&xRight, NULL, 50, 30);
ButtonHandler volDn(&xLeft, NULL, 50, 30);
ButtonHandler prevChannel(&yUp, NULL, 50, 100);
ButtonHandler nextChannel(&yDown, NULL, 50, 100);

ButtonHandler btnStart(new VirtualGPIO(39, false, INPUT_PULLUP), NULL, 700, 50000);
ButtonHandler btnSelect(new VirtualGPIO(27, false, INPUT_PULLUP), NULL, 700, 50000);
ButtonHandler btnVol(new VirtualGPIO(0, false, INPUT_PULLUP), NULL, 700, 50000);
ButtonHandler btnMenu(new VirtualGPIO(13, false, INPUT_PULLUP), NULL, 700, 50000);
ButtonHandler btnA(new VirtualGPIO(32, false, INPUT_PULLUP), NULL, 50, 1000);
ButtonHandler btnB(new VirtualGPIO(33, false, INPUT_PULLUP), NULL, 50, 1000);
bool presetPresent[100];

String readStationfrompref(int fav) {
  String str = readhostfrompref(fav);
  int idx;
  char s[100];
  if ((idx = str.indexOf( "#")) >= 0) {
      str = str.substring(idx + 1);
  }
  str.trim();
  return str;  
}



#define RADIOSTATE_RUN        1
#define RADIOSTATE_VOLUME     2
#define RADIOSTATE_PRESETS    3
#define RADIOSTATE_LIST       4
#define RADIOSTATE_PRESET_SET 5
#define RADIOSTATE_LIST_SET   6
#define RADIOSTATE_MENU       7 
#define RADIOSTATE_MENU_DONE  8 
#define RADIOSTATE_INICONFIG  9
#define RADIOSTATE_INIVS     10 
#define NUM_RADIOSTATES      10

class RadioMenu;

class RadioMenuEntry {
  public:
    RadioMenuEntry(char* txt, int16_t *reference, int16_t min, int16_t max):
          _prev(NULL), _next(NULL), _reference(reference), _txt(txt), _minVal(min), _maxVal(max) {
      if (txt)
        _txt = strdup(txt);
      read();  
    };

    ~RadioMenuEntry() {
      if (_txt)
        free(_txt);
    };
    
   char *text() {
    if (_txt)
      return _txt;
    else
      return "";
   };

   void text(char *txt) {
      if (_txt)
        free (_txt);
      _txt = NULL;
      if (txt)
        _txt = strdup(txt);
   };
   
    virtual void read() {
      if (_reference) {
        _value = *_reference;
        if (_value < _minVal)
          _value = _minVal;
        if (_value > _maxVal)
          _value = _maxVal;
      } else
        _value = 0;
    };

    virtual void save() {
      if (_reference) { 
        *_reference = _value;
//        Serial.printf("Save value %d\r\n", _value);
      }
    };
    
    RadioMenuEntry(char* txt):
          _prev(NULL), _next(NULL), _reference(NULL), _txt(txt), _minVal(0), _maxVal(0) {
      if (txt)
        _txt = strdup(txt);
    };

    virtual int16_t delta(uint16_t repeats) {
        if (repeats < 20) {
          if ((repeats % 5) == 0)
            return 1;
          else
            return 0;
        };
        if (repeats < 40)
          return 1;
        else
          return 1 + (_maxVal - _minVal) / 50;
    };
    
    virtual bool up(uint16_t repeats) {
      bool ret;
      int16_t oldVal = _value;
      if (ret = (_value < _maxVal)) {
        int16_t delta = this->delta(repeats);
//        Serial.printf("DeltaUP: %d\r\n", delta);
        if (((_value + delta) > _maxVal) || ((_value + delta) < _value))
          _value = _maxVal;
        else
          _value = _value + delta;
      }
      return _value != oldVal;
    };

    virtual bool down(uint16_t repeats) {
      bool ret;
      int16_t oldVal = _value;
      if (ret = (_value > _minVal)) {
        int16_t delta = this->delta(repeats);
//        Serial.printf("DeltaDown: %d/r/n", delta);
        if (((_value - delta) < _minVal) || ((_value - delta) > _value))
          _value = _minVal;
        else
          _value = _value - delta;
      }
      return oldVal != _value;
    };


    virtual char *getValueString() {
      if (_reference) {
        sprintf(_valueStr, "%6d", _value);
      } else
        strcpy(_valueStr, "");
      return _valueStr;
    };

    void printValueString() {
      char *s = getValueString();
      tftdata[TFTSEC_MENU_VAL].x = (26 - strlen(s)) * 6;
      tftset(TFTSEC_MENU_VAL, s);
    };
    
    virtual String getMyDisplayString(bool full = true) {
      String ret;
      String valStr = String(getValueString());
      if (_txt)
        ret= String(_txt);
      else
        ret = String("");
      ret = ret + "                            ";
      ret = ret.substring(0, 26 - valStr.length()) + (full?valStr:String(""));
//      Serial.printf("MenuItemDisplayLength: %d\r\n", ret.length());
      return ret;  
    };

    String getDisplayString(int8_t delta=0, bool full=true) {
      String ret = String("");
      RadioMenuEntry *entry = this;
      while ((delta != 0) && (entry != NULL)) {
        if (delta > 0) {
          entry = entry->_next;
          delta--;
        } else {
          entry = entry->_prev;
          delta++;
        } 
      }
      if (entry)
        ret = entry->getMyDisplayString(full);
      return ret;
    };

    int16_t value() {
      return _value;
    };

    void value(uint16_t val) {
      _value = val;
    };

    
  protected:
    char _valueStr[10];
    RadioMenuEntry *_prev;
    RadioMenuEntry *_next;
    int16_t *_reference;
    int16_t _value;
    int16_t _minVal, _maxVal;
    char *_txt;

    friend class RadioMenu;
    friend class RadioMenu2;

};

class RadioMenuEntryKeySpeed : public RadioMenuEntry {
  public:
    RadioMenuEntryKeySpeed() : RadioMenuEntry("JoyPad speed", &odroidRadioConfig.misc.keySpeed, 0, 2) {};
    virtual char *getValueString() {
      char *valueStr[] = {"  Slow", "Medium", "  Fast"};
      _value = _value % 3;
      strcpy(_valueStr, valueStr[_value]);
      return _valueStr;
    };
};

class RadioMenuEntryBool : public RadioMenuEntry {
  public:
    RadioMenuEntryBool(char* txt, int16_t *reference):
          RadioMenuEntry(txt, reference, 0, 1) {};

    void read() {
      _value = 0;
      if (_reference) 
        _value = *_reference;
      if (_value)
        _value = 1;
      else
         _value = 0;
    };


    virtual char *getValueString() {
      if (_reference) {
        sprintf(_valueStr, "%3s", _value?"Yes":"No");
      } else
        strcpy(_valueStr, "");
      return _valueStr;
    };

};

class RadioMenuEntryBriUp : public RadioMenuEntry {
  public:
    RadioMenuEntryBriUp(char* txt, int16_t *reference, int16_t max):
          RadioMenuEntry(txt, reference, 0, max) {};


    virtual char *getValueString() {
      sprintf(_valueStr, "%4d ms", _value * 50);
      return _valueStr;
    };

};


class RadioMenuEntryPreset : public RadioMenuEntry {
  public:
    RadioMenuEntryPreset(int16_t id):
          RadioMenuEntry(NULL, &_selected, 0, 1) {
            char s[50];
            sprintf(s, "%02d ", id);
            strcat(s, readStationfrompref(id).c_str());
            _txt = strdup(s);
            _selected = 0;
            _startId = _id = id;
            };

    virtual char *getValueString() {
      if (_value) {
        strcpy(_valueStr, "MOVE");
      } else
        strcpy(_valueStr, "    ");
      return _valueStr;
    };
    int16_t id() {
      return _id;
    }
    int16_t newId() {
      int16_t ret = _id;
      if (_txt) 
        if (strlen(_txt) > 3) {
          char s[3];
          strncpy(s, _txt, 2);
          s[2] = 0;
          ret = atoi(s);
        }
        return ret;
    };

    virtual String getMyDisplayString(bool full = true) {
      String ret;
      String valStr = String(getValueString());
      char s[20];
      int id;
      strncpy(s, _txt, 2);
      s[2] = 0;
      id = atoi(s);
      if (id >= odroidRadioConfig.misc.favBanks * 8)
        ret = String("  ") + String(_txt);
      else {
          sprintf(s, "%c%c/%c", 'A' + id / (4 * odroidRadioConfig.misc.favBanks), 
                            '1' + (id % (4 * odroidRadioConfig.misc.favBanks)) /4,
                            '1' + (id %4)); 
           ret = String(s) + String(_txt + 2);                      
      }
      ret = ret.substring(0, 26 - valStr.length()) + (full?valStr:String(""));

/*
      if (_txt)
        ret= String(_txt);
      else
        ret = String("");
      ret = ret + "                            ";
      ret = ret.substring(0, 26 - valStr.length()) + (full?valStr:String(""));
//      Serial.printf("MenuItemDisplayLength: %d\r\n", ret.length());
*/
      return ret;  
    };

    
  protected:
    int16_t _selected = 0;
    int16_t _startId = 0;
    int16_t _id = 0;
};



class RadioMenuEntrySleepTime : public RadioMenuEntry {
public:
  RadioMenuEntrySleepTime(char *txt, int16_t *ref, int16_t min, int16_t max):RadioMenuEntry(txt, ref, min, max) {
  };
    virtual char *getValueString() {
      _value *= 5;
      char *s = RadioMenuEntry::getValueString();
      _value /= 5;
      return s;
    };
  };
  
class RadioMenuEntryVolume : public RadioMenuEntry {
  public:
    RadioMenuEntryVolume(char *txt, int16_t *ref) : RadioMenuEntry(txt, &_value/*ref*/, odroidRadioConfig.volume.min, odroidRadioConfig.volume.max) {
    };
    virtual void read() {
      _minVal = odroidRadioConfig.volume.min;
      _maxVal = odroidRadioConfig.volume.max;
      _value = ini_block.reqvol;
      RadioMenuEntry::read();

    };

    virtual bool up(uint16_t repeats) {
      bool ret = RadioMenuEntry::up(repeats);
      if (ret) 
        updateVolume();
      return ret;
    };

    void updateVolume() {
      if (_value != ini_block.reqvol) {
        char s[10];
        sprintf(s,"%d", _value);
        analyzeCmd("Volume", s);
      }
      
    }
    
    };

class RadioMenuEntryStartPreset: public RadioMenuEntry {
  public:
    RadioMenuEntryStartPreset():RadioMenuEntry("Preset at Start", &odroidRadioConfig.start.preset, 0, 4 * FAVORITE_GROUPS * FAVORITE_BANKS) {
    };
    virtual bool up(uint16_t repeats) {
      bool ret = RadioMenuEntry::up(repeats);
      if (ret)
        showStationName();
      return ret;
    };
    virtual bool down(uint16_t repeats) {
      bool ret= RadioMenuEntry::down(repeats);
      if (ret)
        showStationName();
      return ret;
    };

    void showStationName() {
        uint16_t id = _value;
        String str;
        if (id)
          str = String("\nStart with:\n") + readStationfrompref(id-1);
        else
          str = String("\nStart with:\nLast Station (default)");  
        tftset(TFTSEC_LIST_TOP, str);     
    };
    
    virtual char *getValueString() {
      if (!_value) {
        strcpy(_valueStr, " LAST");
      } else {
        int idx = (_value - 1) % (4 * FAVORITE_GROUPS * FAVORITE_BANKS);
        sprintf(_valueStr, "%c%c(%c)", 'A' + (idx / (4 * FAVORITE_BANKS)), '1' + ((idx % (4 * FAVORITE_BANKS)) / 4), '1' + (idx % 4));
      }
      return _valueStr;
      };
  
};


class RadioMenu {
  
  public: 
    RadioMenu(char *txt = NULL):_head(NULL),_firstEntry(NULL), _lastEntry(NULL), _currentEntry(NULL), _wasRead(false), _updated(false) {
        head(txt);
      };

      char* head(char *txt) {
        if (_head)
          free(_head);
        if (!txt) 
          txt = "";
        _head = strdup(txt); 
        return _head;
      }


      
    ~RadioMenu() {
      RadioMenuEntry *entry;
      if (_head)
        free(_head); 
      while (NULL != (entry = _firstEntry)) {
        _firstEntry = _firstEntry->_next;
        delete entry;
      }
    };

    virtual uint8_t getBacklight() {
      return odroidRadioConfig.backlight.max;
    };

   virtual void menuButton(uint8_t button, uint16_t repeats) {
    switch(button) {
      case MENU_UP:
        this->prev();
        break;  
      case MENU_DN:
        this->next();
        break;  
      case MENU_LEFT:
        this->down(repeats);
        break;
      case MENU_RIGHT:
        this->up(repeats);
        break;
      case MENU_A:
        this->save();
          //Intentional fall through!
      case MENU_B:        
        radioStatemachine.setState(RADIOSTATE_MENU_DONE);
      break;
    }
   }
    
    virtual void display() {
//      Serial.println("Display full Menu!");
      if (!_wasRead) {
        read();
        _wasRead = true;
      }
      if (_head) {
        tftset(TFTSEC_MEN_TOP, String(_head) + (_updated?" (updated)":""));
      }
      if (NULL == _currentEntry) {
        if (_currentEntry = _firstEntry)
          if (NULL == _currentEntry->_reference) {
            if (!next(false)) {
              if (_currentEntry->_next)
                _currentEntry = _currentEntry->_next;
              if (_currentEntry->_next)
                _currentEntry = _currentEntry->_next;
            }
          }
      }  
      if (_currentEntry) {
        String str = _currentEntry->getDisplayString(0, false);
        tftset(TFTSEC_LIST_CUR, str);
        _currentEntry->printValueString();
        str = _currentEntry->getDisplayString(-3) + String("\n") + _currentEntry->getDisplayString(-2) + String("\n") + _currentEntry->getDisplayString(-1); 
        tftset(TFTSEC_LIST_TOP, str);
        str = _currentEntry->getDisplayString(1) + String("\n") + _currentEntry->getDisplayString(2);
        tftset(TFTSEC_LIST_BOT, str);

      } else {
//        Serial.println("Showing empty Menu!");
        tftset(TFTSEC_LIST_CUR, "");
        tftset(TFTSEC_LIST_TOP, "");
        tftset(TFTSEC_LIST_BOT, "");
      }
    };

    virtual void read() {
      RadioMenuEntry *entry = _firstEntry;
      while(entry) {
        entry->read();
        entry = entry->_next;
      }
    };

    virtual void save() {
      RadioMenuEntry *entry = _firstEntry;
      while(entry) {
        entry->save();
        entry = entry->_next;
      }
      nvssetOdroid();
    };

    
    virtual void up(uint16_t repeats) {
      if (_currentEntry)
        if (_currentEntry->up(repeats)) {
            if (updateStateChanged()) {
              if (_head) 
                tftset(TFTSEC_MEN_TOP, String(_head) + (_updated?" (updated)":""));
               dbgprint("Menu updated: %d", _updated);  
            }
              

          _currentEntry->printValueString();
        }
    };

    virtual void down(uint16_t repeats) {
      if (_currentEntry)
        if (_currentEntry->down(repeats)) {
            if (updateStateChanged()) {
              if (_head) 
                tftset(TFTSEC_MEN_TOP, String(_head) + (_updated?" (updated)":""));
               dbgprint("Menu updated: %d", _updated);  
            }
            _currentEntry->printValueString();
        }
    };

    bool prev(bool doDisplay = true) {
      bool ret = false;
      RadioMenuEntry *p = _currentEntry;
      if (p)
        p = p->_prev;
      while (p && (NULL == p->_reference))
        p = p->_prev;
      if (ret = (NULL != p)) {
        _currentEntry = p;
        if (doDisplay)
          display();
        }
      return ret;
    };

    bool next(bool doDisplay = true) {
      bool ret = false;
      RadioMenuEntry *p = _currentEntry;
      if (p)
        p = p->_next;
      while (p && (NULL == p->_reference))
        p = p->_next;
      if (ret = (NULL != p)) {
        _currentEntry = p;
        if (doDisplay)
          display();
      }
      return ret;
    };

    
    void addEntry(RadioMenuEntry *entry) {
      if (NULL != (entry->_prev = _lastEntry)) {
        _lastEntry->_next = entry;
        entry->_prev = _lastEntry;
      } else
        _firstEntry = entry;
      _lastEntry = entry;
    }
  protected:
    virtual bool updateStateChanged() {
        bool oldUpdateState = _updated;
        if (_currentEntry)  
          if (_currentEntry->_reference)
            if (*(_currentEntry->_reference) != _currentEntry->_value)
              _updated = true;
            else if (oldUpdateState) {
              _updated = false;
              RadioMenuEntry* p = _firstEntry;
              while (!_updated && p) {
                if (p->_reference)
                  _updated = *(p->_reference) != p->_value;
                p = p->_next;
              }
            }
              
        return oldUpdateState != _updated;
    };
    
    RadioMenuEntry *_firstEntry, *_lastEntry, *_currentEntry;
    char *_head;
    bool _wasRead, _updated;
};

int16_t test1 = 255;
int16_t test2 = -12345;
int16_t test3 = -12345;
int16_t test4 = -12345;

class RadioMenu1: public RadioMenu {
  public:
    RadioMenu1() : RadioMenu("Settings") {
      addEntry(_initPreset = new RadioMenuEntryStartPreset);
//      addEntry(_startVol = new RadioMenuEntry("Start Volume", &odroidRadioConfig.volume.start, 50, 100));
//      addEntry(_minVol = new RadioMenuEntryVolume("Min Volume", &odroidRadioConfig.volume.min));
//      addEntry(_maxVol = new RadioMenuEntryVolume("Max Volume", &odroidRadioConfig.volume.max));

      addEntry(new RadioMenuEntry("--- Volume Settings ------"));
      addEntry(_startVol = new RadioMenuEntry("Volume at Start", &odroidRadioConfig.volume.start, 0, 100));
      addEntry(_minVol = new RadioMenuEntry("Minimum Volume", &odroidRadioConfig.volume.min, 0, 100));
      addEntry(_maxVol = new RadioMenuEntry("Maximum Volume", &odroidRadioConfig.volume.max, 0, 100));
      addEntry(_lifeVol = new RadioMenuEntryBool("Life Volume update", &odroidRadioConfig.volume.life));

      addEntry(new RadioMenuEntry("--- Brightness Settings --"));
      addEntry(_maxBri = new RadioMenuEntry("Brightness normal", &odroidRadioConfig.backlight.max, 1, 255));
      addEntry(new RadioMenuEntry("Brightness down time", &odroidRadioConfig.showtime.brightness, 0, 120));
      addEntry(_minBri = new RadioMenuEntry("Brightness dimmed", &odroidRadioConfig.backlight.min, 0, 255));
      addEntry(new RadioMenuEntryBriUp("Brightness up time", &odroidRadioConfig.showtime.brightnessUp, 10));

      addEntry(new RadioMenuEntry("--- Timeout Settings -----"));
      addEntry(new RadioMenuEntry("Timeout Volume popup", &odroidRadioConfig.showtime.volume,0, 60));
      addEntry(new RadioMenuEntry("Timeout Menu popup", &odroidRadioConfig.showtime.menu,10, 0x7fff));
      addEntry(new RadioMenuEntry("Timeout Channel list", &odroidRadioConfig.showtime.chanList,5, 0x7fff));
      addEntry(new RadioMenuEntry("Timeout Favorites", &odroidRadioConfig.showtime.preset,3, 30));
      
      addEntry(new RadioMenuEntry("--- Misc. Settings -------"));
      addEntry(new RadioMenuEntryBool("ChanSelect closes list", &odroidRadioConfig.misc.closeListOnSelect));
      addEntry(_favBanks = new RadioMenuEntry("Number Fav. Banks", &odroidRadioConfig.misc.favBanks, 1, 9));
      addEntry(new RadioMenuEntryKeySpeed);
      addEntry(new RadioMenuEntryBool("Ignore SD-Card", &odroidRadioConfig.misc.ignoreSD));
      addEntry(_debug = new RadioMenuEntryBool("Show Debug on Serial", &odroidRadioConfig.misc.debug));
      addEntry(new RadioMenuEntry((char *)(String("BTW: IP is ") + ipaddress).c_str()));
    };
    
    uint8_t getBacklight() {
      uint8_t bri = odroidRadioConfig.backlight.max;
//      if (_currentEntry == _firstEntry)
//        return _currentEntry->value();
      if ((_currentEntry == _maxBri) || (_currentEntry == _minBri))
        bri = _currentEntry->value();
      if (!bri)
        bri = 1;
      return bri;
    };
    
   void menuButton(uint8_t button, uint16_t repeats) {
      int16_t vol;
      RadioMenuEntry *lastCurrentEntry = _currentEntry;
      if (MENU_A == button)
        if (DEBUG != _debug->value())
          analyzeCmd("debug", _debug->value()?"1":"0");
        if (odroidRadioConfig.misc.favBanks != _favBanks->value()) {
          char s[20];
          if (favoriteGroup >= (2 * _favBanks->value())) {
            dbgprint("favorite Group from %d-->%d", favoriteGroup, _favBanks->value());
            favoriteGroup = _favBanks->value();
          }
          favoriteIndex[0] = favoriteIndex[1] = (favoriteGroup % _favBanks->value());
          sprintf(s, "Favorites: %c%c", 'A' + (favoriteGroup / _favBanks->value()), '1' + (favoriteGroup % _favBanks->value()));
          dbgprint("set favorites: %s ", s);
          tftset(TFTSEC_FAV_BOT, s); 
        }

      RadioMenu::menuButton(button, repeats);


      if (_currentEntry) {
        if ((_currentEntry == _initPreset) && (lastCurrentEntry != _initPreset)) 
          _initPreset->showStationName();
        if (_lifeVol->value() && ((_currentEntry == _startVol) || (_currentEntry == _minVol) || (_currentEntry == _maxVol)))  
          vol = _currentEntry->value();
        else
          vol = _defaultVolume;
        if (vol != ini_block.reqvol) {
          char s[10];
          sprintf(s, "%d", vol);
          analyzeCmd("Volume", s);
        }
      }
    }

    virtual void read() {
        _defaultVolume = ini_block.reqvol;
        RadioMenu::read();

        if (_minVol)
          _minVol->value(odroidRadioConfig.volume.min);
        if (_maxVol)
          _maxVol->value(odroidRadioConfig.volume.max);
    }

    virtual void display() {
        RadioMenu::display();
        if (_currentEntry == _initPreset)
          _initPreset->showStationName();
    }

    protected: 
      int16_t _defaultVolume;
      RadioMenuEntryStartPreset *_initPreset;
      RadioMenuEntry *_startVol = NULL, *_minVol = NULL, *_maxVol = NULL, *_maxBri, *_minBri, *_favBanks, *_debug;
      RadioMenuEntryBool *_lifeVol;
};

class RadioMenu2: public RadioMenu {
  public:
    RadioMenu2() : RadioMenu("Preset List") {
      for(int i = 0;i < 100;i++)
        addEntry(new RadioMenuEntryPreset(i));
        _updated = false;
    };

    virtual void read() {
        RadioMenuEntryPreset *p = (RadioMenuEntryPreset *)_firstEntry;
        RadioMenu::read();
        while (p && (p->id() != ini_block.newpreset))
          p = (RadioMenuEntryPreset *)p->_next;
        _currentEntry = p;
    }


    void menuButton(uint8_t button, uint16_t repeats) {
      bool special = false;
      if (_saving) {
        if ((_saving > 200) && (MENU_A == button)) {
          dsp_erase();
          analyzeCmd("reset");
        }
        return;
      }
      if (_currentEntry)
        if (_currentEntry->value())
          if ((MENU_UP == button) || (MENU_DN == button))
            special = true;
      if (special) {
        RadioMenuEntry *t = NULL;
        if ((MENU_UP == button) && (_currentEntry->_prev != NULL)) {
          t = _currentEntry->_prev;
          if (NULL == (t->_next = _currentEntry->_next))
            _lastEntry = t;
          else
            t->_next->_prev = t;
          _currentEntry->_next = t;
          if (NULL == (_currentEntry->_prev = t->_prev))
            _firstEntry = _currentEntry;
          else 
            _currentEntry->_prev->_next = _currentEntry;
          t->_prev = _currentEntry;
        } else if ((MENU_DN == button) && (_currentEntry->_next != NULL)) {
          t = _currentEntry->_next;
          if (NULL == (t->_prev = _currentEntry->_prev))
            _firstEntry = t;
          else
            t->_prev->_next = t;
          _currentEntry->_prev = t;
          if (NULL == (_currentEntry->_next = t->_next))
            _lastEntry = _currentEntry;
          else 
            _currentEntry->_next->_prev = _currentEntry;
          t->_next = _currentEntry;
        }
        if (t) {
          char c;
          c = t->_txt[0];
          t->_txt[0] = _currentEntry->_txt[0];
          _currentEntry->_txt[0] = c;
          c = t->_txt[1];
          t->_txt[1] = _currentEntry->_txt[1];
          _currentEntry->_txt[1] = c;
          if (updateStateChanged())
            ;   //do nothing
          display();
        }  
      }
      else {
        if ((MENU_A == button) && _updated)
          _saving = 1;
        else {
          RadioMenu::menuButton(button, repeats);
#ifdef OLD
          if (button == MENU_LEFT) {
            bool updated;
            RadioMenuEntryPreset *p = (RadioMenuEntryPreset *)_currentEntry;
            updated = p->id() != p->newId();
            if (!updated) {
              p = (RadioMenuEntryPreset *)_firstEntry;
              while (!updated && p) {
                updated = p->id() != p->newId();
                p = (RadioMenuEntryPreset *)p->_next;
              }
            }
            if (updated != _updated2) {
              char *s;
              if (_updated2 = updated)
                s = "Preset List (updated)";
              else
                s = "Preset List";
              tftset(TFTSEC_MEN_TOP, head(s));       // set and display Menu Headline
            }
          }
#endif
        }
      }
    }

    uint8_t getBacklight() {
      uint8_t ret = RadioMenu::getBacklight();
      if (_saving || _updated)
        radioStatemachine.resetStateTime();            // avoid automatic Menu timeout.

      if (_saving) {
        static RadioMenuEntryPreset *p = NULL;
        if ((_saving == 1) || (_saving == 101))
          p = (RadioMenuEntryPreset *)_firstEntry;
        if (_saving <= 100) {
          if (_saving == 1) {
            dsp_erase();
            tftset(TFTSEC_LIST_TOP, "");
            tftset(TFTSEC_LIST_CUR, "SAVING!");
            tftset(TFTSEC_LIST_BOT, "");
//            tftset(TFTSEC_MEN_HLP1, "");
//            tftset(TFTSEC_MEN_HLP2, "");
          }
          // do saving step 1;
          if (p->id() != (_saving - 1)) {
            p->text((char *)readhostfrompref(p->id()).c_str());
          }
        } else if (_saving <= 200) {
          // do saving step 2;
          if (p->id() != (_saving - 101)) {
            char key[15];
            sprintf(key, "preset_%02d", _saving - 101);
            dbgprint("Updated %s to: %s (was: preset_%02d)\n", key, p->text(), p->id());
            if (p->id() + 1 == odroidRadioConfig.start.preset) {       // Has station for startup preset be changed?
              odroidRadioConfig.start.preset = _saving - 100;
              nvssetOdroid();
            }
            if (strlen(p->text())) 
              nvssetstr(key, String(p->text()));
            else
              nvs_erase_key(nvshandle, key);
          }
          if (_saving == 200) {
            // NVS writeAll
            nvs_commit(nvshandle);
            tftset(TFTSEC_LIST_TOP, "\nRestart required!");
            tftset(TFTSEC_LIST_CUR, "Press (A) to Restart!");            
          }
        } 
        if (p)
          p = (RadioMenuEntryPreset*)p->_next;
        if (_saving < 201) 
          _saving++; 
        }

        return ret;
        
      }

    uint16_t _saving = 0;
//    bool _updated2 = false;
    protected:
    virtual bool updateStateChanged() {
        bool oldUpdateState = _updated;
        RadioMenuEntryPreset *p = (RadioMenuEntryPreset *)_currentEntry;
        if (p) {  
            _updated = p->id() != p->newId();
            if (!_updated && oldUpdateState) {
              p = (RadioMenuEntryPreset *)_firstEntry;
              while (!_updated && p) {
                if (p->_reference)
                  _updated = p->id() != p->newId();
                p = (RadioMenuEntryPreset *)p->_next;
              }
            }
        }
        return oldUpdateState != _updated;
    };

};

class RadioMenu3: public RadioMenu {
  public:
    RadioMenu3() : RadioMenu() {
    };
};

class RadioSleepMenu: public RadioMenu {
  public:
    RadioSleepMenu() : RadioMenu("SLEEP TIMER") {
      addEntry(_time = new RadioMenuEntrySleepTime("Sleep Time", &odroidRadioConfig.sleep.time, 1, 24));
      addEntry(_volume = new RadioMenuEntryVolume("Start Volume", &odroidRadioConfig.sleep.volume));
      addEntry(_dark = new RadioMenuEntryBool("Dark after 1 min", &odroidRadioConfig.sleep.offAfter1Min));
    };
   
   void menuButton(uint8_t button, uint16_t repeats) {
   static bool ignoreRepeats = false;
      if (repeats)
        if (ignoreRepeats) {
          radioStatemachine.resetStateTime();
//          Serial.printf("Wake SleepMenu ignore repeats Button %d (Repeats: %d)\r\n", button, repeats);
          return;
        }
      ignoreRepeats = false;
      if ((0 == repeats) && (MENU_SHOWTIME < radioStatemachine.getStateTime())) { 
//        Serial.printf("Wake SleepMenu with Button %d (Repeats: %d)\r\n", button, repeats);
        ignoreRepeats = true;
      } else if (MENU_A == button)
        save();
      else {
//        Serial.printf("SleepMenu handle Button %d (Repeats: %d)\r\n", button, repeats);
        bool updated = false;
        int sleepVolume = _volume->value();
        int sleepTime = _time->value();
        RadioMenu::menuButton(button, repeats);
        if (_volume->value() != sleepVolume) {
          updated = true;// analyzeCmd("volume", String(_volume->value()).c_str());
        }
        if (_time->value() != sleepTime) 
          updated = true;
        if (updated) 
          restartSleep(_time->value(), _volume->value());
      }
   };
      void read() {
        RadioMenu::read();
        restartSleep(_time->value(), _volume->value());
        
      };

    uint8_t getBacklight() {
      uint32_t stateTime = radioStatemachine.getStateTime();
      if (_sleepEnd > _sleepStart) {
        bool updated = false;
        int16_t newVolume;
        int16_t newTime;
        uint32_t nowTime = millis();
        if (nowTime > _sleepEnd) {
          newVolume = 0;
          newTime = 0;
        } else {
          newVolume = map(millis(), _sleepStart, _sleepEnd, _volumeStart, odroidRadioConfig.volume.min);
          newTime = 1 + ((_sleepEnd - nowTime) / 300000); 
        }
        if (newVolume != ini_block.reqvol) {
//          Serial.printf("Setting sleep volume to: %d\r\n", newVolume);
          _volume->value(newVolume);
          analyzeCmd("volume", String(newVolume).c_str());
          updated = true;
        }
        if (newTime != _time->value()) {
//          Serial.printf("Setting sleep time to: %d\r\n", newTime * 5);
          _time->value(newTime);
          updated = true;
        }
        if (updated)
          if (stateTime > MENU_SHOWTIME)
            display();
      }
      if (stateTime > 55000) { 
        if ((stateTime > 60000) && _dark->value())
          return 0;
        else {
          uint8_t b;
          if (stateTime >= 59000)
            b = 1;
          else
            b = map(stateTime - 55000, 0, 4000, odroidRadioConfig.backlight.max, 1);
          return b;
        } 
      } else
        return odroidRadioConfig.backlight.max;
    };


   protected:
    virtual bool updateStateChanged() {  
      _updated = false;
      return false; 
    }
    
      void restartSleep(uint32_t sleepTime, int16_t sleepVolume) {
        sleepTime = sleepTime * 5;
        dbgprint("Sleep (Re-)Start: %ld min, (StartVolume %d)\r\n", sleepTime, sleepVolume) ;
        _sleepStart = millis();
        _sleepEnd = _sleepStart + sleepTime * 60000; 
        _volumeStart = sleepVolume; 
      };
      
      RadioMenuEntry *_time;
      RadioMenuEntry *_volume;   
      RadioMenuEntry *_dark;
      uint32_t _sleepStart, _sleepEnd;
      uint8_t _volumeStart;  
};








//#define MAX_TIMEOUT_VOLUME           1000
//#define MAX_TIMEOUT_MEMORY           2500
//#define MAX_TIMEOUT_MEMORY_SET       5000
//#define MAX_TIMEOUT_CHANNEL_SELECT  10000

void handleBtnAB(uint8_t group, bool released = false) {
  char s[100];
  String str;
  int16_t radioState = radioStatemachine.getState();
  if (released) {
    if (RADIOSTATE_MENU_DONE == radioState)
      radioStatemachine.setState(RADIOSTATE_RUN);
    return; 
  }
  if ((RADIOSTATE_MENU == radioState) || (RADIOSTATE_MENU_DONE == radioState)) {
    if (RADIOSTATE_MENU == radioState)
      radioStatemachine.menuButton(group?MENU_B:MENU_A,0);
    return;
  }
  if ((RADIOSTATE_INIVS == radioState) || (RADIOSTATE_INICONFIG == radioState)) {
    if (RADIOSTATE_INIVS == radioState) {
      dsp_erase();
      radioStatemachine.setState(RADIOSTATE_MENU_DONE);
      tftset(TFTSEC_LIST_CLR, "");
      tftdata[TFTSEC_MEN_TOP].color = TFT_WHITE;
    }
    return;
  }
  if (group > 1)
    return;
  if (RADIOSTATE_LIST == radioState)
    return;
  if ((RADIOSTATE_PRESETS != radioState) && (RADIOSTATE_PRESET_SET != radioState)) {
      if ((favoriteGroup / FAVORITE_BANKS) != group)
        favoriteIndex[group] = 0;    
  }else {
      if ((favoriteGroup / FAVORITE_BANKS) == group) 
        favoriteIndex[group] = (1 + favoriteIndex[group]) % FAVORITE_BANKS;    
  }
  if (radioState != RADIOSTATE_PRESETS)
    radioStatemachine.setState(RADIOSTATE_PRESETS);
  else
    radioStatemachine.resetStateTime();
  favoriteGroup = group * FAVORITE_BANKS + favoriteIndex[group]; 
//  Serial.print("Favorite Group: ");Serial.println(favoriteGroup);
  sprintf(s, "Favorites: %c%d", 'A' + group, 1 + favoriteIndex[group]);
  tftset(TFTSEC_FAV_BOT, s);
  sprintf(s, "\nFavorites in %c%d:\n\n", 'A' + group, 1 + favoriteIndex[group]);
  str = String(s);
  char* keyLabels[] = {"<1> ", "<2> ", "<3> ", "<4> "};
  for(int i = 0;i < 4;i++)
    str = str + keyLabels[i] + readStationfrompref(favoriteGroup * 4 + i) + "\n";
  tftset(TFTSEC_FAV, str);
}



void handleBtnMemory(int id) {
int radioState = radioStatemachine.getState();
    if ((0 == id) || (RADIOSTATE_INIVS == radioState) || (RADIOSTATE_INICONFIG == radioState)) 
      return;     
    else if ((RADIOSTATE_MENU == radioState) && (id < 5)) {
        radioStatemachine.startMenu(id);
        return;
      }
      else if (RADIOSTATE_LIST != radioState) {    
        bool newPreset;
        char s[100];
        id = id - 1 + 4 * favoriteGroup;
        String str = readStationfrompref(id); 
        if (newPreset = (id != ini_block.newpreset)) {
          tftset(TFTSEC_TXT, str);
          tftset(TFTSEC_BOT, str);
          sprintf(s, "%d", id);
          analyzeCmd("preset", s); 
        }     
        tftset(TFTSEC_VOL, str);
        if ((RADIOSTATE_PRESETS != radioState) && (RADIOSTATE_PRESET_SET != radioState)) 
          handleBtnAB(favoriteGroup / FAVORITE_BANKS);
        if (RADIOSTATE_PRESET_SET == radioState)
          radioStatemachine.resetStateTime();
        else
          radioStatemachine.setState(RADIOSTATE_PRESET_SET);
      }
}



int getChannelRelativeTo(int channel, int dir) {
int found = 100;
  if ((channel < 0) || (channel > 99))
    return 100;
  if (0 == dir) 
    if (presetPresent[channel])
      return channel;
    else 
      return 100;
  if (dir > 0)
    dir = 1;
  else 
    dir = -1;
  int i = channel + dir;
  for(;(100 == found) && ((i >= 0) && (i < 100)); i = i + dir)
        if (presetPresent[i])
          found = i;
  return found;
}


int handleBtnY(int direction, uint16_t repeats = 0) {
int16_t radioState = radioStatemachine.getState();  
static int channelX;
int channelY;
  if (!direction)
    return channelX;
  if (RADIOSTATE_MENU == radioState) {
    radioStatemachine.menuButton((1 == direction)?MENU_DN:MENU_UP, repeats);
  } else if ((RADIOSTATE_RUN == radioState) || (RADIOSTATE_LIST == radioState) || (RADIOSTATE_PRESETS == radioState) || (RADIOSTATE_PRESET_SET == radioState)
            || (RADIOSTATE_VOLUME == radioState)) {
    if (RADIOSTATE_LIST == radioState) {
      if (0 != (repeats % KEY_SPEED))
        return channelX;
      radioStatemachine.resetStateTime();
      channelY = getChannelRelativeTo(channelX, direction);
    } else {
      channelX = 100;
      channelY = getChannelRelativeTo(ini_block.newpreset, 0);
      radioStatemachine.setState(RADIOSTATE_LIST);
    }   
    if ((channelY != 100) && (channelX != channelY)) {
//      Serial.println(String("New Channel(x): ") + channelY);
      int channel = channelY;
      channelX = channelY;
      String str = readStationfrompref(channelY);
      tftset(TFTSEC_LIST_CUR, str);
      
      if (100 == (channel = getChannelRelativeTo(channelX, -1)))
        str = "";
      else {
        str = readStationfrompref(channel).substring(0, 25);
        if (100 == (channel = getChannelRelativeTo(channel, -1)))
          str = String("\n\n") + str;
        else {
          str = readStationfrompref(channel).substring(0, 25) + "\n" + str;
          if (100 == (channel = getChannelRelativeTo(channel, -1)))
            str = String("\n") + str;
          else
            str = readStationfrompref(channel).substring(0, 25) + "\n" + str;

        }
      }
      tftset(TFTSEC_LIST_TOP, str);

      if (100 == (channel = getChannelRelativeTo(channelX, 1)))
        str = "\n\n";
      else {
        str = readStationfrompref(channel).substring(0,25);
        if (100 != (channel = getChannelRelativeTo(channel, 1)))
          str = str + "\n" + readStationfrompref(channel).substring(0, 25);
      }
      
      tftset(TFTSEC_LIST_BOT, str);
      
    }
  }
  return channelX;
}

void handleBtnX(int dir, bool onLongReleased = false, uint16_t repeats = 0) {
int16_t radioState = radioStatemachine.getState();
    if (onLongReleased) {
      if (RADIOSTATE_LIST_SET == radioState) 
        radioStatemachine.setState(RADIOSTATE_RUN);
      return;
      }
    if ( 0 == dir)
      return;
    if (RADIOSTATE_MENU == radioState) {
      radioStatemachine.menuButton((1 == dir)?MENU_RIGHT:MENU_LEFT, repeats);
    } 
    else if ((radioState == RADIOSTATE_VOLUME) || (radioState == RADIOSTATE_RUN) || (RADIOSTATE_PRESETS == radioState) || (RADIOSTATE_PRESET_SET == radioState)) {
      char s[30];
      if (RADIOSTATE_VOLUME == radioState)
        radioStatemachine.resetStateTime();
      else
        radioStatemachine.setState(RADIOSTATE_VOLUME);
      if (dir > 0) {
        if (ini_block.reqvol >= odroidRadioConfig.volume.max)
          analyzeCmd("volume", String(odroidRadioConfig.volume.max).c_str());
        else  
          analyzeCmd("upvolume", "1");
      } else {
        if (ini_block.reqvol <= odroidRadioConfig.volume.min)
          analyzeCmd("volume", String(odroidRadioConfig.volume.min).c_str());
        else
          analyzeCmd("downvolume", "1");
      }
      sprintf(s, "\n       VOLUME: %3d", ini_block.reqvol);
      tftset(TFTSEC_VOL, s);
      return;
    } else if (RADIOSTATE_LIST == radioState) {
        radioStatemachine.resetStateTime();
        if (1 == dir) {
          int channel =  handleBtnY(0);
          if ((channel != 100) && (repeats == 0)) {
            char s[20];
            String str = readStationfrompref(channel);
            sprintf(s, "%d", channel);
            tftset(TFTSEC_TXT, str);
            tftset(TFTSEC_VOL, str);
            tftset(TFTSEC_BOT, str);
            tftdata[TFTSEC_VOL].hidden = false;
            analyzeCmd("preset", s);
            if (odroidRadioConfig.misc.closeListOnSelect)
              radioStatemachine.setState(RADIOSTATE_LIST_SET);
          }  
        } else {
          tftset(TFTSEC_VOL, "<Cancel>");
          radioStatemachine.setState(RADIOSTATE_LIST_SET);
        }
    }
 
}



uint32_t nvshandleOdroid = 0;

// OdroidConfig NVS-Handling
bool nvsopenOdroid()
{
  if ( ! nvshandleOdroid )                                         // Opened already?
  {
    nvserr = nvs_open ( "ODROCONFIG", NVS_READWRITE, &nvshandleOdroid ) ;  // No, open nvs
    if ( nvserr )
    {
      dbgprint ( "nvs_open failed for OdroidConfig!" ) ;
      return false;
    }
  }
}
esp_err_t nvsclearOdroid()
{
  nvsopenOdroid() ;                                         // Be sure to open nvs
  return nvs_erase_all ( nvshandleOdroid ) ;                // Clear all keys
}


bool nvssetOdroid() {
  esp_err_t  nvserr ;
  dbgprint("nvssetOdroid start");
  nvsopenOdroid();
  if (!nvshandleOdroid)
    return false;
  dbgprint("nvssetOdroid handle valid start");
  nvserr = nvs_set_str(nvshandleOdroid, "version", ODROIDRADIO_VERSION);
  if (!nvserr) {
    nvserr = nvs_set_blob(nvshandleOdroid, "config", &odroidRadioConfig, sizeof(odroidRadioConfig));
    if (!nvserr)
      nvserr = nvs_commit(nvshandleOdroid);
    else
      dbgprint("could not write config blob");
  } else
    dbgprint("could not set version string");
  
  return (0 == nvserr);  
}

bool nvsgetOdroid ()
{
  esp_err_t  nvserr ;
  char versionStr[50];
  size_t len = 50;
  bool ret = true;
  nvsopenOdroid() ;                         // Be sure to open nvs
  if (!nvshandleOdroid)
    return false;
  versionStr[0] = '\0';
  nvserr = nvs_get_str ( nvshandleOdroid, "version", versionStr, &len ) ;
  dbgprint("version string for odroidConfig: %s (err: %d)", versionStr, nvserr);
  if (strcmp(versionStr, ODROIDRADIO_VERSION)) {
      dbgprint ( "wrong or no version number found in nvs for OdroidConfig!" ) ;
      ret = false;
      if (0 == nvserr) 
        nvsclearOdroid(); 
      ret = nvssetOdroid ();
      if (!ret) {
        dbgprint ( "failed to create nvs OdroidConfig!" ) ;
        return false;
      }
  }
  len = sizeof(odroidRadioConfig);
  nvserr == nvs_get_blob(nvshandleOdroid, "config", &odroidRadioConfig, &len);
  return ((len == sizeof(odroidRadioConfig)) && (ESP_OK == nvserr));
}





void odroidSetup() {
    char key[12];
    ledcSetup(0, 5000, 8);
    ledcAttachPin(14, 0);
    ledcWrite(0, 255);
    volUp.onLongPressed([](uint16_t repeats) {handleBtnX(1, false, repeats);}).onLongReleased([]() {handleBtnX(1, true);});
    volDn.onLongPressed([](uint16_t repeats) {handleBtnX(-1, false, repeats);}).onLongReleased([]() {handleBtnX(-1, true);});
    prevChannel.onLongPressed([](uint16_t repeats) {handleBtnY(-1, repeats);});
    nextChannel.onLongPressed([](uint16_t repeats) {handleBtnY(1, repeats);});

    btnStart.onPressed([]() {handleBtnMemory(1);}).onLongPressed([](uint16_t) {radioStatemachine.startMenu(1);});
    btnSelect.onPressed([](){handleBtnMemory(2);}).onLongPressed([](uint16_t) {radioStatemachine.startMenu(2);});
    btnVol.onPressed([]() {handleBtnMemory(3);});
    btnMenu.onPressed([]() {handleBtnMemory(4);}).onLongPressed([](uint16_t) {radioStatemachine.startMenu(4);});
    btnA.onLongPressed([](uint16_t) {handleBtnAB(0);}).onLongReleased([]() {handleBtnAB(0, true);});
    btnB.onLongPressed([](uint16_t) {handleBtnAB(1);}).onLongReleased([]() {handleBtnAB(1, true);});
    for(int i = 0;i < 100;i++) {
      sprintf(key, "preset_%02d", i);
      presetPresent[i] = nvssearch(key);
    }  
    if (nvsgetOdroid())
      dbgprint("Odroid config data from nvs done!");
    dbgprint( "Odroid-Setup done" ) ;
    if (LOW == digitalRead(32)) {
      dsp_erase();
      dsp_setRotation();
      dsp_setTextSize(1);
      dbgprint("Button A pressed");
      dsp_println("\n\nRelease (A) to continue...");
      while (LOW == digitalRead(32))
        delay(10);
      dsp_println("\nNow press (B) to start...");
      while (HIGH == digitalRead(33))
        delay(20);
    }  
}

void odroidRadioFirstLoopSetup() {
char s[50];
int16_t iniVolume;
// setup initial volume
    iniVolume = odroidRadioConfig.volume.useStart?odroidRadioConfig.volume.start:ini_block.reqvol; 
    if (iniVolume >= odroidRadioConfig.volume.max)
      iniVolume = odroidRadioConfig.volume.max;
    if (iniVolume <= odroidRadioConfig.volume.min)
      iniVolume = odroidRadioConfig.volume.min;
    if (iniVolume != ini_block.reqvol) {
        sprintf(s, "%d", iniVolume);
        analyzeCmd("volume", s);
    }
    if (odroidRadioConfig.misc.debug != DEBUG) 
      analyzeCmd("debug", odroidRadioConfig.misc.debug?"1":"0");
    if (odroidRadioConfig.start.preset) {
          uint8_t preset = odroidRadioConfig.start.preset - 1;
          uint8_t group;

                    
          sprintf(s, "%d", preset);
          analyzeCmd("preset", s); 
          group = (preset / (4 * FAVORITE_BANKS)) % FAVORITE_GROUPS;
          favoriteIndex[group] = (preset % (4 * FAVORITE_BANKS)) / 4;
          favoriteGroup = group * FAVORITE_BANKS + favoriteIndex[group]; 
          sprintf(s, "Favorites: %c%d", 'A' + group, 1 + favoriteIndex[group]);
          tftset(TFTSEC_FAV_BOT, s);
          String str = readStationfrompref(preset);
          tftset(TFTSEC_TXT, str);
          tftset(TFTSEC_BOT, str);
        }     
      radioStatemachine._menu[0] = new RadioMenu1;  
      radioStatemachine._menu[1] = new RadioMenu2;
  if (!NetworkFound)
    radioStatemachine.setState(RADIOSTATE_INICONFIG);
  else if (odroidVsError)
    radioStatemachine.setState(RADIOSTATE_INIVS);
  else
    radioStatemachine.resetStateTime();
}

void odroidLoop(void) {    
static bool first = true;
  if (first) {
    odroidRadioFirstLoopSetup();
    first = false;
  } else {
    StatemachineLooper.run();
  }
}


/*
 *  DISPLAY-Section below
 */


uint8_t brightnessLevel = 0xff;
#define BLACK   TFT_BLACK
#define BLUE    TFT_BLUE
#define RED     TFT_RED
#define GREEN   TFT_GREEN
#define CYAN    TFT_CYAN
#define MAGENTA TFT_MAGENTA
#define YELLOW  TFT_YELLOW
#define WHITE   TFT_WHITE

bool displayFlipped = false;

bool dsp_begin()
{
  GO.lcd.begin();
  tft = &GO.lcd;
  GO.lcd.setBrightness(brightnessLevel);
  odroidSetup();
  dsp_setRotation();
  return ( tft != NULL ) ;
}

void dsp_upsideDown()
{
  tft->setRotation (3);               // Needed if Odroid is used with docking station 
  displayFlipped = true;
}


//Implementation of class RadioStatemachine below


  RadioStatemachine::RadioStatemachine() : BasicStatemachine() {
    StatemachineLooper.add(this);
    _menu[0] = NULL; //new RadioMenu1;
    _menu[1] = NULL;
//    new RadioMenu2;
    _menu[2] = new RadioMenu3;
    _menu[3] = new RadioSleepMenu;
    _currentMenu = NULL;
  };
   void RadioStatemachine::resetStateTime() {
     if (getStateTime() > BRIGHTNESS_RUNUPTIME) 
        BasicStatemachine::resetStateTime();
   };

  void RadioStatemachine::startMenu(uint8_t idx) {
    int16_t radioState = getState();
    idx--;
    if ((idx > 3) || (RADIOSTATE_INICONFIG == radioState) || (RADIOSTATE_INIVS == radioState))  
      return;
    if (RADIOSTATE_MENU == radioState) {
      if (_menu[idx] != _currentMenu) {
        _currentMenu = _menu[idx];
 //       _currentMenu->read();
        _currentMenu->display();
      }
      resetStateTime();
    } else {
//      Serial.printf("Start Menu %d\r\n", idx);
      _currentMenu = _menu[idx];
//      _currentMenu->read();
      _currentMenu->display();
//      Serial.println("Menu State will be set.");
      setState(RADIOSTATE_MENU);
    }
  };

   void RadioStatemachine::menuButton(uint8_t button, uint16_t repeats) {
//    Serial.printf("Menu Button %d (repeats: %ld)\r\n", button, repeats);
    if (_currentMenu) {
      if ((MENU_UP == button) || (MENU_DN == button) || (MENU_LEFT == button) || (MENU_RIGHT == button)) {
        if ((repeats % KEY_SPEED) == 0) {
          _currentMenu->menuButton(button, repeats / KEY_SPEED);
          resetStateTime();
          }
      } else {
        _currentMenu->menuButton(button, repeats);
        resetStateTime();
      }
    } else
      resetStateTime();
 };
   
  void RadioStatemachine::onEnter(int16_t currentStatenb, int16_t oldStatenb) {
    const uint32_t screenSections[NUM_RADIOSTATES] = {
     bit(TFTSEC_TOP)|bit(TFTSEC_TXT)|bit(TFTSEC_BOT)|bit(TFTSEC_FAV_BOT),                 // RADIOSTATE_RUN ("normal")
     bit(TFTSEC_TOP)|bit(TFTSEC_TXT)|bit(TFTSEC_VOL)|bit(TFTSEC_FAV_BOT),                 // RADIOSTATE_VOLUME (Volume display after change)
     bit(TFTSEC_FAV_BUT)|bit(TFTSEC_FAV)|bit(TFTSEC_BOT)|bit(TFTSEC_FAV_BOT),             // RADIOSTATE_PRESETS (Show preset Bank)
     bit(TFTSEC_TOP)|bit(TFTSEC_LIST_CLR)|bit(TFTSEC_LIST_HLP1)|bit(TFTSEC_LIST_HLP2)|
     bit(TFTSEC_LIST_CUR)|bit(TFTSEC_LIST_TOP)|bit(TFTSEC_LIST_BOT),                      // RADIOSTATE_LIST (Select station from list)
     bit(TFTSEC_FAV_BUT)|bit(TFTSEC_FAV)|bit(TFTSEC_VOL)|bit(TFTSEC_FAV_BOT),             // RADIOSTATE_PRSET_SET (After select from preset Bank)
     bit(TFTSEC_TOP)|bit(TFTSEC_TXT)|bit(TFTSEC_VOL)|bit(TFTSEC_FAV_BOT),                  // RADIOSTATE_LIST_SET (After selecting from channel list)
     bit(TFTSEC_MEN_TOP)|bit(TFTSEC_LIST_CLR)|bit(TFTSEC_MEN_HLP1)|bit(TFTSEC_MEN_HLP2)|
     bit(TFTSEC_LIST_CUR)|bit(TFTSEC_LIST_TOP)|bit(TFTSEC_LIST_BOT)|bit(TFTSEC_MENU_VAL),                      // RADIOSTATE_MENU (Menu active)
     bit(TFTSEC_TOP)|bit(TFTSEC_TXT)|bit(TFTSEC_BOT)|bit(TFTSEC_FAV_BOT),                 // RADIOSTATE_MENU_DONE (wait to release (A) or (B))
     bit(TFTSEC_MEN_TOP)|bit(TFTSEC_LIST_CLR),                                            // RADIOSTATE_INICONFIG
     bit(TFTSEC_MEN_TOP)|bit(TFTSEC_LIST_CLR)                                             // RADIOSTATE_INIVS
    };

char *radiostateNames[] = {"Start", "Normal operation", "Show Volume", "Show Presets", "Show List", "Preset set", "List Done", "Menu", "Menu Done", "InitConfig", "ErrorVS1053"};
      dbgprint("RadioState: %d (%s)", currentStatenb, radiostateNames[currentStatenb]);
      if ((STATE_INIT_NONE == oldStatenb) && (0 == odroidRadioConfig.start.preset)){
        String str = nvsgetstr ( "preset" );
        dbgprint("Start with last preset: %s\n", str.c_str());

        str = readStationfrompref(str.toInt());
        tftset(TFTSEC_TXT, str);
        tftset(TFTSEC_BOT, str);
      }
    if (STATE_INIT_NONE == currentStatenb)
      currentStatenb = RADIOSTATE_RUN;
    if (RADIOSTATE_MENU != currentStatenb)
      _currentMenu = NULL;  
    if (RADIOSTATE_INICONFIG == currentStatenb) {
      analyzeCmd("stop");
      dsp_erase();
      tftdata[TFTSEC_MEN_TOP].color = TFT_RED;
      tftset(TFTSEC_MEN_TOP, "WiFi failure");
      tftset(TFTSEC_LIST_CLR, "Connect desktop PC to SSID" NAME "/Pw " NAME "Open http://192.198.4.1/  to check your WiFi creden-tials.\n\n"
                        "Press (Default) to load   initial values.\n\n"
                        "Press (Save) and (Restart)when done!");
    }
    if (RADIOSTATE_INIVS == currentStatenb) {
      dsp_erase();
      tftdata[TFTSEC_MEN_TOP].color = TFT_RED;
      tftset(TFTSEC_MEN_TOP, "DAC VS1053 failure");
      tftset(TFTSEC_LIST_CLR, String("Check connection between  Odroid and VS1053 module.\n\n"
                        "Press (A) or (B) to start demomode anyways.\n\nFor more help, open\nhttp://") + ipaddress + String("/startup.html"));
    }
    if (--currentStatenb < NUM_RADIOSTATES) {
      int i;
      uint32_t bm = 1;
      for(int i = 0;i < TFTSECS;i++) {
        if (screenSections[currentStatenb] & bm) {
          if (tftdata[i].hidden) {
            tftdata[i].hidden = false;
            tftdata[i].update_req = true;
          }
        } else
          tftdata[i].hidden = true;
        bm = bm * 2;
      }
    }       
  };

  
  void RadioStatemachine::setBacklight(int16_t stateNb) {
  static uint16_t brightness = odroidRadioConfig.backlight.max;
  uint32_t runTime = getStateTime();
    if (STATE_INIT_NONE == stateNb)
      return;
    if (RADIOSTATE_INICONFIG == stateNb)
      brightness=255;
    else if (stateNb != RADIOSTATE_RUN) {
        if ((runTime >= BRIGHTNESS_RUNUPTIME) || (_currentMenu &&(brightness == odroidRadioConfig.backlight.max))) 
          if (_currentMenu)
            brightness = _currentMenu->getBacklight();
          else
            brightness = odroidRadioConfig.backlight.max;
        else if (brightness != odroidRadioConfig.backlight.max)
          brightness = map(runTime, 0, BRIGHTNESS_RUNUPTIME, odroidRadioConfig.backlight.min,odroidRadioConfig.backlight.max);
    } else {
      if (runTime > BRIGHTNESS_SHOWTIME) {
        if (runTime <= BRIGHTNESS_SHOWTIME + 2500) 
          brightness = map(runTime, BRIGHTNESS_SHOWTIME, BRIGHTNESS_SHOWTIME + 2500, odroidRadioConfig.backlight.max,odroidRadioConfig.backlight.min);
        else
          brightness = odroidRadioConfig.backlight.min;
      } else 
        if (runTime > BRIGHTNESS_RUNUPTIME)  
          brightness = odroidRadioConfig.backlight.max;
        else if (brightness != odroidRadioConfig.backlight.max)
          brightness = map(runTime, 0, BRIGHTNESS_RUNUPTIME, odroidRadioConfig.backlight.min,odroidRadioConfig.backlight.max);
    }
    ledcWrite(0, brightness);    
  }

  uint32_t RadioStatemachine::getStateTime() {
    return BasicStatemachine::getStateTime();
  };
  
  void RadioStatemachine::runState(int16_t stateNb) {
    setBacklight(stateNb);
    switch(stateNb) {
      case STATE_INIT_NONE:
        setState(RADIOSTATE_RUN);
        break;
      case RADIOSTATE_VOLUME:
        if (VOLUME_SHOWTIME < getStateTime())
          setState(RADIOSTATE_RUN);
        break;
      case RADIOSTATE_LIST:
        if (LIST_SHOWTIME < getStateTime())
          setState(RADIOSTATE_RUN);
        break;
      case RADIOSTATE_PRESETS:
      case RADIOSTATE_PRESET_SET:
        if (PRESET_SHOWTIME < getStateTime())
          setState(RADIOSTATE_RUN);
        break;  
      case RADIOSTATE_MENU:
        if (_currentMenu != _menu[3])           // This is SleepTime: can only by cancelled by Keypress!
          if (MENU_SHOWTIME < getStateTime())
            setState(RADIOSTATE_RUN);
        break;  
      case RADIOSTATE_RUN:
        break;
      case RADIOSTATE_INICONFIG:
        // Force stop in irregular (non-playing) mode.
        if (getStateTime() > 2000) {
          if (datamode != STOPPED) 
            datamode = STOPREQD;
          resetStateTime();
        }
        break;      
      default:
        break;
    }
  };


// startup.html file in raw data format for PROGMEM
//
#define about_html_version 170626
const char startup_htmlArr[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
 <head>
  <title>Start the Odroid-Go-Radio</title>
  <meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
 </head>
 <body>
   <h1>Odroid-Go-Radio startup sequence</h1>
  <p>If you get an error concerning failure of VS1053 module, please try the following:<br>
  <ul>
    <li> Check the wiring of the VS1053 module.
    <li> The VS1053 module needs 5V to run, so VSS must be connected to Pin10 of the Odroid-Go header.
    <li> If not powered by USB, the module will not work correctly.
    <li> If powered by USB, the boot of the OdroidGo might be disturbed by the presence of the VS1053 module.
    <li> In that case (wiring is correct, radio does connect to WiFi, but hangs later on/does not play) try the following:
    <ol>
      <li> Switch OdroidGo Off.
      <li> Disconnect USB (but leave VS1053 module connected).
      <li> Switch OdroidGo On (powered from battery) and immediately press button (A).
      <li> Wait for the screen prompt to release (A) and do so.
      <li> Now the screen should prompt "Now press (B) to continue".
      <li> Before pressing (B), connect USB to OdroidGo to supply 5V to VS1053 module.
      <li> Then press (B). Radio should start properly.
      <li> If not, try again (and read Serial.output to check what might go wrong).
    </ol> 
  </ul> 
 </body>
</html>
)=====" ;

size_t startup_htmlSize = sizeof(startup_htmlArr);
const char *startup_html = startup_htmlArr;
