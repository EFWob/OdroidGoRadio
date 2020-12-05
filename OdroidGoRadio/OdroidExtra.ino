#include "src/libraries/BasicStatemachineOdroid.h"
#include "src/libraries/VirtualPinOdroid.h"
#include "src/libraries/ButtonHandlerOdroid.h"
#include "src/libraries/Display.h"
//#include <odroid_go.h>
#include "OdroidExtra.h"

// VS1053 Plugins Spectrum Analyzer

size_t spectrumAnalyzerPluginSize = 1000;
extern const uint16_t spectrumAnalyzerPlugin[1000];




extern char*       dbgprint( const char* format, ... ) ;
extern bool time_req;
extern String nvsgetstr ( const char* key );

class RadioMenu;
bool nvssetOdroid();


class SpectrumAnalyzer {
   public:
     void load();
     bool activation(bool on);
     bool isActive() {return _active && _valid;};
     void run();
     void run1();
     void reStart() {_lastRun = 0;};
     void showSpeed(uint8_t speed) {_showSpeed = speed;};
     void showDynamic(bool dynamic) {_showDynamic = dynamic;};
     void showBarColor(int16_t color) {_showColor = color;dbgprint("Set Color %d", _showColor);};
     void showPeaks(int16_t peaks) { _showPeaks = peaks;};
     void showSegmentWidth(uint16_t segmentWidth) { _showSegments = segmentWidth;};
     void showWidth(int16_t width) {_barWidth = width;};
     void showPeakWidth(int16_t width) {_peakWidth = width;};
     void showSegmentColor(int16_t col) {_showSegmentColor = col;dbgprint("Set SegmentColor %d", _showSegmentColor);};
     void showText(bool showText) {_showText = showText;};
     bool showText() {return _showText;};
     int16_t getBarColor() {return getColor(_showColor);};
     void fastDrop(bool fast) {_fastDrop = fast;}; 
   protected:
    const uint8_t _speedMap[6] = {0, 25, 50, 100, 200, 500};
    uint8_t _showSpeed = 3;
    bool readBands();
    const uint8_t SCI_WRAM          = 0x6 ;
    const uint8_t SCI_WRAMADDR      = 0x7 ;    
    uint8_t _bands = 14;
    uint16_t _base = 0x1810;    //spectrumAnalyzerAppl1053b-2.plg
    int16_t _barWidth = 3;
    int16_t _peakWidth = -1;
    bool _valid = false;
    bool _fastDrop = true;
    bool _showDynamic = false;
    int16_t _showPeaks = 1;
    int16_t _showSegments = -1;
    int16_t _showSegmentColor = 0;
    int16_t _showColor = 5;
    bool _active = false;  
    bool _showText = false;
    uint8_t _drawn = 0;
    uint32_t _readTime = 0;
    uint32_t _lastRun = 0;
    uint8_t _spectrum[14][4];
    bool _dynamicBrightness = false;

    uint16_t getColor(int16_t code);

} spectrumAnalyzer;




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
    int16_t min = 255;            // minimum backlight value (if equal to max, effectively no dimming will happen)
  } backlight;

  struct {
    int16_t closeListOnSelect = 1;
    int16_t favBanks = 3;
    int16_t ignoreSD = 1;
    int16_t keySpeed = 2;
    int16_t debug = 1;
    int16_t flipped = 1;
  } misc;
  struct {
    int16_t preset = 1;
  } start;
  struct {
    int16_t showSpectrumAnalyzer = 0;
    int16_t spectrumAnalyzerSpeed = 3;
    int16_t spectrumAnalyzerDynamic = 0;
    int16_t spectrumAnalyzerPeaks = 1;
    int16_t spectrumAnalyzerSegmentWidth = -1;
    int16_t spectrumAnalyzerWidth = 3;
    int16_t spectrumAnalyzerText = 0;
    int16_t spectrumAnalyzerPeakWidth = -1;
    int16_t spectrumAnalyzerSegmentColor = 0;
    int16_t spectrumAnalyzerBarColor = 5;  
    int16_t spectrumAnalyzerFastDrop = 1;
    int16_t eq0 = 0;
    int16_t eq1 = 0;
    int16_t eq2 = 0;
    int16_t eq3 = 0;
    int16_t eq4 = 0;
    int16_t eq5 = 0;
    int16_t eq6 = 0;
    int16_t eq7 = 0;
  } equalizer;
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
//bool 
uint8_t presetPresent[100];    // 
uint8_t numPresets = 0;
uint8_t presetMap[100];
//uint8_t presetInverseMap[100];


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
    friend class RadioMenu3;
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

class RadioMenuEntrySpectrumSpeed : public RadioMenuEntry {
  public:
    RadioMenuEntrySpectrumSpeed(char* txt, int16_t *reference):
          RadioMenuEntry(txt, reference, 0, 5) {};

    virtual char *getValueString() {
    char *values[] = {"Disco", "Hectic", "Fast", "Smooth", "Slow", "Slug"}; 
      if (_reference) {
        sprintf(_valueStr, "%6s", values[_value]);
      } else
        strcpy(_valueStr, "");
      return _valueStr;
    };

};


class RadioMenuEntrySpectrumColor : public RadioMenuEntry {
  public:
    RadioMenuEntrySpectrumColor(char* txt, int16_t *reference, bool blackIsOff = false):
          RadioMenuEntry(txt, reference, -1, 10) {
              if (_blackIsOff = blackIsOff) {
                _minVal = 0;
                read();
              }
            };

    virtual char *getValueString() {
    char *values[] = {"Black", "White", "Grey", "Red", "Green", "Blue", "Cyan", "Yellow", "Purple", "Olive", "Magenta"}; 
      if (_reference) {
        if ((_value == -1) || ((_value == 0) && _blackIsOff))
          strcpy(_valueStr, "    OFF");
        else 
          sprintf(_valueStr, "%7s", values[_value]);
      } else
        strcpy(_valueStr, "");
      return _valueStr;
    };
  protected:
    bool _blackIsOff;
};

class RadioMenuEntryWidth : public RadioMenuEntry {
  public:
    RadioMenuEntryWidth(char* txt, int16_t *reference):
          RadioMenuEntry(txt, reference, -1, 20) {
              };

