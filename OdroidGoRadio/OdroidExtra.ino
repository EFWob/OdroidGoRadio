#include "src/libraries/BasicStatemachineOdroid.h"
#include "src/libraries/VirtualPinOdroid.h"
#include "src/libraries/ButtonHandlerOdroid.h"
#include "src/libraries/Display.h"
//#include <odroid_go.h>
#include "OdroidExtra.h"
#include "genre_html.h"
#include <Base64.h>
#include <set>

// VS1053 Plugins Spectrum Analyzer

size_t spectrumAnalyzerPluginSize = 1000;
extern const uint16_t spectrumAnalyzerPlugin[1000];

ILI9341 *tft = NULL;

/*
#define TFTSEC_BOT          2             // Index for Bottom text, normal play (usally station name)
#define TFTSEC_VOL          3             // Index for volume display on change (used also for new station name if preset has been pressed)
#define TFTSEC_FAV_BOT      4             // Index for showing current selection of favorite bank
//#define TFTSEC_FAV_STAT     4             // Index for showing status line at Bottom
#define TFTSEC_FAV          5             // Index for showing favorite stations in current bank
#define TFTSEC_LIST_CLR     6             // Index for clear text section
#define TFTSEC_LIST_HLP1    7             // Help text 1 for channel selection list
//#define TFTSEC_LIST_HLP2    8             // Help text 2 for channel selection list
#define TFTSEC_LIST_CUR     9             // current selection in list (Channel or Menu)
#define TFTSEC_LIST_TOP    11             // Top part of channel selection list (Channel or Menu)
#define TFTSEC_LIST_BOT    12             // Bottom part of channel selection list (Channel or Menu)
#define TFTSEC_FAV_BUT     13             // Favorite Channels select button display ("<1> <2>"... on top line below buttons) 
#define TFTSEC_MEN_TOP     14             // Top-Line Menu
#define TFTSEC_MEN_HLP1    15             // Help Text 1 Menu
#define TFTSEC_MEN_HLP2    16             // Help Text 2 Menu
#define TFTSEC_MENU_VAL    10             // updated Menu Item Value
#define TFTSEC_SPECTRUM    17             // Spectrum Analyzer
#define TFTSEC_FAV_BUT2    18             // Favorite Channels select button display ("<1> <2>"... on top line below buttons in not Flipped Display) 
*/

#define TFTSEC_ITEM_SELECT  7
//#define TFTSEC_LIST_CLR TFTSEC_TXT

scrseg_struct     tftdata[TFTSECS] =                        // Screen divided in 3 segments + 1 overlay
{                                                           // One text line is 8 pixels
  { false, WHITE,   2,  8, "", true,0 },                            // 1 top line (TFTSEC_TOP 0)
  { false, CYAN,   16, 64, "", true,0 },                            // 8 lines in the middle (TFTSEC_TXT 1)
  { true, YELLOW, 86, 16, "", true,0 },                            // 2 lines at the bottom (TFTSEC_BOT 2)
  { false, WHITE,  86, 16, "", true,0 },                            // 2 lines at bottom for volume/new selected preset (TFTSEC_VOL 3)
  { true, WHITE,  106, 8, "Favorites: A1", true,0 },                // 1 lines at bottom for selected Favorites (TFTSEC_FAV_BOT 4)
  { false, TFT_GREENYELLOW,   16, 64, "", true,0 },                 // 8 lines in the middle for showing favorites (TFTSEC_FAV 5)
    // identical to TFTSEC_TXT, except color)                       
  { false, WHITE,   16, 64, "", false,0 },                            // 8 lines in the middle to clear channel selection list (TFTSEC_LIST_CLR 6)
    // identical to TFTSEC_TXT, except color)                       
  {false, WHITE, 86, 16, "<UP>/<DOWN> to select\n<LEFT>/<RIGHT> cancel/play", true,0}, // (TFTSEC_LIST_HLP1 7)
    // identical to TFTSEC_BOT, except color & static text!
  {false, WHITE, 106, 8, "OBSOLETE", true,0},                         
  {false, YELLOW, 48, 8, "Station x", true,0},
  {false, WHITE, 48, 8, "", true,20},                                 // updated menu item 
  {false, 0xA510, 16, 24, "Station x-2\nStation x-1", true,0},
  {false, 0xA510, 64, 16, "Station x+1\nStation x+2", true,0},
  { false, WHITE,   2,  8, "<1>    <2>       <3>   <4>", true,0 },                            // 1 top line
  { false, WHITE,   2,  8, "MENU                      ", true ,0},                             // 1 top line MENU
  {false, WHITE, 86, 16, "<UP>   /  <DOWN> to select\n<LEFT> / <RIGHT> to change", true,0},
  {false, WHITE, 106, 8, "<A> to Save  <B> to Cancel", true,0},
  {false, TFT_ORANGE, 16, 64, "  ** SPECTRUM ANALYZER **", true, 0},
  { false, WHITE,   106,  8, "<1>    <2>       <3>   <4>", true,0 },                            // Preset Keys in Upside-Mode
  {false, WHITE, 106, 8, "<A> for channels", true,0}
  
} ;



#define GENRELOOPSTATE_INIT     0
#define GENRELOOPSTATE_CACHING  1
//#define GENRELOOPSTATE_DONE     2

#define GENRELOOPSTATE_DONE     0x8000


#define CHANNEL_1     1
#define CHANNEL_2     2
#define CHANNEL_3     3
#define CHANNEL_4     4
#define CHANNEL_UP    5
#define CHANNEL_DOWN  6
#define CHANNEL_DRAW  7

String genreChannel(int id);


extern char*       dbgprint( const char* format, ... ) ;
extern bool time_req;
extern String nvsgetstr ( const char* key );
int genreId = 0;
int genrePresets = 0;
int genrePreset = 0;
int genreChannelOffset = 0;
char *genreSelected = NULL;
String genreStation;
uint32_t spectrumBlockTime = 0;

int listSelectedPreset = 0;

int theMonkey = 0;

struct cstrless {
    bool operator()(const char* a, const char* b) {
      if ((*a >= 'A') && (*a <= 'Z'))
      {
        if ((*b != 0) && !((*b >= 'A') && (*b <= 'Z')))
          return true;
      }
      else if ((*b >= 'A') && (*b <= 'Z'))
        if (*a)
          return false;
      return strcmp(a, b) < 0;
    }
};

int genreListId = -1;
char *genreListBuffer = NULL;
std::set<const char*, cstrless> genreList;

uint16_t genreLoopState = GENRELOOPSTATE_INIT;




class RadioMenu;
bool nvssetOdroid();
void genreLoop(bool reset = false);
void doGenre ( String param, String value );
void doGenreConfig ( String param, String value );
int searchGenre(const char * name);
bool playGenre(int id);
String split(String& value, char delim);
bool canAddGenreToGenreId(String idStr, int genreId);
int handleBtnY(int direction, uint16_t repeats = 0);



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


void doMonkeyStart(int ivalue)
{
  theMonkey = ivalue;
}


void monkeyRun() {
  static uint32_t lastAction = 0;
  if (theMonkey)
    if (1 == theMonkey)
    {
      if (genreId)
        if (millis() - lastAction > 2000)
          if (!random(10))
            {
            String s = String("--id ") + String(1 + random(genreList.size()));
            doGenre("genre", s);
            lastAction = millis();
            }
    }
}

