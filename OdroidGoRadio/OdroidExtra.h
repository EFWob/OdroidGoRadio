#ifndef __ODROIDEXTRA__H_
#define __ODROIDEXTRA__H_
//#include <odroid_go.h>
#include "./src/libraries/Display.h"
#include "./src/libraries/ESPNowWrapper.h"
#include "./src/libraries/BasicStatemachineOdroid.h"
#define ODROIDRADIO_VERSION "202006182336"
// Data to display.  There are TFTSECS sections
#define TFTSEC_TOP        0             // Index for Top line, normal play
#define TFTSEC_TXT        1             // Index for Radiotext, normal play
#define TFTSEC_BOT        2             // Index for Bottom text, normal play (usally station name)
#define TFTSEC_VOL        3             // Index for volume display on change (used also for new station name if preset has been pressed)
#define TFTSEC_FAV_BOT    4             // Index for showing current selection of favorite bank
#define TFTSEC_FAV        5             // Index for showing favorite stations in current bank
#define TFTSEC_LIST_CLR   6             // Index for clear text section
#define TFTSEC_LIST_HLP1  7             // Help text 1 for channel selection list
#define TFTSEC_LIST_HLP2  8             // Help text 2 for channel selection list
#define TFTSEC_LIST_CUR   9             // current selection in list (Channel or Menu)
#define TFTSEC_LIST_TOP  11             // Top part of channel selection list (Channel or Menu)
#define TFTSEC_LIST_BOT  12             // Bottom part of channel selection list (Channel or Menu)
#define TFTSEC_FAV_BUT   13             // Favorite Channels select button display ("<1> <2>"... on top line below buttons) 
#define TFTSEC_MEN_TOP   14             // Top-Line Menu
#define TFTSEC_MEN_HLP1  15             // Help Text 1 Menu
#define TFTSEC_MEN_HLP2  16             // Help Text 2 Menu
#define TFTSEC_MENU_VAL  10             // updated Menu Item Value
#define TFTSEC_SPECTRUM  17             // Spectrum Analyzer
#define TFTSEC_FAV_BUT2  18             // Favorite Channels select button display ("<1> <2>"... on top line below buttons in not Flipped Display) 


#define TFTSECS 19
ILI9341 *tft = NULL;
scrseg_struct     tftdata[TFTSECS] =                        // Screen divided in 3 segments + 1 overlay
{                                                           // One text line is 8 pixels
  { false, WHITE,   2,  8, "", true,0 },                            // 1 top line
  { false, CYAN,   16, 64, "", true,0 },                            // 8 lines in the middle
  { true, YELLOW, 86, 16, "", true,0 },                            // 2 lines at the bottom
  { false, WHITE,  86, 16, "", true,0 },                            // 2 lines at bottom for volume/new selected preset
  { true, WHITE,  106, 8, "Favorites: A1", true,0 },                // 1 lines at bottom for volume
  { false, TFT_GREENYELLOW,   16, 64, "", true,0 },                             // 8 lines in the middle for showing favorites

  { false, WHITE,   16, 64, "", false,0 },                            // 8 lines in the middle to clear channel selection list
  
  {false, WHITE, 86, 16, "<UP>/<DOWN> to select\n<RIGHT> play Station", true,0},
  {false, WHITE, 106, 8, "<LEFT> to cancel", true,0},
  {false, YELLOW, 48, 8, "Station x", true,0},
  {false, WHITE, 48, 8, "", true,0},                                 // updated menu item 
  {false, 0xA510, 16, 24, "Station x-2\nStation x-1", true,0},
  {false, 0xA510, 64, 16, "Station x+1\nStation x+2", true,0},
  { false, WHITE,   2,  8, "<1>    <2>       <3>   <4>", true,0 },                            // 1 top line
  { false, WHITE,   2,  8, "MENU                      ", true ,0},                             // 1 top line MENU
  {false, WHITE, 86, 16, "<UP>   /  <DOWN> to select\n<LEFT> / <RIGHT> to change", true,0},
  {false, WHITE, 106, 8, "<A> to Save  <B> to Cancel", true,0},
  {false, TFT_ORANGE, 16, 64, "  ** SPECTRUM ANALYZER **", true, 0},
  { false, WHITE,   106,  8, "<1>    <2>       <3>   <4>", true,0 }                            // Preset Keys in Upside-Mode
  
} ;

//VS1053_SPI = SPISettings ( 5000000, MSBFIRST, SPI_MODE0 ) ;

// Various macro's to mimic the ST7735 version of display functions
//#define dsp_setRotation()       tft->setRotation ( 3 )             // Use landscape format (3 for upside down)
void dsp_setRotation();
#define dsp_print(a)            tft->print ( a )                   // Print a string 
#define dsp_println(b)          tft->println ( b )                 // Print a string followed by newline

#define dsp_fillRect(a,b,c,d,e) tft->fillRect ( 2*(a), 2*(b), 2*(c), 2*(d), e )  // Fill a rectange
#define dsp_setTextSize(a)      tft->setTextSize(2*(a))                // Set the text size
#define dsp_setTextColor(a)     tft->setTextColor(a)               // Set the text color
#define dsp_setCursor(a,b)      tft->setCursor ( 2*(a), 2*(b) )            // Position the cursor
#define dsp_erase()             tft->clear() ;        // Clear the screen
#define dsp_getheight()         120                                // Get height of screen
#define dsp_getwidth()          160                                // Adjust to your display

#define dsp_update()                                               // Updates to the physical screen
#define dsp_usesSPI()           true                               // Does use SPI


bool dsp_begin();
void dsp_upsideDown();
void odroidLoop();
void runSpectrumAnalyzer();

class ESPNowRadioServer: public ESPNowServiceHandlerClass, public BasicStatemachine  {
    public:
      ESPNowRadioServer(const uint8_t* mac);
      virtual void flushRX(const uint8_t *mac, const uint8_t packetId, const uint8_t *data, const uint8_t dataLen);
      void runState(int16_t stateNb);
} ;

extern ESPNowRadioServer espNowRadioServer;

#endif