    virtual char *getValueString() {
      if (_reference) {
        if (_value == -1)
          strcpy(_valueStr, "same");
        else if (0 == _value)
          strcpy(_valueStr, " off");
        else 
          sprintf(_valueStr, "%4d", _value);
      }  else
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
      _updated = false;
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
      addEntry(_flip = new RadioMenuEntryBool("Display Flipped", &odroidRadioConfig.misc.flipped));
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
      if (MENU_A == button) {
        if (_flip->value() != odroidRadioConfig.misc.flipped)
          dsp_erase();
        if (DEBUG != _debug->value())
          analyzeCmd("debug", _debug->value()?"1":"0");
        }
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
      RadioMenuEntryBool *_lifeVol, *_flip;
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
    RadioMenu3() : RadioMenu("Equalizer") {
      addEntry(new RadioMenuEntry("--- Equalizer Settings ---"));
      addEntry(new RadioMenuEntry("  ... to be done..."));
      addEntry(new RadioMenuEntry("--- Spectrum Analyzer ----"));
      addEntry(_showSpectrum = new RadioMenuEntryBool("Show Spectrum Analyzer", &odroidRadioConfig.equalizer.showSpectrumAnalyzer));
      addEntry(_showSpeed = new RadioMenuEntrySpectrumSpeed("Analyzer Speed", &odroidRadioConfig.equalizer.spectrumAnalyzerSpeed));
      addEntry(_barWidth = new RadioMenuEntry("Bar width", &odroidRadioConfig.equalizer.spectrumAnalyzerWidth,0,20));
      addEntry(_showColor = new RadioMenuEntrySpectrumColor("Bar Color", &odroidRadioConfig.equalizer.spectrumAnalyzerBarColor)); 
      addEntry(_showDynamic = new RadioMenuEntryBool("Dynamic Brightness", &odroidRadioConfig.equalizer.spectrumAnalyzerDynamic));
      addEntry(_showPeaks = new RadioMenuEntrySpectrumColor("Show Peaks", &odroidRadioConfig.equalizer.spectrumAnalyzerPeaks, true));
      addEntry(_peakWidth = new RadioMenuEntryWidth("Peak width",  &odroidRadioConfig.equalizer.spectrumAnalyzerPeakWidth));
      addEntry(_fastDrop = new RadioMenuEntryBool("Peak drop fast", &odroidRadioConfig.equalizer.spectrumAnalyzerFastDrop));      
      addEntry(_showSegments = new RadioMenuEntryWidth("Segm.Divider width", &odroidRadioConfig.equalizer.spectrumAnalyzerSegmentWidth));
      addEntry(_segmentColor = new RadioMenuEntrySpectrumColor("Segm.Divider color", &odroidRadioConfig.equalizer.spectrumAnalyzerSegmentColor));
      addEntry(_showText = new RadioMenuEntryBool("Show Radiotext", &odroidRadioConfig.equalizer.spectrumAnalyzerText));
    };

    void menuButton(uint8_t button, uint16_t repeats) {
      int16_t showSpectrum = _showSpectrum->value();
      int16_t spectrumSpeed = _showSpeed->value();
      int16_t showDynamic = _showDynamic->value();
      int16_t showPeaks = _showPeaks->value();
      int16_t showSegments = _showSegments->value();
      int16_t barWidth = _barWidth->value();
      int16_t showText = _showText->value();
      int16_t peakWidth = _peakWidth->value();
      int16_t segmentColor = _segmentColor->value();
      int16_t barColor = _showColor->value();
      int16_t fastDrop = _fastDrop->value();
      RadioMenu::menuButton(button, repeats);
      dbgprint("MenuButton done. Value BarColor: %d (was: %d)", _showColor->value(), barColor);
      if (barColor != _showColor->value())
        spectrumAnalyzer.showBarColor(_showColor->value());
      else if (showSpectrum != _showSpectrum->value())
        spectrumAnalyzer.activation(!showSpectrum);
      else if (spectrumSpeed != _showSpeed->value())
        spectrumAnalyzer.showSpeed(_showSpeed->value());
      else if (showDynamic != _showDynamic->value())
        spectrumAnalyzer.showDynamic(_showDynamic->value());
      else if (showPeaks != _showPeaks->value())
        spectrumAnalyzer.showPeaks(_showPeaks->value());
      else if (showSegments != _showSegments->value())
        spectrumAnalyzer.showSegmentWidth(_showSegments->value());
      else if (barWidth != _barWidth->value())
        spectrumAnalyzer.showWidth(_barWidth->value());
      else if (showText != _showText->value())
        spectrumAnalyzer.showText(_showText->value());
      else if (peakWidth != _peakWidth->value())
        spectrumAnalyzer.showPeakWidth(_peakWidth->value());
      else if (segmentColor != _segmentColor->value())
        spectrumAnalyzer.showSegmentColor(_segmentColor->value());
      else if (fastDrop != _fastDrop->value())
        spectrumAnalyzer.fastDrop(_fastDrop->value());

    };
    protected:
      RadioMenuEntryBool* _showSpectrum;
      RadioMenuEntryBool* _showDynamic;
      RadioMenuEntryBool* _fastDrop;
      RadioMenuEntrySpectrumColor* _showPeaks;
      RadioMenuEntrySpectrumColor* _showColor;
      RadioMenuEntrySpectrumColor* _segmentColor;
      RadioMenuEntry* _barWidth;
      RadioMenuEntryWidth* _showSegments;
      RadioMenuEntrySpectrumSpeed* _showSpeed;
      RadioMenuEntryBool* _showText;
      RadioMenuEntryWidth* _peakWidth;
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
    if (!odroidRadioConfig.misc.flipped)
      id = 5 - id;
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
  if (!odroidRadioConfig.misc.flipped)
    direction = -direction;
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
    if (!odroidRadioConfig.misc.flipped)
      dir = -dir;
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
    btnVol.onPressed([]() {handleBtnMemory(3);}).onLongPressed([](uint16_t) {radioStatemachine.startMenu(3);});
    btnMenu.onPressed([]() {handleBtnMemory(4);}).onLongPressed([](uint16_t) {radioStatemachine.startMenu(4);});
    btnA.onLongPressed([](uint16_t) {handleBtnAB(0);}).onLongReleased([]() {handleBtnAB(0, true);});
    btnB.onLongPressed([](uint16_t) {handleBtnAB(1);}).onLongReleased([]() {handleBtnAB(1, true);});
    for(int i = 0;i < 100;i++) {
      sprintf(key, "preset_%02d", i);
      if (nvssearch(key)) {
        presetMap[numPresets++] = i;
        presetPresent[i] = numPresets;
      } else
          presetPresent[i] = 0;
    }  
    if (nvsgetOdroid())
      dbgprint("Odroid config data from nvs done!");
    dbgprint( "Odroid-Setup done" ) ;
    dsp_setRotation();
    
/*    if (odroidRadioConfig.misc.flipped)
      tft->setRotation(3);
    else
      tft->setRotation(1);
*/
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

bool odroidRadioFirstLoopSetup() {
static uint8_t step = 0;
char s[50];
int16_t iniVolume;
// setup initial volume
  if (0 == step) {
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
//      radioStateMachine._menu[2] = new RadioMenu3;
  if (!NetworkFound)
    radioStatemachine.setState(RADIOSTATE_INICONFIG);
  else if (odroidVsError)
    radioStatemachine.setState(RADIOSTATE_INIVS);
  else
    radioStatemachine.resetStateTime();
  } else if (1 == step) {
    spectrumAnalyzer.load();
    spectrumAnalyzer.activation(odroidRadioConfig.equalizer.showSpectrumAnalyzer);
    spectrumAnalyzer.showSpeed(odroidRadioConfig.equalizer.spectrumAnalyzerSpeed);
    spectrumAnalyzer.showDynamic(odroidRadioConfig.equalizer.spectrumAnalyzerDynamic);
    spectrumAnalyzer.showPeaks(odroidRadioConfig.equalizer.spectrumAnalyzerPeaks);
    spectrumAnalyzer.showSegmentWidth(odroidRadioConfig.equalizer.spectrumAnalyzerSegmentWidth);
    spectrumAnalyzer.showSegmentColor(odroidRadioConfig.equalizer.spectrumAnalyzerSegmentColor);
    spectrumAnalyzer.showWidth(odroidRadioConfig.equalizer.spectrumAnalyzerWidth);
    spectrumAnalyzer.showText(odroidRadioConfig.equalizer.spectrumAnalyzerText);
    spectrumAnalyzer.showPeakWidth(odroidRadioConfig.equalizer.spectrumAnalyzerPeakWidth);
    spectrumAnalyzer.showBarColor(odroidRadioConfig.equalizer.spectrumAnalyzerBarColor);
    spectrumAnalyzer.fastDrop(odroidRadioConfig.equalizer.spectrumAnalyzerFastDrop);
  }
  step++;
  return step < 2;      // more to do?
}

void odroidLoop(void) {    
static bool first = true;
  if (first) {
    first = odroidRadioFirstLoopSetup();
    dbgprint("First loop setup done!");
  } else {
    StatemachineLooper.run();
    runSpectrumAnalyzer();
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
//  GO.lcd.begin();
//  tft = &GO.lcd;
//  GO.lcd.setBrightness(brightnessLevel);
  tft = new ILI9341();
  tft->begin();
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
    if (!odroidRadioConfig.misc.flipped)
      idx = 5 - idx;
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

  void dsp_setRotation() {
      if (odroidRadioConfig.misc.flipped) {
        tft->setRotation(3);
        dbgprint("DISPLAY Rotation3");
      } else {
        tft->setRotation(1);
        dbgprint("DISPLAY Rotation1");

      }    
  }
   
  void RadioStatemachine::onEnter(int16_t currentStatenb, int16_t oldStatenb) {
    uint32_t screenSections[NUM_RADIOSTATES] = {
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
      bool ignoreSpectrumAnalyzerSection = false;
      tftdata[TFTSEC_TXT].color = CYAN;
      
      dsp_setRotation();

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

    if ((RADIOSTATE_RUN == currentStatenb) || (RADIOSTATE_VOLUME == currentStatenb) || (RADIOSTATE_MENU_DONE == currentStatenb) || (RADIOSTATE_LIST_SET == currentStatenb)) {
//  These are the states where Spectrum Analyzer (if switched On) or else RadioText is visible
      screenSections[currentStatenb-1] = bit(TFTSEC_TOP)|bit(TFTSEC_TXT)|
            (RADIOSTATE_VOLUME == currentStatenb?bit(TFTSEC_VOL):bit(TFTSEC_BOT))|bit(TFTSEC_FAV_BOT);          // "default" with RadioText
      if (spectrumAnalyzer.isActive()) {
        spectrumAnalyzer.reStart();
        dbgprint("Try switch screen to spectrum analyzer");
//        spectrumAnalyzer.activation(false);
//        if (spectrumAnalyzer.activation(true)) 
        {     // forces initalization of spectrum values and verifies if plugin is still working correctly
          screenSections[currentStatenb - 1] = bit(TFTSEC_TOP)|bit(TFTSEC_SPECTRUM)|
          (RADIOSTATE_VOLUME == currentStatenb?bit(TFTSEC_VOL):bit(TFTSEC_BOT))|bit(TFTSEC_FAV_BOT);     // RADIOSTATE_RUN ("normal") with Spectrum Analyzer     
          dbgprint("Screen set to spectrum analyzer");
          if (ignoreSpectrumAnalyzerSection = spectrumAnalyzer.showText()) {
              tftdata[TFTSEC_TXT].color = spectrumAnalyzer.getBarColor();            
              screenSections[currentStatenb - 1] = screenSections[currentStatenb - 1] | bit(TFTSEC_TXT);
         }
          else
              tftdata[TFTSEC_SPECTRUM].color = spectrumAnalyzer.getBarColor();

/*          if (tftdata[TFTSEC_SPECTRUM].hidden)
            if (spectrumAnalyzer.showText()) {
              screenSections[currentStatenb - 1] = screenSections[currentStatenb - 1] | bit(TFTSEC_TXT);
              tftdata[TFTSEC_SPECTRUM].color = tftdata[TFTSEC_TXT].color; 
              tftset(TFTSEC_SPECTRUM, tftdata[TFTSEC_TXT].str);
            } else {
              tftset(TFTSEC_SPECTRUM, "  ** SPECTRUM ANALYZER **");
              tftdata[TFTSEC_SPECTRUM].color = TFT_ORANGE; 
            }
*/
        }
      }
    } else if (RADIOSTATE_INICONFIG == currentStatenb) {
      analyzeCmd("stop");
      dsp_erase();
      tftdata[TFTSEC_MEN_TOP].color = TFT_RED;
      tftset(TFTSEC_MEN_TOP, "WiFi failure");
      tftset(TFTSEC_LIST_CLR, "Connect desktop PC to SSID" NAME "/Pw " NAME "Open http://192.198.4.1/  to check your WiFi creden-tials.\n\n"
                        "Press (Default) to load   initial values.\n\n"
                        "Press (Save) and (Restart)when done!");
    } else if (RADIOSTATE_INIVS == currentStatenb) {
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
            if ((i == TFTSEC_SPECTRUM) && ignoreSpectrumAnalyzerSection) // Special for Spectrum analyzer: hidden flag is evaluated to run analyzer.
              tftdata[i].update_req = false;                             // however, if Radiotext is to displayed as well, the contents are ignored 
            else                                                         // and radiotext section is used instead (kind of a hack here). 
              tftdata[i].update_req = true;
          }
        } else
          tftdata[i].hidden = true;
        bm = bm * 2;
      }
    }
    //Special for "not-flipped-Display"
    if (false == tftdata[TFTSEC_FAV_BUT].hidden)
      if (!odroidRadioConfig.misc.flipped) {
        tftdata[TFTSEC_TOP].hidden = false;
        tftdata[TFTSEC_FAV_BOT].hidden = true;
        tftdata[TFTSEC_TOP].update_req = true;
        tftdata[TFTSEC_FAV_BUT].hidden = true;
        tftdata[TFTSEC_FAV_BUT2].hidden = false;
        tftdata[TFTSEC_FAV_BUT2].update_req = true;
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
//        spectrumAnalyzer.run();
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
//        spectrumAnalyzer.run();
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


void SpectrumAnalyzer::load() {
  vs1053player->loadUserCode((const uint16_t *)&spectrumAnalyzerPlugin, spectrumAnalyzerPluginSize); 
  _valid = true;
  memset(_spectrum, 0, sizeof(_spectrum));
  if (readBands())
    dbgprint("Spectrum analyzer plugin installed successfully");
  else
    dbgprint("Failure installing spectrum analyzer plugin");    
}

bool SpectrumAnalyzer::activation(bool on) {
  if (_valid)
    if (on != _active) {
      _active = on;
      if (on) {
        memset(_spectrum, 0, sizeof(_spectrum));
        readBands();
        _lastRun = 0;
      }
    }
  return _valid && _active;
}


#ifdef OLD
void SpectrumAnalyzer::run() {
//static uint32_t lastRun = 0;
//  if (isActive()) {
//    uint32_t timeNow = millis();
    if (!_lastRun || (millis() - _lastRun >= 0)) {
      _lastRun = (_lastRun > 0)?_lastRun -8:millis();
      if (_drawn == _bands) {
        static uint8_t idx = 0;
        readBands();
//        if (0 == (idx++ & 0x1f))
//        dbgprint("Spectrum read: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
//          _spectrum[0][0],_spectrum[1][0],_spectrum[2][0],_spectrum[3][0],_spectrum[4][0],_spectrum[5][0],_spectrum[6][0],_spectrum[7][0],_spectrum[8][0],
//          _spectrum[9][0],_spectrum[10][0],_spectrum[11][0],_spectrum[12][0],_spectrum[13][0]);
      }
      uint8_t toDraw = esp_random();
      bool refresh = 0 == (toDraw & 0x3f);
      toDraw = toDraw % _bands;
      while (_spectrum[toDraw][3])
        toDraw = (toDraw + 1) % _bands;
      _spectrum[toDraw][3] = 1;
//      toDraw = _drawn;
      uint8_t x = 12 + 10 * toDraw;
      uint8_t y = 18 + (31 - _spectrum[toDraw][0]) * 2;  
      uint8_t yOld = 18 + (31 - _spectrum[toDraw][1]) * 2;
      uint8_t w = 6;

      if ((y != yOld) || (_spectrum[toDraw][2])) { 
        if (_spectrum[toDraw][2]) {
//          claimSPI("spectrum");
          //dsp_fillRect(x, _spectrum[toDraw][2],w ,1, TFT_BLACK);
          tft->fillRect(2*x,2*_spectrum[toDraw][2],2*w ,1, TFT_BLACK);
//          releaseSPI();
          if (y > _spectrum[toDraw][2]) {
            if (++_spectrum[toDraw][2] > 78)
              _spectrum[toDraw][2] = 0;
          } else
            _spectrum[toDraw][2] = y - 2;
        } else if (y < 80)
          _spectrum[toDraw][2] = y - 2;
        if (y > yOld) {      // was higher before
//          claimSPI("spectrum");
          dsp_fillRect(x, yOld, w, y - yOld, TFT_BLACK);
//          releaseSPI();
        } else if (y < yOld) { 
//         if (refresh)
          yOld = 80;
//         claimSPI("spectrum");
         dsp_fillRect(x, y, w, yOld - y, BLUE);
         for(int i = 156;i >= 2 * y;i = i - 4)
              tft->fillRect ( 2*x, i, 2*w, 1, TFT_BLACK);
//         releaseSPI();
        }
        if (_spectrum[toDraw][2]) {
//          claimSPI("spectrum");
//          dsp_fillRect(x, _spectrum[toDraw][2],w,1, TFT_WHITE);
          tft->fillRect(2*x,2*_spectrum[toDraw][2],2*w ,1, TFT_WHITE);
//          releaseSPI();
        }
      }
      _spectrum[toDraw][1] = _spectrum[toDraw][0];
      _drawn++;
    }
//  }
}

#endif

void SpectrumAnalyzer::run() {
//static uint32_t lastRun = 0;
//  if (isActive()) {
//    uint32_t timeNow = millis();
//    const uint8_t _speedMap[5] = {0, 25, 50, 100, 250};
//    uint8_t _showSpeed = 3;

    if (!_lastRun || (millis() - _lastRun >= _speedMap[_showSpeed])) {
      _lastRun = millis();
      readBands();
      //hack
      if (_showText)
        tftdata[TFTSEC_TXT].hidden = false;
    for(uint8_t toDraw = 0;toDraw < 14;toDraw++) {
      uint8_t w = _barWidth;
      uint8_t x = 10 + (5 - w/2) + 10 * toDraw;
      uint8_t y = 18 + (31 - _spectrum[toDraw][0]) * 2;  
      uint8_t yOld = 18 + (31 - _spectrum[toDraw][1]) * 2;

      
      if ((y != yOld) || (_spectrum[toDraw][2])) { 
        if (_spectrum[toDraw][2]) {
//          if (_showText)
            tft->fillRect(20 + 20 * toDraw, 2*_spectrum[toDraw][2]-1,20 ,2, TFT_BLACK);
//          else
//            tft->fillRect(2*x,2*_spectrum[toDraw][2],2*(w?w:1) ,1, TFT_BLACK);
          if (!_showPeaks)
            _spectrum[toDraw][2] = 0;
          else if (y > _spectrum[toDraw][2]) {
            if (++_spectrum[toDraw][2] > 78)
              _spectrum[toDraw][2] = 0;
          } else
            _spectrum[toDraw][2] = y - 2;
        } else if ((y < 80) && _showPeaks)
          _spectrum[toDraw][2] = y - 2;
        if (y > yOld) {      // was higher before
          if (_showText)
            dsp_fillRect(10 + 10 * toDraw, yOld, 10, y - yOld, TFT_BLACK);          
          else
            dsp_fillRect(x, yOld, w, y - yOld, TFT_BLACK);
        } /*else if (y < yOld)*/ { 
          yOld = 80;
          uint16_t col = BLUE;
//          col = 8 * (uint16_t)_spectrum[toDraw][0];
         if (_showDynamic) {
            if (_spectrum[toDraw][0] < 16)
              col = col - (16 -  _spectrum[toDraw][0])  - (16 -  _spectrum[toDraw][0])/4 - (16 -  _spectrum[toDraw][0])/8;
            else
              col = col + 0x20 * (uint16_t)((_spectrum[toDraw][0]) & 0xf);
            col = (_spectrum[toDraw][0] > 15?0x1f + 0x40 * (_spectrum[toDraw][0] >> 3):(2 * (_spectrum[toDraw][0] & 0xc) + 7));
         }
         dsp_fillRect(x, y, w, yOld - y, col);
         if (_showSegments) {
            uint16_t ws, xs, col = getColor(_showSegmentColor);
            if ((_showSegments == -1) || (_showSegments == w)) {
              ws = w;xs = x;
            } else {
              ws = _showSegments;
              xs = 10 + (5 - ws/2) + 10 * toDraw;
            }
            for(int i = 156;i >= 2 * y;i = i - 4)
              tft->fillRect ( 2*xs, i, 2*ws, 1, col);
         }
        }
        if (_spectrum[toDraw][2] && _peakWidth) {
            if ((_peakWidth != -1) && (_peakWidth != w)) {
              w = _peakWidth;
              x = 10 + (5 - w/2) + 10 * toDraw;
            }
          tft->fillRect(2*x,2*_spectrum[toDraw][2],2*w ,1, getColor(_showPeaks));
        }
      }
      _spectrum[toDraw][1] = _spectrum[toDraw][0];
    }
    }
}

void SpectrumAnalyzer::run1() {
//static uint32_t lastRun = 0;
//  if (isActive()) {
//    uint32_t timeNow = millis();
//    const uint8_t _speedMap[5] = {0, 25, 50, 100, 250};
//    uint8_t _showSpeed = 3;

    if (!_lastRun || (millis() - _lastRun >= _speedMap[_showSpeed])) {
      _lastRun = millis();
      readBands();
      //hack
      if (_showText)
        tftdata[TFTSEC_TXT].hidden = false;
    for(uint8_t toDraw = 0;toDraw < 14;toDraw++) {
      uint16_t w = _barWidth;
      uint16_t x = 20 + (10 - w/2) + 20 * toDraw;
      uint16_t y = 36 + (31 - _spectrum[toDraw][0]) * 4;  
      uint16_t yOld = 36 + (31 - _spectrum[toDraw][1]) * 4;

      
      if ((y != yOld) || (_spectrum[toDraw][2])) { 
        if (_spectrum[toDraw][2]) {
            tft->fillRect(20 + 20 * toDraw, _spectrum[toDraw][2]-1,20 ,2, TFT_BLACK);
          if (!_showPeaks)
            _spectrum[toDraw][2] = 0;
          else if (y > _spectrum[toDraw][2]) {
            _spectrum[toDraw][2] = _spectrum[toDraw][2] + (_fastDrop?2:1);
            if (_spectrum[toDraw][2] > 156)
              _spectrum[toDraw][2] = 0;
          } else
            _spectrum[toDraw][2] = y - 2;
        } else if ((y < 160) && _showPeaks)
          _spectrum[toDraw][2] = y - 2;
        if (y > yOld) {      // was higher before
//          if (_showText)
            tft->fillRect(20 + 20 * toDraw, yOld, 20, y - yOld, TFT_BLACK);          
//          else
//            dsp_fillRect(x, yOld, w, y - yOld, TFT_BLACK);
        } /*else if (y < yOld)*/ { 
          yOld = 160;
          uint16_t col = getColor(_showColor);
//          col = 8 * (uint16_t)_spectrum[toDraw][0];
         if (_showDynamic) {
            uint8_t val;
            if ((val = _spectrum[toDraw][0]) < 16)
              col = (map(val, 0, 15, col & 8, col & 0x1f) & 0x1f) + (map(val, 0, 15, col & 0x100, col & 0x7e0) & 0x7e0) + (map(val, 0, 15, col & 0x4000, col & 0xf800) & 0xf800);
         }
         tft->fillRect(x, y, w, yOld - y, col);
         if (_showSegments) {
            uint16_t ws, xs, col = getColor(_showSegmentColor);
            if ((_showSegments == -1) || (_showSegments == w)) {
              ws = w;xs = x;
            } else {
              ws = _showSegments;
              xs = 20 + (10 - ws/2) + 20 * toDraw;
            }
            for(int i = 156;i > y;i = i - 4)
              tft->fillRect ( xs, i, ws, 1, col);
         }
        }
        if (_spectrum[toDraw][2] && _peakWidth) {
            if ((_peakWidth != -1) && (_peakWidth != w)) {
              w = _peakWidth;
              x = 20 + (10 - w/2) + 20 * toDraw;
            }
          tft->fillRect(x,_spectrum[toDraw][2],w ,1, getColor(_showPeaks));
        }
      }
      _spectrum[toDraw][1] = _spectrum[toDraw][0];
    }
    }
}

uint16_t SpectrumAnalyzer::getColor(int16_t code) {
  uint16_t colorTable[] = {0, 0xffff, 0x8bef, 0xf800, 0x7e0, 0x1f, 0x7ff, 0xffe0, 0x880f, 0x8be0, 0xf81f};
  if ((code >= 0) && (code <= 10))
    return colorTable[code];
  else
    return 0;
}


void runSpectrumAnalyzer() {
  if (!tftdata[TFTSEC_SPECTRUM].hidden)             // this is only true, if spectrum analyzer should be displayed
    if (!(spectrumAnalyzer.showText()?tftdata[TFTSEC_TXT].update_req:tftdata[TFTSEC_SPECTRUM].update_req)){      
      // and this only after the display area has been cleared (and thus deleted the previous overlaying content)
      claimSPI("spectrum");
      spectrumAnalyzer.run1();
      releaseSPI();
  }
}

bool SpectrumAnalyzer::readBands() {
uint8_t bands;
     _drawn = 0;
//    claimSPI("spectrum");
//  if (_valid) {
        vs1053player->write_register(SCI_WRAMADDR, _base+2);
        bands = vs1053player->read_register(SCI_WRAM);
//        _valid = _bands == 14;
        if (bands == _bands) {
//          vs1053player->write_register(SCI_WRAMADDR, _base+1);
//          vs1053player->write_register(SCI_WRAM, 0xffff);
          vs1053player->write_register(SCI_WRAMADDR, _base+4);
          for (uint8_t i = 0; i < _bands; i++) {
            _spectrum[i][0] = vs1053player->read_register(SCI_WRAM) & 0x1f;
            _spectrum[i][3] = 0;
          }
        } else {
          dbgprint("Spectrum analyzer read failed! Bands: %d", bands);
          for (uint8_t i = 0; i < _bands; i++) {
            if (_spectrum[i][0])  
              _spectrum[i][0]--;  
            _spectrum[i][3] = 0;
          }
        }
//     releaseSPI();
  return bands == _bands;
}


const uint16_t spectrumAnalyzerPlugin[1000] = { /* Compressed plugin */
  0x0007, 0x0001, 0x8d00, 0x0006, 0x0004, 0x2803, 0x5b40, 0x0000, /*    0 */
  0x0024, 0x0007, 0x0001, 0x8d02, 0x0006, 0x00d6, 0x3e12, 0xb817, /*    8 */
  0x3e12, 0x3815, 0x3e05, 0xb814, 0x3615, 0x0024, 0x0000, 0x800a, /*   10 */
  0x3e10, 0x3801, 0x0006, 0x0800, 0x3e10, 0xb803, 0x0000, 0x0303, /*   18 */
  0x3e11, 0x3805, 0x3e11, 0xb807, 0x3e14, 0x3812, 0xb884, 0x130c, /*   20 */
  0x3410, 0x4024, 0x4112, 0x10d0, 0x4010, 0x008c, 0x4010, 0x0024, /*   28 */
  0xf400, 0x4012, 0x3000, 0x3840, 0x3009, 0x3801, 0x0000, 0x0041, /*   30 */
  0xfe02, 0x0024, 0x2903, 0xb480, 0x48b2, 0x0024, 0x36f3, 0x0844, /*   38 */
  0x6306, 0x8845, 0xae3a, 0x8840, 0xbf8e, 0x8b41, 0xac32, 0xa846, /*   40 */
  0xffc8, 0xabc7, 0x3e01, 0x7800, 0xf400, 0x4480, 0x6090, 0x0024, /*   48 */
  0x6090, 0x0024, 0xf400, 0x4015, 0x3009, 0x3446, 0x3009, 0x37c7, /*   50 */
  0x3009, 0x1800, 0x3009, 0x3844, 0x48b3, 0xe1e0, 0x4882, 0x4040, /*   58 */
  0xfeca, 0x0024, 0x5ac2, 0x0024, 0x5a52, 0x0024, 0x4cc2, 0x0024, /*   60 */
  0x48ba, 0x4040, 0x4eea, 0x4801, 0x4eca, 0x9800, 0xff80, 0x1bc1, /*   68 */
  0xf1eb, 0xe3e2, 0xf1ea, 0x184c, 0x4c8b, 0xe5e4, 0x48be, 0x9804, /*   70 */
  0x488e, 0x41c6, 0xfe82, 0x0024, 0x5a8e, 0x0024, 0x525e, 0x1b85, /*   78 */
  0x4ffe, 0x0024, 0x48b6, 0x41c6, 0x4dd6, 0x48c7, 0x4df6, 0x0024, /*   80 */
  0xf1d6, 0x0024, 0xf1d6, 0x0024, 0x4eda, 0x0024, 0x0000, 0x0fc3, /*   88 */
  0x2903, 0xb480, 0x4e82, 0x0024, 0x4084, 0x130c, 0x0006, 0x0500, /*   90 */
  0x3440, 0x4024, 0x4010, 0x0024, 0xf400, 0x4012, 0x3200, 0x4024, /*   98 */
  0xb132, 0x0024, 0x4214, 0x0024, 0xf224, 0x0024, 0x6230, 0x0024, /*   a0 */
  0x0001, 0x0001, 0x2803, 0x54c9, 0x0000, 0x0024, 0xf400, 0x40c2, /*   a8 */
  0x3200, 0x0024, 0xff82, 0x0024, 0x48b2, 0x0024, 0xb130, 0x0024, /*   b0 */
  0x6202, 0x0024, 0x003f, 0xf001, 0x2803, 0x57d1, 0x0000, 0x1046, /*   b8 */
  0xfe64, 0x0024, 0x48be, 0x0024, 0x2803, 0x58c0, 0x3a01, 0x8024, /*   c0 */
  0x3200, 0x0024, 0xb010, 0x0024, 0xc020, 0x0024, 0x3a00, 0x0024, /*   c8 */
  0x36f4, 0x1812, 0x36f1, 0x9807, 0x36f1, 0x1805, 0x36f0, 0x9803, /*   d0 */
  0x36f0, 0x1801, 0x3405, 0x9014, 0x36f3, 0x0024, 0x36f2, 0x1815, /*   d8 */
  0x2000, 0x0000, 0x36f2, 0x9817, 0x0007, 0x0001, 0x8d6d, 0x0006, /*   e0 */
  0x01f6, 0x3613, 0x0024, 0x3e12, 0xb817, 0x3e12, 0x3815, 0x3e05, /*   e8 */
  0xb814, 0x3645, 0x0024, 0x0000, 0x800a, 0x3e10, 0xb803, 0x3e11, /*   f0 */
  0x3805, 0x3e11, 0xb811, 0x3e14, 0xb813, 0x3e13, 0xf80e, 0x4182, /*   f8 */
  0x384d, 0x0006, 0x0912, 0x2803, 0x6105, 0x0006, 0x0451, 0x0006, /*  100 */
  0xc352, 0x3100, 0x8803, 0x6238, 0x1bcc, 0x0000, 0x0024, 0x2803, /*  108 */
  0x7705, 0x4194, 0x0024, 0x0006, 0x0912, 0x3613, 0x0024, 0x0006, /*  110 */
  0x0411, 0x0000, 0x0302, 0x3009, 0x3850, 0x0006, 0x0410, 0x3009, /*  118 */
  0x3840, 0x0000, 0x1100, 0x2914, 0xbec0, 0xb882, 0xb801, 0x0000, /*  120 */
  0x1000, 0x0006, 0x0810, 0x2915, 0x7ac0, 0xb882, 0x0024, 0x3900, /*  128 */
  0x9bc1, 0x0006, 0xc351, 0x3009, 0x1bc0, 0x3009, 0x1bd0, 0x3009, /*  130 */
  0x0404, 0x0006, 0x0451, 0x2803, 0x66c0, 0x3901, 0x0024, 0x4448, /*  138 */
  0x0402, 0x4294, 0x0024, 0x6498, 0x2402, 0x001f, 0x4002, 0x6424, /*  140 */
  0x0024, 0x0006, 0x0411, 0x2803, 0x6611, 0x0000, 0x03ce, 0x2403, /*  148 */
  0x764e, 0x0000, 0x0013, 0x0006, 0x1a04, 0x0006, 0x0451, 0x3100, /*  150 */
  0x8024, 0xf224, 0x44c5, 0x4458, 0x0024, 0xf400, 0x4115, 0x3500, /*  158 */
  0xc024, 0x623c, 0x0024, 0x0000, 0x0024, 0x2803, 0x7691, 0x0000, /*  160 */
  0x0024, 0x4384, 0x184c, 0x3100, 0x3800, 0x2915, 0x7dc0, 0xf200, /*  168 */
  0x0024, 0x003f, 0xfec3, 0x4084, 0x4491, 0x3113, 0x1bc0, 0xa234, /*  170 */
  0x0024, 0x0000, 0x2003, 0x6236, 0x2402, 0x0000, 0x1003, 0x2803, /*  178 */
  0x6fc8, 0x0000, 0x0024, 0x003f, 0xf803, 0x3100, 0x8024, 0xb236, /*  180 */
  0x0024, 0x2803, 0x75c0, 0x3900, 0xc024, 0x6236, 0x0024, 0x0000, /*  188 */
  0x0803, 0x2803, 0x7208, 0x0000, 0x0024, 0x003f, 0xfe03, 0x3100, /*  190 */
  0x8024, 0xb236, 0x0024, 0x2803, 0x75c0, 0x3900, 0xc024, 0x6236, /*  198 */
  0x0024, 0x0000, 0x0403, 0x2803, 0x7448, 0x0000, 0x0024, 0x003f, /*  1a0 */
  0xff03, 0x3100, 0x8024, 0xb236, 0x0024, 0x2803, 0x75c0, 0x3900, /*  1a8 */
  0xc024, 0x6236, 0x0402, 0x003f, 0xff83, 0x2803, 0x75c8, 0x0000, /*  1b0 */
  0x0024, 0xb236, 0x0024, 0x3900, 0xc024, 0xb884, 0x07cc, 0x3900, /*  1b8 */
  0x88cc, 0x3313, 0x0024, 0x0006, 0x0491, 0x4194, 0x2413, 0x0006, /*  1c0 */
  0x04d1, 0x2803, 0x9755, 0x0006, 0x0902, 0x3423, 0x0024, 0x3c10, /*  1c8 */
  0x8024, 0x3100, 0xc024, 0x4304, 0x0024, 0x39f0, 0x8024, 0x3100, /*  1d0 */
  0x8024, 0x3cf0, 0x8024, 0x0006, 0x0902, 0xb884, 0x33c2, 0x3c20, /*  1d8 */
  0x8024, 0x34d0, 0xc024, 0x6238, 0x0024, 0x0000, 0x0024, 0x2803, /*  1e0 */
  0x8dd8, 0x4396, 0x0024, 0x2403, 0x8d83, 0x0000, 0x0024, 0x3423, /*  1e8 */
  0x0024, 0x34e4, 0x4024, 0x3123, 0x0024, 0x3100, 0xc024, 0x4304, /*  1f0 */
  0x0024, 0x4284, 0x2402, 0x0000, 0x2003, 0x2803, 0x8b89, 0x0000, /*  1f8 */
  0x0024, 0x3423, 0x184c, 0x34f4, 0x4024, 0x3004, 0x844c, 0x3100, /*  200 */
  0xb850, 0x6236, 0x0024, 0x0006, 0x0802, 0x2803, 0x81c8, 0x4088, /*  208 */
  0x1043, 0x4336, 0x1390, 0x4234, 0x0024, 0x4234, 0x0024, 0xf400, /*  210 */
  0x4091, 0x2903, 0xa480, 0x0003, 0x8308, 0x4336, 0x1390, 0x4234, /*  218 */
  0x0024, 0x4234, 0x0024, 0x2903, 0x9a00, 0xf400, 0x4091, 0x0004, /*  220 */
  0x0003, 0x3423, 0x1bd0, 0x3404, 0x4024, 0x3123, 0x0024, 0x3100, /*  228 */
  0x8024, 0x6236, 0x0024, 0x0000, 0x4003, 0x2803, 0x85c8, 0x0000, /*  230 */
  0x0024, 0xb884, 0x878c, 0x3900, 0x8024, 0x34e4, 0x4024, 0x3123, /*  238 */
  0x0024, 0x31e0, 0x8024, 0x6236, 0x0402, 0x0000, 0x0024, 0x2803, /*  240 */
  0x8b88, 0x4284, 0x0024, 0x0000, 0x0024, 0x2803, 0x8b95, 0x0000, /*  248 */
  0x0024, 0x3413, 0x184c, 0x3410, 0x8024, 0x3e10, 0x8024, 0x34e0, /*  250 */
  0xc024, 0x2903, 0x4080, 0x3e10, 0xc024, 0xf400, 0x40d1, 0x003f, /*  258 */
  0xff44, 0x36e3, 0x048c, 0x3100, 0x8024, 0xfe44, 0x0024, 0x48ba, /*  260 */
  0x0024, 0x3901, 0x0024, 0x0000, 0x00c3, 0x3423, 0x0024, 0xf400, /*  268 */
  0x4511, 0x34e0, 0x8024, 0x4234, 0x0024, 0x39f0, 0x8024, 0x3100, /*  270 */
  0x8024, 0x6294, 0x0024, 0x3900, 0x8024, 0x0006, 0x0411, 0x6894, /*  278 */
  0x04c3, 0xa234, 0x0403, 0x6238, 0x0024, 0x0000, 0x0024, 0x2803, /*  280 */
  0x9741, 0x0000, 0x0024, 0xb884, 0x90cc, 0x39f0, 0x8024, 0x3100, /*  288 */
  0x8024, 0xb884, 0x3382, 0x3c20, 0x8024, 0x34d0, 0xc024, 0x6238, /*  290 */
  0x0024, 0x0006, 0x0512, 0x2803, 0x9758, 0x4396, 0x0024, 0x2403, /*  298 */
  0x9703, 0x0000, 0x0024, 0x0003, 0xf002, 0x3201, 0x0024, 0xb424, /*  2a0 */
  0x0024, 0x0028, 0x0002, 0x2803, 0x9605, 0x6246, 0x0024, 0x0004, /*  2a8 */
  0x0003, 0x2803, 0x95c1, 0x4434, 0x0024, 0x0000, 0x1003, 0x6434, /*  2b0 */
  0x0024, 0x2803, 0x9600, 0x3a00, 0x8024, 0x3a00, 0x8024, 0x3213, /*  2b8 */
  0x104c, 0xf400, 0x4511, 0x34f0, 0x8024, 0x6294, 0x0024, 0x3900, /*  2c0 */
  0x8024, 0x36f3, 0x4024, 0x36f3, 0xd80e, 0x36f4, 0x9813, 0x36f1, /*  2c8 */
  0x9811, 0x36f1, 0x1805, 0x36f0, 0x9803, 0x3405, 0x9014, 0x36f3, /*  2d0 */
  0x0024, 0x36f2, 0x1815, 0x2000, 0x0000, 0x36f2, 0x9817, 0x0007, /*  2d8 */
  0x0001, 0x1868, 0x0006, 0x0010, 0x0032, 0x004f, 0x007e, 0x00c8, /*  2e0 */
  0x013d, 0x01f8, 0x0320, 0x04f6, 0x07e0, 0x0c80, 0x13d8, 0x1f7f, /*  2e8 */
  0x3200, 0x4f5f, 0x61a8, 0x0000, 0x0007, 0x0001, 0x8e68, 0x0006, /*  2f0 */
  0x0054, 0x3e12, 0xb814, 0x0000, 0x800a, 0x3e10, 0x3801, 0x3e10, /*  2f8 */
  0xb803, 0x3e11, 0x7806, 0x3e11, 0xf813, 0x3e13, 0xf80e, 0x3e13, /*  300 */
  0x4024, 0x3e04, 0x7810, 0x449a, 0x0040, 0x0001, 0x0003, 0x2803, /*  308 */
  0xa344, 0x4036, 0x03c1, 0x0003, 0xffc2, 0xb326, 0x0024, 0x0018, /*  310 */
  0x0042, 0x4326, 0x4495, 0x4024, 0x40d2, 0x0000, 0x0180, 0xa100, /*  318 */
  0x4090, 0x0010, 0x0fc2, 0x4204, 0x0024, 0xbc82, 0x4091, 0x459a, /*  320 */
  0x0024, 0x0000, 0x0054, 0x2803, 0xa244, 0xbd86, 0x4093, 0x2403, /*  328 */
  0xa205, 0xfe01, 0x5e0c, 0x5c43, 0x5f2d, 0x5e46, 0x020c, 0x5c56, /*  330 */
  0x8a0c, 0x5e53, 0x5e0c, 0x5c43, 0x5f2d, 0x5e46, 0x020c, 0x5c56, /*  338 */
  0x8a0c, 0x5e52, 0x0024, 0x4cb2, 0x4405, 0x0018, 0x0044, 0x654a, /*  340 */
  0x0024, 0x2803, 0xb040, 0x36f4, 0x5810, 0x0007, 0x0001, 0x8e92, /*  348 */
  0x0006, 0x0080, 0x3e12, 0xb814, 0x0000, 0x800a, 0x3e10, 0x3801, /*  350 */
  0x3e10, 0xb803, 0x3e11, 0x7806, 0x3e11, 0xf813, 0x3e13, 0xf80e, /*  358 */
  0x3e13, 0x4024, 0x3e04, 0x7810, 0x449a, 0x0040, 0x0000, 0x0803, /*  360 */
  0x2803, 0xaf04, 0x30f0, 0x4024, 0x0fff, 0xfec2, 0xa020, 0x0024, /*  368 */
  0x0fff, 0xff02, 0xa122, 0x0024, 0x4036, 0x0024, 0x0000, 0x1fc2, /*  370 */
  0xb326, 0x0024, 0x0010, 0x4002, 0x4326, 0x4495, 0x4024, 0x40d2, /*  378 */
  0x0000, 0x0180, 0xa100, 0x4090, 0x0010, 0x0042, 0x4204, 0x0024, /*  380 */
  0xbc82, 0x4091, 0x459a, 0x0024, 0x0000, 0x0054, 0x2803, 0xae04, /*  388 */
  0xbd86, 0x4093, 0x2403, 0xadc5, 0xfe01, 0x5e0c, 0x5c43, 0x5f2d, /*  390 */
  0x5e46, 0x0024, 0x5c56, 0x0024, 0x5e53, 0x5e0c, 0x5c43, 0x5f2d, /*  398 */
  0x5e46, 0x0024, 0x5c56, 0x0024, 0x5e52, 0x0024, 0x4cb2, 0x4405, /*  3a0 */
  0x0010, 0x4004, 0x654a, 0x9810, 0x0000, 0x0144, 0xa54a, 0x1bd1, /*  3a8 */
  0x0006, 0x0413, 0x3301, 0xc444, 0x687e, 0x2005, 0xad76, 0x8445, /*  3b0 */
  0x4ed6, 0x8784, 0x36f3, 0x64c2, 0xac72, 0x8785, 0x4ec2, 0xa443, /*  3b8 */
  0x3009, 0x2440, 0x3009, 0x2741, 0x36f3, 0xd80e, 0x36f1, 0xd813, /*  3c0 */
  0x36f1, 0x5806, 0x36f0, 0x9803, 0x36f0, 0x1801, 0x2000, 0x0000, /*  3c8 */
  0x36f2, 0x9814, 0x0007, 0x0001, 0x8ed2, 0x0006, 0x000e, 0x4c82, /*  3d0 */
  0x0024, 0x0000, 0x0024, 0x2000, 0x0005, 0xf5c2, 0x0024, 0x0000, /*  3d8 */
  0x0980, 0x2000, 0x0000, 0x6010, 0x0024, 0x000a, 0x0001, 0x0d00
};


uint8_t getPresetList(uint8_t* buf, uint16_t size1, uint8_t idx) {
bool done = idx >= numPresets;
  *buf = 0;
  uint16_t size = size1 - 1;//remaining buffer size
  char *s = (char *)(buf + 1); 
  while (!done) {
    String station = readStationfrompref(presetMap[idx]);
    if (size >= station.length() + 1) {
      Serial.println(station.c_str());
      strcpy(s, station.c_str());
      s = s + station.length() + 1;
      size = size - station.length() - 1;
      (*buf)++;
      done = (++idx >= numPresets);
    }
    else {
      done = true;
    }
  }
  if (idx >= numPresets)
    *buf += 128;
  return size1 - size;
}

uint8_t getStatus(uint8_t* buf, uint16_t size) {
  /* 
   *  buf[0] = volume
   *  buf[1] = volume.min
   *  buf[2] = volume.max
   *  buf[3] = current preset (in "compressed" list)
   *  buf[4] = number of presets
   *  buf[5..n] = radiotext (0 terminated) [if buf is big enough...]
   *  buf[n..m] = station name (0 terminated) [if buf is big enough...]
   */
uint16_t size1;
  if (size < 5)       // need at least 5 bytes
    return 0;
  buf[0] = ini_block.reqvol;
  buf[1] = odroidRadioConfig.volume.min;
  buf[2] = odroidRadioConfig.volume.max;
  buf[3] = presetPresent[ini_block.newpreset] - 1;
  buf[4] = numPresets;
  size1 = 5;
  if (size > size1) {
    const char *s = tftdata[TFTSEC_TXT].str.c_str();
    if (size > size1 + strlen(s)) {
      memcpy(buf + 5, s, strlen(s) + 1);
      size1 = size1 + strlen(s);
    }
    else {
      buf[5] = 0;
    }
    size1++; 
  }
  if (size > size1 + strlen(timetxt)) {
    memcpy(buf + size1, timetxt, strlen(timetxt) + 1);
    size1 = size1 + strlen(timetxt) + 1;
  }
  return size1;
}

void recvCbFunction(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char s[255];
  memcpy(s+1, data, data_len);
  s[data_len +1] = 0;
  Serial.printf("Got something from {%02X, %02X, %02X, %02X, %02X, %02X}, len=%d, content=%s\r\n",
    mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5],
       data_len, s+1);

}




ESPNowRadioServer::ESPNowRadioServer(const uint8_t* mac): ESPNowServiceHandlerClass(mac, 0x6161) {
         espnowWrapper.addServiceHandler(this);
         if (isBroadcastMac())
            StatemachineLooper.add(this);
         else
          espnowWrapper.addPeer(mac);
      };
  
void ESPNowRadioServer::flushRX(const uint8_t *mac, const uint8_t packetId, const uint8_t *data, const uint8_t dataLen) {
ESPNowRadioServer *clientHandler = this;
  if (this->isBroadcastMac()) {
    clientHandler = new ESPNowRadioServer(mac);
//    clientHandler->dataCopy(this);
    Serial.printf("New radio client: {%02X, %02X, %02X, %02X, %02X, %02X}\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
    Serial.printf("Ignore radio from broadcast. packetId = %d, Len = %d, Content = %s\r\n", 
                  packetId, dataLen, data); 
  } 
  Serial.printf("Handle radio client: {%02X, %02X, %02X, %02X, %02X, %02X}, packetId = %d, Len = %d, Content = %s\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                  packetId, dataLen, data);
  char tst[241];
  int len = 0;
  if (packetId < 100) {
    len = getPresetList((uint8_t *)tst, 240, packetId);
  } else if (packetId == 100) {
    len = getStatus((uint8_t *)tst, 240);
    Serial.printf("Prepared status reply, len=%d\r\n", len);
  } else if (packetId == 200) {
    if ((dataLen == 1) && (ini_block.newpreset != presetMap[data[0]]) && (data[0] < numPresets)) {
      char s[10];
      sprintf(s, "%d", presetMap[data[0]]);
      analyzeCmd("preset", s);
      String str = readStationfrompref(presetMap[data[0]]);
      tftset(TFTSEC_TXT, str);
  //    tftset(TFTSEC_BOT, str);

    }
  } else if (packetId == 201) {
    if ((dataLen == 1) && (ini_block.reqvol != data[0]) && (data[0] >=  odroidRadioConfig.volume.min) && (data[0] <= odroidRadioConfig.volume.max)) {
      char s[10];
      sprintf(s, "%d", data[0]);
      analyzeCmd("volume", s);
    }
  } else if (packetId == 202) {
    strcpy(tst, NAME);
    len = strlen(NAME) + 1;
  }
  if (len > 0)
    clientHandler->setTX((const uint8_t *)tst, len, packetId, 3);
} ;

void ESPNowRadioServer::runState(int16_t stateNb) {
#define STATE_RADIOSERVER_RUN 1
/*  if (stateNb != STATE_INIT_NONE) {
    if (WL_CONNECTED == WiFi.status())
      espnowWrapper.loop();
    else {
      espnowWrapper.end();
      setState(STATE_INIT_NONE);
      return;
    }
  }*/
  switch(stateNb) {  
    case STATE_INIT_NONE:
      if (WL_CONNECTED == WiFi.status())
        if (ESPNOWWRAPPER_OK == espnowWrapper.begin(WiFi.channel())) {
          dbgprint("Radio listening for ESPNow-Clients on channel %d", espnowWrapper.channel());
          espnowWrapper.setRecvCb(recvCbFunction);
          setState(STATE_RADIOSERVER_RUN);
        }
      break;
    case STATE_RADIOSERVER_RUN:
      if (WL_CONNECTED == WiFi.status())
        espnowWrapper.loop();
      else {
        dbgprint("WiFi lost, ESPNow-Server stopped");
        espnowWrapper.end();
        setState(STATE_INIT_NONE);
      }
      break;
  }
}


ESPNowRadioServer espNowRadioServer(broadcastMac);

