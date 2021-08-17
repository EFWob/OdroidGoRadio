# Odroid-Go-Radio
## Latest updates
You can now play stations from genre lists (i. e. Pop, Rock, etc...) that are synchronized using a public radio database server (https://www.radio-browser.info/#/). 
This database has more than 27,000 stations organised in different categories (by region or genre tags). With this addition, you can 
download stations from that radio database as given by their genre-tag. As a result you can have more station lists
(in addition to the default preset list in NVS) to chose from.
By now, that lists are organized by genre tag only. So you can create lists like "Pop", "Rock", etc. but not for instance by country.

The Genre lists will be stored in Flash using LITTLEFS. Therefore, you want to have a SPIFFS partition as big as possible. The Odroid has 
16MB Flash, unfortunately the default partitions suggested in Arduino for the Odroid do not make use of those MBs for flash. Workaround (tested) is 
to set the board "M5Stack-FIRE" and Partition Scheme "Large SPIFFS" in the Arduino Tools-Menu.

Three more libraries are needed compared to the previous release, see [Compiling](#compiling) below.

Before using Genre playlist, you must download the playlists from the radio database. See [Using Genres](#using-genres) below.

## Basic Idea
This is a project to port the ESP32-Radio by Edzelf (see https://github.com/Edzelf/ESP32-Radio) to the Odroid-Go to use the 
display and the buttons to control the radio.

It also features a very cool 14 channel spectrum analyzer (see Menu3 below). The spectrum analyzer will not work if VS1053 is
not connected (i. e. if radio is in demo mode).

For testing, the SW runs in demo mode without a VS1053 module connected. All buttons are fully functional, but there will 
be no noticable audio output...

It will compile in Arduino-IDE. 

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
* ESP32-Core (tested v1.0.6)
* Other libraries needed:
	* time.h (Michael Margolis, maintainer Paul Stoffregen, v1.5.0)
	* PubSubClient.h (by Nick O'Leary, v2.7.0)
	* Base64 (by Arturo Guadalupi, v1.0.0) (new for this release)
	* ArduinoJson (by Benoit Blanchon, v6.13.0) (new for this release)
	* LittleFS_esp32 (by lorol v1.0.6) (new for this release)
	bblanchon/ArduinoJson

* Other versions might work too, its just untested. Please let me know if you encounter any difficulties.
* Select Board "M5Stack-FIRE" and Partition Scheme "Large SPIFFS". In fact, any board definition should be fine, however the ones with
PSRAM enabled and a large SPIFFS should be preferred ("ODROID ESP32" as usual will work fine, too with less space for genre playlists in SPIFFS).
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
Press/hold (Left) or (Right) to decrease/increase the volume. The current volume setting is shown on button of screen.
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
(Right). The list will be cancelled if (Left) is pressed or after a timeout without any user input.


## Using Genres
### Loading Genres
Befor using genre playlists, you must download them to the Odroid. The playlists will be stored in the Flash-Filesystem, so you probably want to 
use a huge SPIFFS partition (see [Compiling](#compiling) above). This setting can not be changed at runtime!!

The genre playlists of the Odroid are maintained using the public radio database server (https://www.radio-browser.info/#/). 
This database has more than 27,000 stations organised in different categories (by reagion or genre tags). With this addition, you can 
download stations from that radio database as given by their genre-tag. As a result you can have more station lists
(in addition to the default preset list in NVS) to chose from.
By now, that lists are organized by genre tag only. So you can create lists like "Pop", "Rock", etc. but not for instance by country.

### Creating genre playlists
A specific playlist can have any number of stations, as long as there is free flash left in the flash file system.
Unlike the preset list, the station URLs are not entered direct, but are downloaded from the database above. 
For maintaining the genre playlist, you need to open the URL http://RADIO-IP/genre.html. The first time you do so
you should see the following. 
![Web API for genre](pics/genre0.jpg?raw=true "Empty genre playlists")

There are two main parts on that page:
- on top (currently empty) is the list of genres that are already loaded into the radio.
- the bottom section is the interface to the internet radio database.

You first should start with the interface to the radio database. In the leftmost input, enter (part) of a genre tag name, e. g. "rock". Enter a minimum number of stations that should be returned for each list (if the number of 
stations is less than that number, that specific list is not returned). Can be left empty, try 20 for now. If you press "Apply Filter" (ignore the right input field for now) you should see the result of the database request after 
a few moments:

![Web API for genre](pics/genre1.jpg?raw=true "Radio database answered the request")

The result list at the bottom just shows the result of the request, the lists are not yet loaded to the radio.
In the dropdown at the right side of the result list you can choose which entries you want to "Load" to the radio.
When decided, press the button labelled "HERE" at the bottom of the page. Only then the station lists selected will
be loaded into the radio. The website will show the progress:

![Web API for genre](pics/genre2.jpg?raw=true "Transfer from radio database to radio")

While you see that page, do not reload the page or load any other page into this tab as otherwise transfer will be
cancelled. If you accidentially cancel the tansfer, the radio will still be in a consistent state. Just the list(s) of stations will be truncated/incomplete.
After the transfer is completed, the page will automatically reload and should look like this:

![Web API for genre](pics/genre3.jpg?raw=true "First genre playlists loaded to radio")

More often than not, you will notice that the stations of your interest are distributed over several genre groups on 
the database. Like in our example "indie rock", "pop rock" and "progressive rock" we just loaded would be an example of "subgenres" for rock. 

As you will see later, you can only select one of those genres. You can however cluster the result list from the database into a "Cluster" called "Rock". There is only requirement for the clustername: it must start with a letter. And that first letter will be converted into uppercase automatically. All genre names in the database are returned in always lowercase letters. That way, you can have a cluster called "Rock" that can be distinguished from the "native" genre "rock".

So instead of loading 3 seperate genres, you could chose the Action "Add to:" for the genres of choice. You must enter the desired cluster name ("Rock" in our example) into the input field right of the button "Apply Filter".

![Web API for genre](pics/genre4.jpg?raw=true "Creating cluster Rock from 3 genres")


Clusters must not be created in one step, you can always add more stations from another database request later.
You can however not delete a subgenre from a cluster (but only delete the whole cluster).
If you press on the station number of a Cluster in the page section "Maintain loaded Genres" a popup will show which genres from the database are clustered into.

![Web API for genre](pics/genre5.jpg?raw=true "In the maintenance section")


In the maintenance section you can also Delete or Refresh any or all of the genres loaded into the database (a cyclic refresh is needed to throw out station URLS that got nonoperational in between).

If you click on a genre name on the left side in this list, the radio will play a (random) station from that genre.
(If you press again, another random station from that list will be played).
That is currently the only way to play from a genre using the web-interface.

Some details:
Each genre name (either if coming direct from the database or a user defined clustername) must be unique. From the database that is guaranteed. The API does not allow to download the same genre twice. Clusternames are different 
from loaded genre names because they always start with an uppercase letter. You can use the same cluster name again in further database requests to add more genres into it. You cannot add the same genre twice to the same cluster, but you can add the same genre to more than one cluster.
You cannot edit the resulting playlist any further. You can not add single station URLs "by hand", you can not delete a genre from a cluster. You can only delete the full cluster (or a whole genre).

With the default partition the file system is big enough to support (estimate) between 10,000 and 12,000 stations in total. (With the radio4MB_default partition it is still somewhat in between 8,000 and 10,000). For instance the radio that I just use for debugging has a total of 3182 stations stored which consumes 360,448 of 1,114,112 bytes of the flash file system leaving more than 66 per cent free for further playlists.


The Web interface can hande extended unicode characters. The only thing thats not working currently is if the genre name of the database contains the '/'-character. That I have seen on one genre so far and already forgotten again what it was, so it is currently no priority...

On the Odroid display special Unicode characters can not be displayed properly. The result shortened (or empty) list entries on the Odroid display,
that can be selected correctly nnonetheless. The same applies to station names including unicode characters.

![Web API for genre](pics/genreunicode.jpg?raw=true "Unicode is fine (but not for ODROID display")


### Using genre playlists
A playlist can be selected by the command interface by using the command
_genre=Rock_
to play one genre playlist. If that genre exists (as a cluster in this example, but could also be a native genre name from a direct download). 
**Remeber that genre playlist names are case sensitive.** Genres loaded from database direct will always have 
lowercase letters only, while cluster names start with an uppercase letter (followed by only lowercase letters).
So, _Rock_ is a valid name for a cluster, _rock_ is a valid name for a genre tag from the internet radio database,
but constucts like _ROCK_ or _rOcK_ are invalid. When in doubt, copy the name from the web API.

If a valid genre name has been assigned (and that genre exists in SPIFFS), the radio will start to play a random station from that genre. If the same command is issued again (with the same name), another (random) station from that genre will play.

You can switch to a station direct using the command 

_gpreset=Number_  

where Number can be any number. If this number (lets call it n) is between 0 and 'number of stations in that genre - 1', the n-th entry of that list is selected. N can also be greater than the number of stations (or even below zero), modulo function is used in that case to map n always between 0 and 'number of stations in that genre' - 1.

If no genre has been set by the _genre=XXXX_ command above, _gpreset=n_ has no effect.

So selecting a station is still "fishing in the dark", but it is at least reproducible (if you find that favorite station of yours at index 4711).

If you issue command _genre=Anothername_ with another genre name, the radio will switch to that genre.

If you issue command with no name (empty), the genre playmode will be stopped. The current station will continue to
play (until another preset or channel is selected). _gpreset=n_ however will have no effect. You can also issue the command _genre_ with parameter _genre=--stop_ to achieve the same if you prefer a more clear reading.

You can also switch stations within a genre using the _channel_ command from above. To do so, a channel-list must be defined.
The preset-numbers assigned to the channel entries are ignored, just the size of the channel list is important.
In the channel example above we defined a channel list with 9 channels. If you switch to genre-playmode, that channel
list can be used to change stations within that genre by the following algorithm:
- each channel has a random number between 0 and 'number of channels in that genre - 1' assigned.
- one of the channel entries is the current channel (the one last selected by _channel=n_)
- the distance between two channel stations is the same for all neighbors (and wrapped around between channel 9 and 1) in our example. So if in our case we would have a genre list with 90 stations, the distance between two channels would be 10.
- example list in that case could be Channel 1: 72, Channel 2: 82, Channel 3: 2, Channel 4: 12 .... Channel 9: 62
- that assignment stays stable until:
  - a new random station is forced by the command _genre = Xxxxxx_, this will result in a completely different list.
  - a new station is forced by the command _gpreset = n_
  - the command _channe=up_ is issued. The numbers associated to the channels are increased by one (as well as the currently playing station from the genre will be switched to next in list)
  - the command _channel=down_ is issued. The numbers in the channellist are decreased by 1 (also for the current station)
  - the command _channel=any_ is issued. This will result in playing another (random) station and will also result in
    a completely different list.



### Configuring anything around genres

To configure genre settings use the command 

**_gcfg.subcommand=value_**

The command can be used from command line or from the preference settings in NVS. If not set in the preferences (NVS) all settings are set to the defaults described below.

The following commands (including subcommands) are defined:

- **_gcfg.path=/root/path_** All playlist information is stored in Flash (using LITTLEFS) or on SD-Card. If path value does start with 'SD:' (case ignored), the genre lists will be stored on SD card using the path following the token 'SD:'. If not, the genre information is stored in Flash, using LITTLEFS with path '/root/path' in this example.
No validity checking, if the given path does not exists or is invalid, genre playlists will be dysfunctional. (For historic reason, **defaults to '/____gen.res/genres'**). Must not end with '/'! Can be changed at any time (to allow for different genre playlists for different users).
- **_gcfg.host=hostURL_** Set the host to RDBS. **Defaults to 'de1.api.radio-browser.info'** if not set. 'de', 'nl', 'fr' can be used (as short cuts) to address 'de1.api.radio-browser.info', 'nl1.api.radio-browser.info' or 'fr1.api.radio-browser.info' respectively. Otherwise full server name must be given.
- **_gcfg.nonames=x_** if x is nonZero, station names will be stored in genre playlists (not just URLs). Currently, station names from genre playlists are not used at all but might be useful in future versions. When short on storage, set to '1'. **Defaults to 0**, so station names will be stored.

### Considerations (and limitations) around using genre playlists

The total number of genre lists is limited (to 1000). This is a compile time limitation that can not be changed by a command or a preference setting. 

For faster access, some information is cached. For caching, PSRAM is preferred. If PSRAM is not available, normal heap is used. PSRAM should be plenty, however, if there is no sufficient heap, operation might be slower. (Use command _test_ from the Serial input. If the reported Free Memory is below 100.000, it is likely that RAM caching is
not available.)

On a board with 16MB flash I was able to load around 33.000 stations within 740 genres before the flash was fully used. With SD-card even more will be possible. However, SD access is slower. It is using shared access with the SPI. You will notice quite a few debug messages saying "SPI semaphore not taken...". That is annoying but still fine. It is probably advisable to stop radio playback (using _stop_ from the Serial command line) to limit the access conflicts on the SPI bus if you maintain the genre playlists (adding/deleting/reloading).

If the radio stream stalls, the radio has a fallback strategy to switch to another preset. If that happens when playing from a genre playlist, the radio would fall back to a preset from the preset list in preferences.
You will notice that in genre playlist mode this can happen (for remote stations with a bad connection). In a next step, this fallback needs to be recoded to use another station from the current genre playlist. If you want to avoid the fallback to a preset from NVS, use the command _preset=--stop_. This will block fallback to a station from presets until a station from presets is requested (for instance by _preset=n_  or _channel=m_ if genre play mode has stopped).

When in genre play mode, you can still issue a _preset=n_ command and the radio will play the according preset from the preferences. However, genre playlist mode will not be stopped: the command _gpreset=n_ as well as _channel=m_ (if channellist is defined) will still operate on the current genre playlist.



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
* Bar width: the width of channel bars (from 0 to 20). Bars are always shown equidistant (20 points away from each other). 
Smaller bars will have spacing in between. 
* Peak width: the width of the peak indicator (from 0 to 20 or set to "same" to follow the width of the channel bar.
* Segment Divider width: divides the channel bar in equal segments. Width can be 0 (means no divider) up to 20 or "same" to
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



