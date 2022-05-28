// about.html file in raw data format for PROGMEM
//
#define about_html_version 170626
const char about_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
 <head>
  <title>About ODROID-GO-Radio</title>
  <meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
  <link rel="Shortcut Icon" type="image/ico" href="favicon.ico">
  <link rel="stylesheet" type="text/css" href="radio.css">
 </head>
 <body>
  <ul>
   <li><a class="pull-left" href="#">ODROID-GO-Radio</a></li>
   <li><a class="pull-left" href="/index.html">Control</a></li>
   <li><a class="pull-left" href="/genre.html">Genre</a></li>
   <li><a class="pull-left" href="/config.html">Config</a></li>
   <!-- <li><a class="pull-left" href="/mp3play.html">MP3 player</a></li> -->
   <li><a class="pull-left active" href="/about.html">About</a></li>

  </ul>
  <br><br><br>
  <center>
   <h1>** ODROID-GO-Radio **</h1>
  </center>
	<p>ODROID-GO-Radio -- Webradio receiver for Odroid-Go.<br>
  This project is documented at <a target="blank" href="https://github.com/EFWob/OdroidGoRadio">Github</a>.</p>
  <p>Author: Erik Foltin (erik.foltin@gmx.de)<br>
  
	This project is based on the project ESP32 Radio, also documented at <a target="blank" href="https://github.com/Edzelf/ESP32-radio">Github</a>.</p>
	<p>Author: Ed Smallenburg (ed@smallenburg.nl)<br>
	Webinterface design: <a target="blank" href="http://www.sanderjochems.nl/">Sander Jochems</a><br>
	App (Android): <a target="blank" href="https://play.google.com/store/apps/details?id=com.thunkable.android.sander542jochems.ESP_Radio">Sander Jochems</a><br>
	Date: June 2017</p>
 </body>
</html>
)=====" ;
