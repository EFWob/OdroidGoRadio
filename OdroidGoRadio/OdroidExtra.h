#ifndef __ODROIDEXTRA__H_
#define __ODROIDEXTRA__H_
//#include <odroid_go.h>
#include <ArduinoJson.h>
#include "src/libraries/Display.h"
#include "genres.h"
#define ODROIDRADIO_VERSION "202006182336"

#if defined(ARDUINO_M5STACK_FIRE)
#define ARDUINO_ODROID_ESP32
#endif

// Data to display.  There are TFTSECS sections
#define TFTSEC_TOP          0             // Index for Top line, normal play
#define TFTSEC_TXT          1             // Index for Radiotext, normal play
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
//#define TFTSEC_GLIST_HLP2  19             // Help text 2 for genre selection list


#define TFTSECS 20
#define DEBUG_BUFFER_SIZE 150

extern ILI9341 *tft;// = NULL;

struct scrseg_struct                                  // For screen segments
{
  bool     update_req ;                               // Request update of screen
  uint16_t color ;                                    // Textcolor
  uint16_t y ;                                        // Begin of segment row
  uint16_t height ;                                   // Height of segment
  String   str ;                                      // String to be displayed
#if defined(ARDUINO_ODROID_ESP32)
  bool     hidden;                                    // if set, update_req will not be set. 
  uint16_t x;                                         // x coordinate. Will be 0 normally
#endif
} ;


extern scrseg_struct     tftdata[TFTSECS];
#if 0
=                        // Screen divided in 3 segments + 1 overlay
{                                                           // One text line is 8 pixels
  { false, WHITE,   2,  8, "", true,2 },                            // 1 top line
  { false, CYAN,   16, 64, "", true,2 },                            // 8 lines in the middle
  { true, YELLOW, 86, 16, "", true,2 },                            // 2 lines at the bottom
  { false, WHITE,  86, 16, "", true,2 },                            // 2 lines at bottom for volume/new selected preset
  { true, WHITE,  106, 8, "Favorites: A1", true,2 },                // 1 lines at bottom for volume
  { false, TFT_GREENYELLOW,   16, 64, "", true,2 },                             // 8 lines in the middle for showing favorites

  { false, WHITE,   16, 64, "", false,2 },                            // 8 lines in the middle to clear channel selection list
  
  {false, WHITE, 86, 16, "<UP>/<DOWN> to select\n<LEFT>/<RIGHT> cancel/play", true,2},
  {false, WHITE, 106, 8, "<B> for genres", true,2},
  {false, YELLOW, 48, 8, "Station x", true,2},
  {false, WHITE, 48, 8, "", true,2},                                 // updated menu item 
  {false, 0xA510, 16, 24, "Station x-2\nStation x-1", true,2},
  {false, 0xA510, 64, 16, "Station x+1\nStation x+2", true,2},
  { false, WHITE,   2,  8, "<1>    <2>       <3>   <4>", true,2 },                            // 1 top line
  { false, WHITE,   2,  8, "MENU                      ", true ,2},                             // 1 top line MENU
  {false, WHITE, 86, 16, "<UP>   /  <DOWN> to select\n<LEFT> / <RIGHT> to change", true,2},
  {false, WHITE, 106, 8, "<A> to Save  <B> to Cancel", true,2},
  {false, TFT_ORANGE, 16, 64, "  ** SPECTRUM ANALYZER **", true, 2},
  { false, WHITE,   106,  8, "<1>    <2>       <3>   <4>", true,2 },                            // Preset Keys in Upside-Mode
  {false, WHITE, 106, 8, "<A> for channels", true,2}
  
} ;
#endif
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
extern esp_err_t nvssetstr(const char* key, String value);
extern char* dbgprint ( const char* format, ... );
extern void doGpreset(String value);
extern void doGenre(String param, String value);

extern int DEBUG;
#endif