void drawPresetList(int channelX)
{
      String str = readStationfrompref(channelX);
      tftset(TFTSEC_LIST_CUR, str);
      int channel;
      listSelectedPreset = channelX;
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

void drawGenreList(int delta = 0)
{
String str;
std::set<const char*>::iterator it = genreList.begin();
std::set<const char*>::iterator current;
      if (NULL != genreSelected)
        it = genreList.find(genreSelected);
      else if (delta != 0)
        return;
      current = it;
      if (delta == 1)
      {
        ++it;
        if (it == genreList.end())
        {
          it = current;
          delta = 0;
        }
      }
      else if (delta == -1)
      {
        if (it != genreList.begin())
          --it;
        else 
          delta = 0;
      }
      if (delta)
      {
        if (genreSelected)
          free(genreSelected);
        genreSelected = strdup(String(*it).c_str());
      }
      if (it != genreList.end())
      {
        str = String(*it).substring(0, 25);
        tftset(TFTSEC_LIST_CUR, str);
        if (genreSelected)
          free(genreSelected);
        genreSelected = strdup(*it);
        current = it;
        if (it != genreList.begin())
        {
          --it;
          str = String(*it).substring(0, 25);
          if (it != genreList.begin())
          {
            --it;
            str = String(*it).substring(0, 25) + "\n" + str;
            if (it != genreList.begin())
            {
              --it;
              str = String(*it).substring(0, 25) + "\n" + str;              
            }
            else
              str = String("\n") + str;
          }
          else
            str = String("\n\n") + str;
        }
        else
          str = "\n\n\n";
        tftset(TFTSEC_LIST_TOP, str);
      }
      else
      {
        
      }
      it = current;
      str = String("");
      ++it;
      if (it != genreList.end())
      {
        str = String(*it).substring(0, 25);
        ++it;
        if (it != genreList.end())
          str = str + String("\n") + String(*it).substring(0, 25);  
      }
      tftset(TFTSEC_LIST_BOT, str);
}




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

#define BOTTOMLINE_NORMAL 1
#define BOTTOMLINE_GENRE_START 2
#define BOTTOMLINE_GENRE_SECOND 3
#define BOTTOMLINE_GENRE_CHANGE 4
#define BOTTOMLINE_LIST_PRESETS 5
#define BOTTOMLINE_LIST_GENRE   6
#define BOTTOMLINE_NO_GENRES    7
#define BOTTOMLINE_NO_GENRES2   8
#define BOTTOMLINE_NO_GENRES3   9
#define BOTTOMLINE_HAVE_GENRES  10



class BottomLineStatemachine: public BasicStatemachine {
  public:
    BottomLineStatemachine();
  protected:
    String _display;
    int _lastGenre = 0;
    int _idx = 0;
    void onEnter(int16_t currentStatenb, int16_t oldStatenb);
    void runState(int16_t stateNb);
  private:
    void scrolltext();
} bottomLineStatemachine;


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



#define RADIOSTATE_RUN        1     // Normal running (No menues etc)          
#define RADIOSTATE_VOLUME     2     // Showing volume (in addition to normal running)
#define RADIOSTATE_PRESETS    3
#define RADIOSTATE_LIST       4     // List of presets to select from
#define RADIOSTATE_PRESET_SET 5
#define RADIOSTATE_LIST_SET   6     // Preset chosen from list (state to avoid volume change)
#define RADIOSTATE_MENU       7 
#define RADIOSTATE_MENU_DONE  8     
#define RADIOSTATE_INICONFIG  9     // No WiFi found, AP is open for initial config
#define RADIOSTATE_INIVS     10     // VS1053 not initialized, normal operation not possible
#define RADIOSTATE_GENRE     11     // List of genres to select from
//#define RADIOSTATE_GENRE_SET 12     // Genre chosen from list (state to avoid volume change)
#define NUM_RADIOSTATES      11

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
        //Serial.printf("current menu display string is: %s\r\n", str.c_str());
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
          /*
          sprintf(s, "Favorites: %c%c", 'A' + (favoriteGroup / _favBanks->value()), '1' + (favoriteGroup % _favBanks->value()));
          dbgprint("set favorites: %s ", s);
          tftset(TFTSEC_FAV_STAT, s); 
          */
          bottomLineStatemachine.setState(BOTTOMLINE_NORMAL);
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
  if (group > 1)
    return;
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
  if (RADIOSTATE_LIST == radioState)
  {
   if ((group == 1) && !genres.config.disable())
    {
      if (-1 != genreListId)
      {
        radioStatemachine.setState(RADIOSTATE_GENRE);
        bottomLineStatemachine.setState(BOTTOMLINE_LIST_GENRE);
      }
    }
    return;
  }
  if (RADIOSTATE_GENRE == radioState)
  {
    if (group == 0)
    {
      radioStatemachine.setState(RADIOSTATE_LIST);
      bottomLineStatemachine.setState(BOTTOMLINE_LIST_PRESETS);
      handleBtnY(0);
    }
    return;
  }
  
  if ((RADIOSTATE_PRESETS != radioState) && (RADIOSTATE_PRESET_SET != radioState)) {
      if (genreId)
        genreChannel(CHANNEL_DRAW);
      else
        if ((favoriteGroup / FAVORITE_BANKS) != group)
        {
          favoriteIndex[group] = 0;    
        //
        }
  }else {
      if (genreId)
      {
        if (group)
          genreChannel(CHANNEL_UP);
        else
          genreChannel(CHANNEL_DOWN);
      }
      else
      {
        if ((favoriteGroup / FAVORITE_BANKS) == group) 
          favoriteIndex[group] = (1 + favoriteIndex[group]) % FAVORITE_BANKS;    
      }
  }

  if ( 0 == genreId)
  {
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
  if (radioState != RADIOSTATE_PRESETS)
  {
    radioStatemachine.setState(RADIOSTATE_PRESETS);
    //bottomLineStatemachine.setState(BOTTOMLINE_NORMAL);
  }
  else
    radioStatemachine.resetStateTime();
}



String genreChannel(int id)
{
  String ret = "";
  if ((id >= CHANNEL_1) && (id <= CHANNEL_4))
  {
    int channel = id - CHANNEL_1;
    // set a new channel
    int delta = genrePresets / 4;
    if (0 == delta)
      delta = 1;
    channel = (channel * delta + genreChannelOffset) % genrePresets;
    doGpreset(String(channel));
  }
  else if ((id == CHANNEL_UP) || (id == CHANNEL_DOWN))
  {
    if (id == CHANNEL_UP)
      genreChannelOffset = (genreChannelOffset + 1) % genrePresets;
    else
    {
      genreChannelOffset--;
      if (genreChannelOffset < 0)
        genreChannelOffset = genrePresets - 1;
    }
    id = CHANNEL_DRAW;
  }
  if (id ==  CHANNEL_DRAW)
  {
    char s[200];
    String str = "\n";
    int delta = genrePresets / 4;
    if (0 == delta)
      delta = 1;
    for(int i = 1;i < 5;i++) {
      String line = genres.getUrl(genreId, (genreChannelOffset + delta * (i - 1)) % genrePresets, false);
      const char *p = strchr(line.c_str(), '#');
      if (p)
        if (0 < strlen(++p))
          line = String(p);
      //line = String((genreChannelOffset + delta * (i - 1)) % genrePresets) + String(" ") + line;
      if (line.length() > 21)
        line = line.substring(0, 21) + ">";
      str = str  + String("<") + String(i) + String("> ") + line + String("\n");
    }

    tftset(TFTSEC_FAV, str);
    
  }
  return ret;
}

void handleBtnMemory(int id) {
int radioState = radioStatemachine.getState();
String str;
    if (!odroidRadioConfig.misc.flipped)
      id = 5 - id;
    if ((0 == id) || (RADIOSTATE_INIVS == radioState) || (RADIOSTATE_INICONFIG == radioState)) 
      return;     
    else if ((RADIOSTATE_MENU == radioState) && (id < 5)) {
        radioStatemachine.startMenu(id);
        return;
      }
      else if ((RADIOSTATE_LIST != radioState) && (RADIOSTATE_GENRE != radioState)) {    
        if (genreId)
        {
          genreChannel(CHANNEL_1 + id - 1);
          str = genreStation;
        }
        else
        {
          bool newPreset;
          char s[100];
          id = id - 1 + 4 * favoriteGroup;
          str = readStationfrompref(id); 
          if (newPreset = (id != ini_block.newpreset)) {
            tftset(TFTSEC_TXT, str);
            tftset(TFTSEC_BOT, str);
            sprintf(s, "%d", id);
            analyzeCmd("preset", s); 
          }
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


int handleBtnY(int direction, uint16_t repeats) {
int16_t radioState = radioStatemachine.getState();  
static int channelX;
int channelY;
  if (!direction)
    return channelX;
  if (!odroidRadioConfig.misc.flipped)
    direction = -direction;
  if (RADIOSTATE_MENU == radioState) {
    radioStatemachine.menuButton((1 == direction)?MENU_DN:MENU_UP, repeats);
  } else if ((RADIOSTATE_RUN == radioState) || (RADIOSTATE_LIST == radioState) || (RADIOSTATE_GENRE == radioState) ||
            (RADIOSTATE_PRESETS == radioState) || (RADIOSTATE_PRESET_SET == radioState)
            || (RADIOSTATE_VOLUME == radioState)) {
    if (RADIOSTATE_LIST == radioState) {
      if (0 != (repeats % KEY_SPEED))
        return channelX;
      radioStatemachine.resetStateTime();
      channelY = getChannelRelativeTo(channelX, direction);
    }
    else if (RADIOSTATE_GENRE == radioState)
    {
       genres.dbgprint("Button Y[dir: %d, repeats: %d]  in Genre mode!", direction, repeats);
      if (0 != (repeats % KEY_SPEED))
        return channelX;

       drawGenreList(direction);
       radioStatemachine.resetStateTime();
       return -1;
    }
    else if ((genreId != 0) && (direction != 0))
    {
      radioStatemachine.setState(RADIOSTATE_GENRE);
      bottomLineStatemachine.setState(BOTTOMLINE_LIST_GENRE);
      return -1;
    }
    else
    {
      channelX = channelY = getChannelRelativeTo(ini_block.newpreset, 0);
      radioStatemachine.setState(RADIOSTATE_LIST);
      bottomLineStatemachine.setState(BOTTOMLINE_LIST_PRESETS);
    }   
    if ((channelY != 100) && (channelX != channelY)) {
//      Serial.println(String("New Channel(x): ") + channelY);
      int channel = channelY;
      channelX = channelY;
      drawPresetList(channelX);
#if defined(OBSOLETE)
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
#endif      
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
    } else if ((RADIOSTATE_LIST == radioState) || (RADIOSTATE_GENRE == radioState)) {
        radioStatemachine.resetStateTime();
        if (1 == dir) 
        {
          if (RADIOSTATE_LIST == radioState)
          {
            int channel =  handleBtnY(0);
            if ((channel != 100) && (repeats == 0)) 
            {
              char s[20];
              String str = readStationfrompref(channel);
              sprintf(s, "%d", channel);
              tftset(TFTSEC_TXT, str);
              tftset(TFTSEC_VOL, str);
              tftset(TFTSEC_BOT, str);
              tftdata[TFTSEC_VOL].hidden = false;
              bottomLineStatemachine.setState(BOTTOMLINE_NORMAL);
              playGenre(0);
              analyzeCmd("preset", s);
            }
          }
          else
          {
              if (genreSelected)
              {
                doGenre("genre", genreSelected);
                bottomLineStatemachine.setState(BOTTOMLINE_GENRE_START);
              }
              
            // Select Genre  
          }
          if (odroidRadioConfig.misc.closeListOnSelect)
              radioStatemachine.setState(RADIOSTATE_LIST_SET);  
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
      presetPresent[i] = nvssearch(key);
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
    dsp_println("Try to load Genres.");
    if (!genres.begin(false))
    {
      dsp_print("Failed, try to format LITTLEFS...");
      if (genres.begin(true))
        dsp_println("OK");
      else
      {
        dsp_println("FAIL!!");
        delay(3000);
      }
    }
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
          //sprintf(s, "Favorites: %c%d", 'A' + group, 1 + favoriteIndex[group]);
          //tftset(TFTSEC_FAV_STAT, s);
          bottomLineStatemachine.setState(BOTTOMLINE_NORMAL);
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

void odroidSetup2(void)
{
    dsp_fillRect ( 0, 8 + tftdata[0].y,                                  // Clear most of the screen
                   dsp_getwidth(),
                   dsp_getheight() - 8 - tftdata[0].y, BLACK ) ;
    dsp_setCursor ( 0, tftdata[1].y ) ;                           // Top of screen  
  while (!odroidRadioFirstLoopSetup())
    ;
}


void odroidLoop(void) {    
static bool first = true;
  if (first) {
    dbgprint("\n\nRunning First Loop Setup\n\n");
    first = odroidRadioFirstLoopSetup();
    dbgprint("First loop setup done!");
  } else {
    genreLoop();
    StatemachineLooper.run();
    claimSPI("spectrum");
    runSpectrumAnalyzer();
    releaseSPI();
    monkeyRun();
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
    dbgprint("Menu Button %d (repeats: %ld)\r\n", button, repeats);
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
     bit(TFTSEC_TOP)|bit(TFTSEC_LIST_CLR)|bit(TFTSEC_LIST_HLP1)|bit(TFTSEC_FAV_BOT)|
     bit(TFTSEC_LIST_CUR)|bit(TFTSEC_LIST_TOP)|bit(TFTSEC_LIST_BOT),                      // RADIOSTATE_LIST (Select station from list)
     bit(TFTSEC_FAV_BUT)|bit(TFTSEC_FAV)|bit(TFTSEC_VOL)|bit(TFTSEC_FAV_BOT),             // RADIOSTATE_PRSET_SET (After select from preset Bank)
     bit(TFTSEC_TOP)|bit(TFTSEC_TXT)|bit(TFTSEC_VOL)|bit(TFTSEC_FAV_BOT),                  // RADIOSTATE_LIST_SET (After selecting from channel list)
     bit(TFTSEC_MEN_TOP)|bit(TFTSEC_LIST_CLR)|bit(TFTSEC_MEN_HLP1)|bit(TFTSEC_MEN_HLP2)|
     bit(TFTSEC_LIST_CUR)|bit(TFTSEC_LIST_TOP)|bit(TFTSEC_LIST_BOT)|bit(TFTSEC_MENU_VAL),                      // RADIOSTATE_MENU (Menu active)
     bit(TFTSEC_TOP)|bit(TFTSEC_TXT)|bit(TFTSEC_BOT)|bit(TFTSEC_FAV_BOT),                 // RADIOSTATE_MENU_DONE (wait to release (A) or (B))
     bit(TFTSEC_MEN_TOP)|bit(TFTSEC_LIST_CLR),                                            // RADIOSTATE_INICONFIG
     bit(TFTSEC_MEN_TOP)|bit(TFTSEC_LIST_CLR),                                             // RADIOSTATE_INIVS
     bit(TFTSEC_TOP)|bit(TFTSEC_LIST_CLR)|bit(TFTSEC_LIST_HLP1)|bit(TFTSEC_FAV_BOT)|
     bit(TFTSEC_LIST_CUR)|bit(TFTSEC_LIST_TOP)|bit(TFTSEC_LIST_BOT)                      // RADIOSTATE_GENRE (Select station from list)
     //bit(TFTSEC_TOP)|bit(TFTSEC_TXT)|bit(TFTSEC_VOL)|bit(TFTSEC_FAV_BOT),                  // RADIOSTATE_GENRE_SET (After selecting from channel list)
    };

char *radiostateNames[] = {"Start", "Normal operation", "Show Volume", "Show Presets", "Show List", "Preset set", "List Done", 
                           "Menu", "Menu Done", "InitConfig", "ErrorVS1053", "Genre List", "Genre List done"};
      dbgprint("RadioState: %d (%s)", currentStatenb, radiostateNames[currentStatenb]);
      bool ignoreSpectrumAnalyzerSection = false;
      tftdata[TFTSEC_TXT].color = CYAN;
      dsp_setRotation();
      if ((RADIOSTATE_PRESET_SET == currentStatenb) 
            || (RADIOSTATE_LIST_SET == currentStatenb))
        spectrumBlockTime = millis();
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
    if (RADIOSTATE_RUN == currentStatenb)
    {
        tftset(TFTSEC_TOP, "ODROID-GO-Radio");
    }
    if ((RADIOSTATE_RUN == currentStatenb) || (RADIOSTATE_VOLUME == currentStatenb) || (RADIOSTATE_MENU_DONE == currentStatenb) || (RADIOSTATE_LIST_SET == currentStatenb)) {
//  These are the states where Spectrum Analyzer (if switched On) or else RadioText is visible
      screenSections[currentStatenb-1] = bit(TFTSEC_TOP)|bit(TFTSEC_TXT)|
            (RADIOSTATE_VOLUME == currentStatenb?bit(TFTSEC_VOL):bit(TFTSEC_BOT))|bit(TFTSEC_FAV_BOT);          // "default" with RadioText
      if (genreId)
        bottomLineStatemachine.setState(BOTTOMLINE_GENRE_START);
      else
        bottomLineStatemachine.setState(BOTTOMLINE_NORMAL);
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
    } 
    else if (RADIOSTATE_LIST == currentStatenb)
    {
        tftset(TFTSEC_TOP, "Preset List");
        drawPresetList(getChannelRelativeTo((RADIOSTATE_GENRE == oldStatenb?listSelectedPreset:ini_block.newpreset), 0));
    }
    else if (RADIOSTATE_GENRE == currentStatenb)
    {
        tftset(TFTSEC_TOP, "Genre List");
        drawGenreList();
    }
    
    else if (RADIOSTATE_INICONFIG == currentStatenb) {
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
      case RADIOSTATE_GENRE:
        if (LIST_SHOWTIME < getStateTime())
          setState(RADIOSTATE_RUN);
        break;
      case RADIOSTATE_PRESETS:
      case RADIOSTATE_PRESET_SET:
        if (PRESET_SHOWTIME + (((stateNb == RADIOSTATE_PRESETS) && (0 != genreId))?PRESET_SHOWTIME/2:0) < getStateTime())
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



BottomLineStatemachine::BottomLineStatemachine():BasicStatemachine() {
    StatemachineLooper.add(this);      
}

void BottomLineStatemachine::onEnter(int16_t currentStatenb, int16_t oldStatenb) {
char buf[50 + (genreId?genres.getName(genreId).length():0)];
const char *s = (const char *)&buf;  

  switch(currentStatenb)
  {
    case BOTTOMLINE_NORMAL:
      _lastGenre = 0;
      sprintf(buf, "Favorites: %c%c", 'A' + (favoriteGroup / FAVORITE_BANKS), '1' + (favoriteGroup % FAVORITE_BANKS));
      tftdata[TFTSEC_FAV_BOT].color = WHITE;
      break;
    case BOTTOMLINE_GENRE_CHANGE:
      _lastGenre = genreId;
      s = NULL;
      break;
    case BOTTOMLINE_GENRE_START:  
      tftdata[TFTSEC_FAV_BOT].color = WHITE;
      _lastGenre = genreId;
      sprintf(buf, "Genre '%s' (station %d of %d)", genres.getName(genreId).c_str(), genrePreset, genrePresets);
      break;
    case BOTTOMLINE_GENRE_SECOND:
        tftdata[TFTSEC_FAV_BOT].color = TFT_GREENYELLOW;
        s = NULL;  
      break;
    case BOTTOMLINE_NO_GENRES:
        tftdata[TFTSEC_FAV_BOT].color = YELLOW;
        strcpy(buf, "No Genres loaded!");
      break;
    case BOTTOMLINE_NO_GENRES2:
        strcpy(buf, (String("http://") + ipaddress + String("/genre.html")).c_str());
      break;
    case BOTTOMLINE_LIST_PRESETS:
      tftdata[TFTSEC_FAV_BOT].color = YELLOW;
      if (genres.config.disable())
        strcpy(buf, "");
      else
      {
        if (genreLoopState & GENRELOOPSTATE_DONE)
        {
          if (genreListId >= 0)
          {
            setState(BOTTOMLINE_HAVE_GENRES);
            s = NULL;
          }
          else
          {
            s = NULL;
            setState(BOTTOMLINE_NO_GENRES);  
          }
        }
        else
          strcpy(buf, "Wait for Genres");
        
        }
      
      break;
    case BOTTOMLINE_HAVE_GENRES:
      strcpy(buf, "<B> for Genres");
      break;
    case BOTTOMLINE_LIST_GENRE:
      tftdata[TFTSEC_FAV_BOT].color = YELLOW;
      strcpy(buf, "<A> for Presets");
      break;  
    default:
      s = NULL;
  }
  if (s) {
    _display = String(s);
    _idx = 0;
    //String str = String(s).substring(0, 26);
    tftset(TFTSEC_FAV_BOT, _display);  
    tftdata[TFTSEC_FAV_BOT].update_req = true;              
    tftdata[TFTSEC_FAV_BOT].hidden = false;              
    //Serial.printf("Setting Bottom line to: '%s'", s);    
  }
}


void BottomLineStatemachine::scrolltext() {
  if (_display.length() > 26)
    if (getStateTime() > 500)
    {          
      _idx = (_idx + 1) % (_display.length() + 1);            
      String s = _display.substring(_idx, _idx + 26) + " " + _display;
      tftset(TFTSEC_FAV_BOT, s);  
      resetStateTime();
    }
}

void BottomLineStatemachine::runState(int16_t stateNb){
    //Serial.printf("StateNb: %d, GenreId: %d, LastGenre: %d\r\n", stateNb, genreId, _lastGenre);
    /*
    if ((genreId == 0) && (stateNb != BOTTOMLINE_NORMAL))
      setState(BOTTOMLINE_NORMAL);
    else if ((genreId != 0) && (_lastGenre != 0) && (_lastGenre != genreId))
      setState(BOTTOMLINE_GENRE_CHANGE);
    else
    */ 
      switch(stateNb) 
      {
        case BOTTOMLINE_NORMAL:
          break;
        case BOTTOMLINE_GENRE_START:
          if (getStateTime() > 3000)
            setState(BOTTOMLINE_GENRE_SECOND);
          break;
        case BOTTOMLINE_GENRE_CHANGE:
          setState(BOTTOMLINE_GENRE_START);
          break;
        case BOTTOMLINE_GENRE_SECOND:
          scrolltext();
          break;
        case BOTTOMLINE_LIST_PRESETS:
          break;
        case BOTTOMLINE_LIST_GENRE:
          break;
        case BOTTOMLINE_NO_GENRES:
          if (getStateTime() > 1000)
            setState(BOTTOMLINE_NO_GENRES2);
          break;
        case BOTTOMLINE_NO_GENRES2:
          scrolltext();
          /*
          if (getStateTime() > 500)
            {
                _idx = (_idx + 1) % _display.length();            
                String s = _display.substring(_idx, _idx + 26);
                if (s.length() < 26)
                  setState(BOTTOMLINE_NO_GENRES3);
                else
                {
                  tftset(TFTSEC_FAV_BOT, s);  
                  resetStateTime();
                }
            }
          */
          break;
        case BOTTOMLINE_NO_GENRES3:
          if (getStateTime() > 1000)
            if (-1 == genreListId)
              setState(BOTTOMLINE_NO_GENRES);
            else
              setState(BOTTOMLINE_HAVE_GENRES);
          break;
        case BOTTOMLINE_HAVE_GENRES:
          if (getStateTime() > 2000)
            setState(BOTTOMLINE_NO_GENRES2);
          break;  
        default:
          setState(BOTTOMLINE_NORMAL);
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

void VS1053::loadUserCode(const uint16_t* plugin, size_t pluginSize) {
  int i = 0;
  while (i < pluginSize) {
    uint16_t addr, n, val;
    addr = plugin[i++];
    n = plugin[i++];
    if (n & 0x8000U) {/* RLE run, replicate n samples */
      n &= 0x7FFF;
      val = plugin[i++];
      while (n--) {
        write_register(addr, val);
      }
    } else { /* Copy run, copy n samples */
      while (n--) {
        val = plugin[i++];
        write_register(addr, val);
      }
    }
  }
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
  
      //claimSPI("spectrum");
      readBands();
      //releaseSPI();
      _lastRun = millis();
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
    if (spectrumBlockTime)
    {
      if (millis() - spectrumBlockTime > 1500)
        spectrumBlockTime = 0;
    }
    else
      if (!(spectrumAnalyzer.showText()?tftdata[TFTSEC_TXT].update_req:tftdata[TFTSEC_SPECTRUM].update_req)){      
      // and this only after the display area has been cleared (and thus deleted the previous overlaying content)
        spectrumAnalyzer.run1();  
    }
}

bool SpectrumAnalyzer::readBands() {
uint8_t bands;
static bool readSuccess = true;
     _drawn = 0;
        vs1053player->write_register(SCI_WRAMADDR, _base+2);
        bands = vs1053player->read_register(SCI_WRAM);
        if (bands <= _bands) {
          vs1053player->write_register(SCI_WRAMADDR, _base+4);
          int8_t skip = (_bands - bands) / 2;
          for (int8_t i = 0; i < _bands; i++) {
            if ((i >= skip) && (i - skip < bands ))
              _spectrum[i][0] = vs1053player->read_register(SCI_WRAM) & 0x1f;
            else
            {
              if (_spectrum[i][0])  
                _spectrum[i][0]--;  
            }
            _spectrum[i][3] = 0;
            
          }
        } else {
          if (readSuccess)
            dbgprint("Spectrum analyzer read failed! Bands: %d", bands);
          readSuccess = false;
          for (uint8_t i = 0; i < _bands; i++) {
            if (_spectrum[i][0])  
              _spectrum[i][0]--;  
            _spectrum[i][3] = 0;
          }
        }
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


#define BASE64PRINTER_BUFSIZE       1500        // multiple of 3!!!!

class Base64Printer: public Print {
public:  
  Base64Printer(Print& myPrinter) : _myPrinter(myPrinter), _step(0) {
  };

  ~Base64Printer() {
    done();
  }

  void done() {
    if (_step !=0 )
    {
      size_t l = Base64.encodedLength(_step);
      uint8_t encodedString[l];
      Base64.encode((char *)encodedString, (char *)_buf, _step);
      _myPrinter.write(encodedString, l);
      _step=0;
    }
  };

  size_t write(uint8_t byte) {
    //Serial.write(byte);
    _buf[_step++] = byte;
    if (_step == BASE64PRINTER_BUFSIZE) {
      size_t l = Base64.encodedLength(BASE64PRINTER_BUFSIZE);
      uint8_t encodedString[l];
      Base64.encode((char *)encodedString, (char *)_buf, BASE64PRINTER_BUFSIZE);
      _myPrinter.write(encodedString, l);
      _step = 0;
    }
    return 1;
  };

protected:
  Print& _myPrinter;  
  static const int bufsize;
  uint8_t _buf[BASE64PRINTER_BUFSIZE];
  uint16_t _step;
};



String urlDecode(String &SRC) {
    String ret;
    //char ch;
    int i, ii;
    for (i=0; i<SRC.length(); i++) {
        if (SRC.c_str()[i] == 37) {
            sscanf(SRC.substring(i+1,i + 3).c_str(), "%x", &ii);
            //ch=static_cast<char>(ii);
            //doprint("urldecode %%%s to %d", SRC.substring(i+1,i + 3).c_str(), ii);
            ret = ret + (char)ii;
            i=i+2;
        } else {
            ret+=SRC.c_str()[i];
        }
    }
    return (ret);
}



void httpHandleGenre ( String http_rqfile, String http_getcmd ) 
{
String sndstr = "";
  http_getcmd = urlDecode (http_getcmd) ;
  //doprint("Handle HTTP for %s with ?%s...", http_rqfile.c_str(), http_getcmd.substring(0,100).c_str());
  if (http_rqfile == "genre.html")
  {
    sndstr = httpheader ( String ("text/html") );
    cmdclient.println(sndstr);
    //cmdclient.println(genre_html);
    const char *p = genre_html;
    int l = strlen(p);
    while ( l )                                       // Loop through the output page
    {
      if ( l <= 1024 )                              // Near the end?
      {
        cmdclient.write ( p, l ) ;                    // Yes, send last part
        l = 0 ;
      }
      else
      {
        cmdclient.write ( p, 1024 ) ;         // Send part of the page
        p += 1024 ;                           // Update startpoint and rest of bytes
        l -= 1024 ;
      }
    }
    cmdclient.println("\r\n");                        // Double empty to end the send body
  }
  else if (http_rqfile == "genredir")
  {
    Base64Printer base64Stream(cmdclient);
    genres.dbgprint("Sending directory of loaded genres to client");
    sndstr = httpheader ( String ("text/json") ) ;
    cmdclient.print(sndstr);
    //dirGenre (&cmdclient, true) ;
    //genres.lsJson(cmdclient);
    genres.lsJson(base64Stream);
    base64Stream.done();
    cmdclient.println("\r\n\r\n");                        // Double empty to end the send body
  }
  else if (http_rqfile == "genre")
  {
    int genreId = http_getcmd.toInt();
    //bool dummy;
    http_rqfile = String("--id ") + genreId;
    genres.dbgprint("Genre switch by http:...%s (id: %d)", http_rqfile.c_str(), genreId);
    if (genreId > 0)
      doGenre("genre", http_rqfile);
    sndstr = httpheader("text/html") + "OK\r\n\r\n";
    cmdclient.println(sndstr);
  }
  else if (http_rqfile == "genreaction")
  {
    sndstr = httpheader ( String ("text/text")) ;
    cmdclient.println(sndstr);
    
    int decodedLength = Base64.decodedLength((char *)http_getcmd.c_str(), http_getcmd.length());
    char decodedString[decodedLength];
    Base64.decode(decodedString, (char *)http_getcmd.c_str(), http_getcmd.length());
    http_getcmd = String(decodedString);
    genres.dbgprint("Running HTTP-genreaction with: '%s'%s", 
      http_getcmd.substring(0, 100).c_str(), (http_getcmd.length() > 100?"...":""));
    //Serial.print("Decoded string is:\t");Serial.println(decodedString);    
    

    String command = split(http_getcmd, '|');
    String genre, idStr;
    int idx = command.indexOf("genre=");
    if (idx >= 0) 
    {
      String dummy = command.substring(idx + 6);
      genre = split(dummy, ',');
      genre.trim();
    }
    if (command.startsWith("link="))
    {
      sndstr = "OK";
    }    
    else if (/*command.startsWith("link") || */command.startsWith("link64="))
    {
      String dummy = command.substring(7);
      idStr = split(dummy, ',');
      idStr.trim();
      int genreId = idStr.toInt();
      //int genreId = searchGenre(genre.c_str());
      if (0 == genreId)
      {
        sndstr =  "ERR: Could not load genre '" + genre + "' to get links...";
      }
      else
      {
        /*
        char key[30];
        sprintf(key, "%d_x", genreId);
        sndstr = gnvsgetstr(key) + "\r\n\r\n"; */
        sndstr = genres.getLinks(genreId);// + "\r\n\r\n";
        //doprint("GenreLinkResult: %s", sndstr.c_str());
      }
      if (command.startsWith("link64"))
      {
        int encodedLength = Base64.encodedLength(sndstr.length() + 1);        
        char encodedString[encodedLength];
        Base64.encode(encodedString, (char *)sndstr.c_str(), sndstr.length());
        sndstr = String(encodedString);// + "\r\n\r\n";
      }  
    }
    else if (command.startsWith("createwithlinks"))
    {
      int genreId;
      if ((genreId =genres.createGenre(genre.c_str())))
      {
        genres.dbgprint("Created genre '%s', Id=%d, links given with len=%d (%s)",
                genre.c_str(), genreId, http_getcmd.length(), http_getcmd.c_str());
        genres.addLinks(genreId, http_getcmd.c_str());
      }
      else
        genres.dbgprint("Error: could not create genre '%s' (HTTP: createwithlinks", genre.c_str());
    }
    else if (command.startsWith("nvsgenres"))
    {
      sndstr = nvsgetstr("$$genres");
      chomp(sndstr);
      int encodedLength = Base64.encodedLength(sndstr.length() + 1);        
      char encodedString[encodedLength];
      Base64.encode(encodedString, (char *)sndstr.c_str(), sndstr.length());
      sndstr = String(encodedString);// + "\r\n\r\n";
    }
    else if (command.startsWith("del=") /*|| (command.startsWith("clr="))*/)
    {
      String dummy = command.substring(4);
      idStr = split(dummy, ',');
      idStr.trim();
      //bool deleteLinks = command.c_str() [0] == 'd' ;
      genres.dbgprint("HTTP is about to delete genre '%s' with id %d", genre.c_str(), idStr.toInt());
      //doDelete(idStr.toInt(), genre, deleteLinks, sndstr);
      genres.deleteGenre(idStr.toInt());
      genreLoop(true);
      //delay(2000);
      sndstr = "Delete done, result is:"  + sndstr;
      genres.dbgprint(sndstr.c_str());
      sndstr = "OK";//\r\n\r\n";
    }
    else if (command.startsWith("save=") || (command.startsWith("add=")))
    {
      int genreId;
      bool isAdd = command.startsWith("add=");
      bool isStart = false;
      const char *s;
      String dummy = command.substring(isAdd?4:5);
      idStr = split(dummy, ',');
      idStr.trim();
      int count=-1;int idx = -1;
      s = strstr(command.c_str(), "count=");
      if (s)
        count = atoi(s + 6);
      s = strstr(command.c_str(), "idx=");
      if (s)
        idx = atoi(s + 4);
      s = strstr(command.c_str(), "start=");
      if (s)
        isStart = atoi(s + 6);
      if (isStart)
      {
        if (0 < (genreId = genres.findGenre(genre.c_str())))
        {
            if (isAdd) 
            {
              if (genres.count(genreId) == 0)
              {
                genres.createGenre(genre.c_str(), true);
                genres.dbgprint("First deleting genre: '%s' (also deleting links=%d)", genre.c_str(), 1);
              }
            }
            else
            {
              genres.dbgprint("First deleting genre: '%s' (also deleting links=%d)", genre.c_str(), idStr == "undefined");
              genres.createGenre(genre.c_str(), idStr == "undefined");
            }
        }
        else
        {
            genres.dbgprint("Creating empty genre: '%s'.", genre.c_str());
            genres.createGenre(genre.c_str());
        }
      }
      genreId = genres.findGenre(genre.c_str());
      sndstr = "OK";//\r\n\r\n";
      if (genreId)
      {
        bool fail = false;
        if (isAdd && (idx == 0))                  // possibly a new subgenre to add to the main genre?
          if (!isStart || (genre != idStr))       // special case: genre name is identical to first subgenre
            if (!canAddGenreToGenreId(idStr, genreId))
            {
              sndstr = "Error: can not add genre " + idStr + " to genre " + genre ;
              genres.dbgprint(sndstr.c_str());
              //sndstr = sndstr + "\r\n\r\n";
              fail = true;
            }
            else
              genreLoop(true);
        if (!fail)
        {
          genres.dbgprint("Adding %d presets to genre '%s'.", count, genre.c_str());
          genres.addChunk(genreId, http_getcmd.c_str(), '|');
        }  
      } // if genreId
      else
      {
        sndstr = "Error: genre " + genre + " not found in Flash!";
        genres.dbgprint(sndstr.c_str());
        //sndstr = sndstr + "\r\n\r\n";
      }
    }
    else if (command.startsWith("setconfig="))
    {
      DynamicJsonDocument doc(JSON_OBJECT_SIZE(20));
      DeserializationError err = deserializeJson(doc, command.c_str() + strlen("setconfig="));
      if (err == DeserializationError::Ok)
      {
        const char* path = doc["path"];
        const char* rdbs = doc["rdbs"];
        int noname = doc["noname"];
        int verbose = doc["verbose"];
        int showid = doc["showid"];
        int save = doc["save"];
        int disable = doc["disable"];
        doGenreConfig("verbose", String(verbose));
        doGenreConfig("rdbs", String(rdbs));
//        doGenreConfig("path", String(path));
//        doGenreConfig("noname", String(noname));
        doGenreConfig("showid", String(showid));
        doGenreConfig("disable", String(disable));
        if (save)
          genres.config.toNVS();
      }
    }
    else
    {
      sndstr = "Unknown genre action '" + command + "'\r\n\r\n";
    }
    if (sndstr.lastIndexOf("\r\n\r\n") < 0)
      sndstr += "\r\n\r\n";
    if (sndstr.length() > 0) 
    {
        genres.dbgprint("CMDCLIENT>>%s", sndstr.substring(0,500).c_str());
        cmdclient.println(sndstr);
        cmdclient.flush();
        //delay(20);
    }
    //nvs_commit(gnvshandle);
  }
  else if (http_rqfile == "genrelist" )
  {
    Base64Printer base64Stream(cmdclient);
    genres.dbgprint("Sending directory of loaded genres to (remote) client");
    sndstr = httpheader ( String ("application/json") ) ;
    cmdclient.print(sndstr);
    //dirGenre (&cmdclient, true) ;
    //genres.lsJson(cmdclient);
    genres.lsJson(base64Stream, LSMODE_SHORT|LSMODE_WITHLINKS);
    base64Stream.done();
    cmdclient.println("\r\n\r\n");                        // Double empty to end the send body
  }
  else if (http_rqfile == "genreconfig")
  {
    sndstr = httpheader ( String ("application/json") ) ;
    cmdclient.print(sndstr);
    sndstr = genres.config.asJson();
    genres.dbgprint("Sending config: '%s'", sndstr.c_str());
    cmdclient.println(sndstr);
    cmdclient.println("\r\n\r\n");                        // Double empty to end the send body
  }
  else if (http_rqfile == "genreformat")
  {
    sndstr = httpheader ( String ("text/text") );
    cmdclient.println(sndstr);

    cmdclient.println("OK\r\n");
    if (genres.format(true))
      cmdclient.println("OK: LITTLEFS formatted for genre info");
    else
      cmdclient.println("Error: could not format LITTLEFS for genre info.");
    cmdclient.println();
    cmdclient.println();
  }
  else
  {
    sndstr = httpheader ( String ("text/text") );
    cmdclient.println(sndstr);
    cmdclient.printf("Unknown file?command request with %s?%s.\r\n\r\n\r\n", 
                        http_rqfile.c_str(), http_getcmd.c_str());                        // Double empty to end the send body
  }
  cmdclient.stop();
}


void doGenreConfig(String param, String value)
{
  bool ret = true;
  if (param == "rdbs")
    genres.config.rdbs(value.c_str());
//  else if (param == "noname")
//    genres.config.noName(value.toInt());
  else if (param == "showid")
    genres.config.showId(value.toInt());
  else if (param == "verbose")
    genres.config.verbose(value.toInt());
//  else if (param == "usesd")
//    genres.config.useSD(value.toInt());
//  else if (param == "path")
//    genres.nameSpace(value.c_str());
  else if (param == "store")
    genres.config.toNVS();
  else if (param == "disable")
    genres.config.disable(value.toInt());
  else if (param == "info")
    genres.config.info();
  else
    ret = false;
  if (!ret)
    genres.dbgprint("Unknown genre config parameter '%s', ignored!", param.c_str());
}

void doGenre(String param, String value)
{
  
  if (value.startsWith("--")) 
  {
    //static bool gverbosestore;
    const char *s = value.c_str() + 2;
    const char *remainder = strchr(s, ' ');
    if (remainder) 
    {
      param = String(s).substring(0, remainder - s) ;
      remainder++;
      value = String(remainder);
      chomp(value);
    }
    else
    {
      value = "";
      param = String(s);
    }
      genres.dbgprint("Genre controlCommand --%s ('%s')",
        param.c_str(), value.c_str());
    if (param == "id")
    {
      if (isdigit(value.c_str()[0]))
        playGenre(value.toInt());
      genres.dbgprint("Current genre id is: %d", genreId);
    }
    else if (param == "preset")
    {    
      doGpreset ( value );
    }
    else if (param.startsWith("verb"))
    {
      if (isdigit(value.c_str()[0]))
        genres.config.verbose(value.toInt());//gverbose = value.toInt();
      genres.dbgprint("is in verbose-mode.");
    }
    else if (param == "stop")
    {
      playGenre(0);
      genres.dbgprint("stop playing from genre requested.");
    } 
    else if (param == "deleteall")
    {
      genres.deleteAll();
    }
    else if (param == "create")
    {
      genres.createGenre(value.c_str());
    }
    else if (param == "delete")
    {
      genres.deleteGenre(value.toInt());
    }
    else if (param == "find")
    {
      int id = genres.findGenre(value.c_str());
      genres.dbgprint("Genre '%s'=%d", value.c_str(), id);
    }
    else if (param == "format")
    {
      genres.format(value.toInt());
    }
    /*
    else if (param == "add")
    {
      char *s = strchr(value.c_str(), ',');
      if (s)
        s = s+1;
      genres.add(value.toInt(), s);
    }
    */
    else if (param == "count")
    {
      int ivalue = value.toInt();
      genres.dbgprint("Number urls in Genre with id=%d is: %d", ivalue, genres.count(ivalue));
    }
    else if (param == "fill")
    {
      for (int i = 0;i < 1000;i++)
      {
        char key[50];
        sprintf(key, "genre%d", i);
        genres.createGenre(key);
      }
    }
    else if (param == "url")
    {
      uint16_t nb;
      char *s = strchr(value.c_str(), ',');
      if (s)
        nb = atoi(s+1);
      else
        nb = 0;
      genres.dbgprint("URL[%d] of genre with id=%d is '%s'", nb, value.toInt(), genres.getUrl(value.toInt(), nb).c_str());
    }
    else if (param == "addchunk")
    {
      const char *s = value.c_str();
      int id;
      if (1 == sscanf(s, "%d", &id))
      {
        int idx = strspn(s, "1234567890");
        if ((idx > 0) && (s[idx] != 0))
        {
            genres.dbgprint("AddChunk '%s' to genre=%d, delimiter='%c'", s + idx + 1, id, s[idx]);
            genres.addChunk(id, s + idx + 1, s[idx]);
        }
      }
    }
    else if (param == "ls")
    {
      genres.ls();
    }
    else if (param == "lsjson")
    {
      //const char *s = param.c_str() + 6;
      genres.dbgprint("List genres as JSON, full=%d", value.toInt());
      genres.lsJson(Serial, value.toInt());
    }
    else if (param == "test")
    {
      genres.test();
    }
  }
  else
  {
    if (value.length() == 0) 
    {
      doGenre("genre", "--stop");
    }
    else
    {
      int id = searchGenre(value.c_str());
      if (id)
        playGenre(id);
      else
      {
        genres.dbgprint ("Error: Genre '%s' is not known.", value.c_str()) ;
      }
    }
  }
  // else   gnvsTaskList.push(new GnvsTask(value.c_str(), GNVS_TASK_OPEN));
  return;

}

int searchGenre(const char * name)
{
  int ret = genres.findGenre(name);
  if (ret > 0)
  {
    if (genres.count(ret) == 0)
      ret = 0;
  }
  else
    ret = 0;
  return ret;
}



bool playGenre(int id)
{
  if (0 == id)
  {
    /*
    if (genreId != 0)
    {
      char s[30];
      sprintf(s, "Favorites: %c%c", 'A' + (favoriteGroup / FAVORITE_BANKS), '1' + (favoriteGroup % FAVORITE_BANKS));
      tftset(TFTSEC_FAV_BOT, s);       
    }
    */
    genreId = genrePresets = genrePreset = genreChannelOffset = 0;
    genres.dbgprint("Genre playmode is stopped");
    return true;
  }

  int numStations = genres.count(id);
  char cmd[50];
  if (numStations == 0)
    return false;
  
  genreId = id;
  genrePresets = numStations ;
  genreChannelOffset = genrePreset = random(genrePresets) ;
  if (genreSelected)
    free(genreSelected);
  genreSelected = NULL;
  genreSelected = strdup(genres.getName(id).c_str());
//  :::genreListId
  genres.dbgprint("Active genre is now: '%s' (id: %d), random start gpreset=%d (of %d)",
        genres.getName(id).c_str(), id, genrePreset, genrePresets);
//  sprintf(cmd, "Genre has %d stations", numStations);      
//  tftset(TFTSEC_FAV_BOT, cmd);
  //currentpreset = ini_block.newpreset = -1;
  sprintf(cmd, "gpreset=%d", genrePreset);
  bottomLineStatemachine.setState(BOTTOMLINE_GENRE_START);
  analyzeCmd(cmd);
  return true;
}


//**************************************************************************************************
//                                S P L I T                                                        *
//**************************************************************************************************
// Splits the input String value at the delimiter identified by parameter delim.                   *
//  - Returns left part of the string, chomped                                                     *
//  - Input String will contain the remaining part after delimiter (not altered)                   *
//  - will return the input string if delimiter is not found
//**************************************************************************************************
String split(String& value, char delim) {
  int idx = value.indexOf(delim);
  String ret = value;
  if (idx < 0) {
    value = "";
  } else {
    ret = ret.substring(0, idx);
    value = value.substring(idx + 1);
  }
  chomp(ret);
  return ret;
}

void doGpreset(String value)
{
  int ivalue = value.toInt();
  if (genreId != 0) 
  {
    if (ivalue >= genrePresets)
      ivalue = ivalue - genrePresets * (ivalue / genrePresets) ;
    else if (ivalue < 0)
      ivalue = ivalue + genrePresets * (1 + (-ivalue - 1) / genrePresets) ;

    if (ivalue < genrePresets) 
    {
      char key[20];
      String s, s1;
      int idx;
      sprintf(key, "%d_%d", genreId, ivalue);
      genrePreset = ivalue;
      //s = gnvsgetstr(key);
      s = genres.getUrl(genreId, ivalue, false);
      idx = s.indexOf('#');
      if (idx >= 0)
      {
        s1 = s.substring(idx + 1);
        s = s.substring(0, idx);
      }
      {
        if (idx >= 0)
        {
          genres.dbgprint("Switch to GenreUrl: '%s', Station name is: '%s'", s.c_str(), s1.c_str());
          genreStation = s1;
        }
        else
          {
          genres.dbgprint("Switch to GenreUrl: '%s', Station name is not on file!", s.c_str());
          genreStation = s;
          }
        tftset(TFTSEC_TXT, genreStation);
        genreStation = genreStation.substring(0, 42);
        tftset(TFTSEC_VOL, genreStation);
        tftset(TFTSEC_BOT, genreStation);
        tftdata[TFTSEC_VOL].hidden = false;
      }
      if (s.length() > 0)
      {
        char cmd[s.length() + 10];
        sprintf(cmd, "station=%s", s.c_str());
        //ini_block.newgenrestation = genreId;
        //ini_block.newgenrestation = (ini_block.newgenrestation << 16) + ivalue;
        ini_block.newpreset = currentpreset;
        bottomLineStatemachine.setState(BOTTOMLINE_GENRE_START);
        analyzeCmd(cmd);
        
//        host = s;
//        connecttohost();
      }
      else
      {
        genres.dbgprint("BUMMER: url not on file");

      }
    }
    else
    {
      genres.dbgprint("Id %d is not in genre:presets(%d)", ivalue);
    }
  }
  else
  {
    dbgprint("BUMMER: genrePreset is called but no genre is selected!");
  }
}




bool canAddGenreToGenreId(String idStr, int genreId)
{
  String knownLinks = genres.getLinks(genreId);
  if (knownLinks.length() == 0)
  {
    genres.addLinks(genreId, idStr.c_str());
    return true;
  }  
  do {
    if (idStr == split(knownLinks, ','))
      return false;
  } while (knownLinks.length() > 0);
  genres.addLinks(genreId, idStr.c_str());
  return true;
}

void genreLoop(bool reset)
{
  if (reset)
  {
    genreLoopState = GENRELOOPSTATE_INIT;
    if (genreListBuffer)
    {
      free(genreListBuffer);
      genreListBuffer = NULL;
    }
    genreList.clear();
    genreListId = -1;
  } 
  else if (0 == (genreLoopState & GENRELOOPSTATE_DONE)) {
    if (GENRELOOPSTATE_INIT == genreLoopState)
    {
      if (genres.cacheStep()) 
      {
        String list = genres.playList();
        if (list.length() >= 0)
        {
          genreListBuffer = (char *)genres.gmalloc(list.length() + 1, true);
          if (genreListBuffer)
          {
            char *p = genreListBuffer;
            memcpy(p, list.c_str(), list.length() + 1);
            while(p)
            {
              char *p1 = strchr(p, ',');
              if (p1)
              {
                *p1 = 0;
                p1++;
              }
              genreList.insert(p);
              //genres.dbgprint("genreList[%d]=%s", genreList.size(), p);
              p = p1;
            }
          }
        }
        genreLoopState = GENRELOOPSTATE_DONE;      
        genres.dbgprint("Have found %d genres", genreList.size());

        /*
        for(int i = 0;i < genreList.size();i++)
        {
          std::set<const char*>::iterator it = genreList.begin();
          std::advance(it, i);
          genres.dbgprint("Genre[%d]='%s'", i, *it);
        }
        */
        if (genreList.size())
          genreListId = 0;
        else
          genreListId = -1;
      }
    }
  }
}
