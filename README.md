# Odroid-Go-Radio
## Caveat Emptor
This is beta SW. 

For testing, the SW runs in demo mode without a VS1053 module connected. All buttons are fully functional, but there will 
be no noticable audio output...

## Basic Idea
This is a project to port the ESP32-Radio by Edzelf (see https://github.com/Edzelf/ESP32-Radio) to the Odroid-Go to use the 
display and the buttons to control the radio.

It also features a very cool 14 channel spectrum analyzer (see Menu3 below). The spectrum analyzer will not work if VS1053 is
not connected (i. e. if radio is in demo mode).

It will compile in Arduino-IDE. You have to make sure to set the board to "ODROID ESP32" as conditional compile is used 
to apply some changes to the base software.

Note that this project is going on for quite some time already. I originally branched from Version "Thu, 14 Oct 2018 07:22:32 GMT".

The connection to the VS1053 is done through the Odroid-header pins as follows (schematic to follow):

* Pin 1 to VS1053 GND
* Pin 2 to VS1053 SCK
* Pin 3 to VS1053 CS
* Pin 4 to VS1053 DCS
* Pin 5 to VS1053 DREQ
* Pin 6 not connected (switch pin 3V3 can be used to add a power controller for VS1053, see Limitations below)
* Pin 7 to VS1053 MISO
* Pin 8 to VS1053 MOSI
* Pin 9 not connected
* Pin 10 to VS1053 VCC and Reset

## Compiling
* Arduino-IDE v1.8 or later (tested v1.8.5, required at least v1.8.0)
* ESP32-Core (tested v1.0.4)
* Other libraries needed:
	* time.h (Michael Margolis, maintainer Paul Stoffregen, v1.5.0)
	* PubSubClient.h (by Nick O'Leary, v2.7.0)
* Other versions might work too, its just untested. Please let me know if you encounter any difficulties.
* Select Board "ODROID ESP32". This is important, otherwise it will not include 
all the specific funtionalities for Odroid-Go.
* All other settings should be standard (Flash Mode QIO, Partition Scheme Standard, Flash Frequency 80 Mhz, Upload Speed 921600 in my case)

## Limitations
* The initial setup (connecting to your WiFi) needs to be done through the WebInterface as usual. Please read the doc 
https://github.com/Edzelf/ESP32-Radio/blob/master/doc/ESP32-radio.pdf namely section "Configuration". Note that the name and 
password of the Access Point are changed to "ODROIDRadio". The IP will be the same 192.168.194.1
	* To start, it is sufficient to configure your WiFi credentials. 
	* Connect your desktop PC to access point ODROIDRadio (Password ODROIDRadio) and open http://192.168.4.1/config.html
	* Press Button (Default) to load the initial values.
	* Search for the entry wifi_00 = and set the line to wifi_00 = YourSSID/YourPassword
	* Press buttons (Save) and (Restart) and you should be done (Observe screen output to check for success or Serial output 
for more verbose debug information).
* There are no available pins available for attaching functions to. Any settings in the configuration file are ignored.
* Also the pin assignments (for VS1053) are fixed by the limitations of the header pin. Any pin settings in the config file 
are ignored.
* The most important settings are thus the WiFi credentials in wifi_00 (or wifi_01 etc. if you want WiFiMulti) and the 
radio presets in preset_00 through preset_99. Some examples are already in the default config. 
* The SD slot of the VS1053 cannot be used. The internal SD-card is wired, but I did put no effort whatsoever on the mp3-player.
It might work, but can only be controlled through the web-Interface and might interfere with the button and screen-API for the
radio.
* The VS1053 board is powered with 5V. Thus USB needs to be connected to the Odroid to supply that 5V through the header pins.
You can not power from the battery.
* The connection is done through the header pin on top. In my application I built a "docking station" to connect. Therefore 
the display is flipped. If buttons "(Left)", "(Right)", "(Up)", "(Down)" are referred later on that will be from that Upside-down 
perspective as well. The four buttons (Start, Menu etc.) on top of the screen (again, from upside-down perspective) will be 
referred to as "(1)" to "(4)" (from left to right upside down view).

## Possible Startup-Issue
The VS1053 must be powered through USB. At least one of the GPIO header pins is connected to a strapping pin that alters the 
bootup sequence. Sometimes (that depends: I have tested several combinations of Odroids and VS modules. For some that means 
always, for others there was no issue at all) the Odroid will not start properly when USB and VS1053 are connected and the Odroid is switched on. The safe 
sequence is to connected Odroid to VS1053, switch on the Odroid from battery and after this switch on USB power supply 
(and thus power the VS1053). The timeframe between switching Odroid On and Power Up USB is in the range 0.5 to 4 seconds 
(roughly).

There is a SW-solution: If you switch the Odroid ON (from Battery, USB not yet powered) press and hold button (A). A screen 
will show "Release (A) to continue...". If you do so, next prompt will be "Now press (B) to start...". Now you have all the 
time you need to power USB. After you press (B), startup will commence. That does the trick for me.

I have also a HW-solution involving an ATTINY. Details to follow.

Again, if you get lucky you will not encounter that issue at all.

## Normal Operation
### Screen layout

The radio screen is mainly similar to the default ESP32-Radio-Screen. 
* Top line shows Application name and the current time.
* Middle section shows Radio Text as provided as metadata in stream
* Bottom section (yellow. Original colors look brighter than my pictures taken with mobile) has Station info
* Bottom line shows current "bank" for favorite stations.

### Changing Volume
Press (and hold) <Left> or <Right> to decrease/increase the volume. The current volume setting is shown on button of screen.
The range is limited by a configurable Minimum and Maximum.

### Changing Station
#### By favorites
When in normal radio screen, you can press "(1)" to "(4)" to select one of four stations out of the currrent "favorite bank".
There are 6 banks in total (A1..A3, B1..B3). You switch between that banks by pressing (A) (sequence will be 
<i>A1->A2->A3->A1->...</i> on repeated single presses or longpress) or (B) (to switch between <i>B1/B2/B3</i>).

Note that pressing (A) or (B) will not change the station but only the stations assigned to Buttons (1) to (4).
That change will happen only if you press either one of (1) to (4). (There are also Menues linked to (1) to (4) which will 
appear after longpress (>1sec) to a button. The buttons (1) to (4) are a bit fiddely with their haptics, so you might have 
to practice a bit to apply the right amount of pressure and timing).

If you press (A) or (B) or any of (1) to (4) a popup will show the stations in current bank. This popup will disapear after a
(configurable) timeout. 
 
The favorites are linked directly to preset_00 to preset_23 as configured. A1/(1) starts preset_00, A1/(2) preset_01 and so 
on till B3/(4) for preset_23.

Presets that are not assigned will show as empty line. If selected, the radio will skip to the next available station in 
the preset list.

#### By Channel list
By pressing (Up) or (Down) in normal screen, a station list will appear that allows to browse through all defined presets.
Empty (not defined) presets are not shown in this list (also not the preset number). A station can be started by pressing 
(Right). The list will be cancelled if (Right) is pressed or after a timeout without any user input.

## MENUs
### General
* Menus allow to edit certain parameters of the radio.
* Menus can be reached by a longpress (ca. 1 sec) on (1) to (4).
* You scroll through the menu entries using (Up) and (Down).
* The value of the selected entry can be changed using (Left) or (Right).
* Usally changes are stored in NVS persistently. Within the menu, use (A) to store updated values to NVS.
* Use (B) to cancel menu without writing the changes to NVS.
* There is a (configurable) timeout that also closes the menu without saving.

### Menu 1: Settings

* Preset at Start: Select the station to be played at Power On. Can be any preset from A1/(1) to Bx/(4) or LAST to 
start with last station played (like standard ESP32-Radio implementation)
* Volume at Start: Select the volume at Power On.
* Minimum Volue: Select the lower limit of Volume that can be set by pressing (Left) in playing mode. (Can be 0, but 
depending on the amplifier/headphones used anything below around 50 is probably not audible at all).
* Maximum Volume: Select the uper limit of Volume can be set by pressing (Right) in playing mode. (Max. 100)
* Life Volume Updated: if set to Yes, playing volume will be adjusted to the current value of the menu entry if either of
the former 3 Volume entries is the current menu entry. (Can be annoying as this will adjust volume direct if you select 
any of these entries).
* Brightness Normal: Display backlight setting for normal operation. 255 is maximum, 0 is invisible.
* Brightness down time: in seconds: timeout for no user interaction (key press) the display will stay on full brightness before
dimming down to the dimmed level in normal play mode.
* Brightness Dimmed: after a configurable timeout the display will be dimmed down to this value. Again, this is only for the
"Normal mode", if the radio is playing with the default radio screen (no menu/popup shown).
* Brightness up time: if dimmed down, time to ramp up to normal brightness after key press. Can be adjusted in steps of 50 ms
from 0 (direct) to 500 msec.
* Timeout Volume popup: how long (secs) will the volume information stay on display after being changed by pressing (Left) or
(Right). Valid range: 0 to 60
* Timeout Menu popup: how long will the menus stay on display without user input (in seconds). Range: 10..32767
* Timeout Channel list: how long will the Channel list stay on the screen without user input before being cancelled. Range: 5..32767 
* Timeout Favorites: how long will the contents of the current favorite bank stay on display after a favorite has been
selected by a shortpress on (1) to (4) or the bank has been switched by pressing (A) or (B) in normal play mode. (3..32767)
* ChanSelect closes list: If set to "No", the Channel list will stay active after a channel has been selected by pressing (Right).
* Number fav. Banks: how many banks are associated to button (A) and (B). Defaults to 3, which means there is the possibility
to select 24 favorites from A1/(1)...B3(4). If you are happy with just 8 stations, you can set this value to 1 (to have the
first 8 presets linked to A1(1)..A1/(4), B1/(1)..B1(4). Maximum value is 9 for 72 presets linked to A1/(1) ... B9/(4).
* JoyPad speed: set the speed of the JoyPad buttons to Low, Medium or High. This affects all functions these buttons are linked
to (updating volume, updating menu entry, selection from list...)
* Ignore SD-card: if set to Yes, the lengthy search for mp3-files on SD card will be skipped. Suggested setting if you do not
plan to use the media-player (as there is currently no support in this version to control the player by Odroid buttons).
* Show debug on Serial: if set to yes, some debug output is shown on Serial. (There is no real reason to set this to No)
* For information purposes, the current IP is shown at the end of the menu list.

### Menu 2: Preset list
* Shows all defined presets (from preset_00 to preset_99)
* Is station is mapped to a favorite button, it is labelled Ax/1..4 or Bx/1..4 accordingly. Otherwise its number (up to 99) 
is shown in addition to the station name.
* If a preset is not defined, an empty string will be displayed (so there are always 100 entries in this list).
* This menu allows to change the order of the preset.
* Select the preset you want to change in position. Press (Right). Now that station is locked and will follow if you press
(Up) or (Down). As long as the entry is locked, the word "MOVE" will be displayed at the right side of that entry.
* You can release the locked entry by pressing (Left).
* During the move, the position indicator is updated accordingly. That allows to conveniently re-organize the favorite banks.
* You have to confirm the changes by pressing (A). This change requires a restart of the radio (will be prompted on screen).

(You can not enter new stations here. That needs to be done through the configuration webpage as usual.=

### Menu 3: Equalizer/Spectrum analyzer

Here you change the equalizer settings (not yet implemented) and configure the spectrum analyzer.

#### Equalizer
* Not yet implemented

#### Spectrum Analyzer
* Show Spectrum Analyzer: if switched to yes, a 14 channel spectrum analyzer will be shown in the radiotext section.
* Analyzer Speed: defines the update speed of the analyzer from "Disco" (very fast) to "Slug" (about 2 frames per second).
* Dynamic Brightness: if set to "yes", the brightness of channel bars is altered by its value (the higher the brighter 
a bar will appear).
* Show Peaks: show additional peak indicator on top of the channel bar.
* Bar width: the width of channel bars (from 0 to 10)
* Peak width: the width of the peak indicator (from 0 to 10 or set to "same" to follow the width of the channel bar.
* Segment Divider width: divides the channel bar in equal segments. Width can be 0 (means no divider) up to 10 or "same" to
follow the channel bar width.
* Segm.Divider color: color of segment divider.
* Show Radiotext: Show radiotext in addition to spectrum analyzer. Often the radiotext is just two lines or less on the 
display and usally will not interfere with the spectrum analyzer that stays below normally. Howerver with high peaks 
or lengthy text, the text might get erased by the analyzer. Text will only be refreshed if send newly by the current
radiostation.

Just play around with the settings to adjust to your liking. Note that any change in the menu will be applied directly. 
If you find the settings to your liking, remember to press (A) to store the settings for next Power On.
With few stations the equalizer will not work (reproducible). I have not yet found the reason for that issue.

ToDo: Parameter for channel bar color (now blue only).


### Menu 4: Sleep timer
* Sleep time: from now on in this many minutes the volume will dim to minimum value (and then to 0)
* Start volume: from now on this volume will will be ramped down to minimum/0 in the remaining time
* Dark after 1 min: if set to yes, the display will be switched off completely after 1 minute (otherwise it will just dim
down to brightness 1, which is still pretty dark).
* This menu will stay on until cancelled by pressing (B), which cancels the sleep function alltogether.
* Pressing (A) will write the current settings as default values. Note that over time the values for sleep time and volume
will adjust according to the elapsed time.



