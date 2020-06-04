// Default preferences in raw data format for PROGMEM
//
#define defaultprefs_version 1808016
const char defprefs_txt[] PROGMEM = R"=====(
# Set WiFi credentials here!
wifi_00 = YourSSID/YourPassword!
# uncomment the following line(s) if you need access to more than one WiFi network with the same device (
#wifi_01 = AnotherSSID/WithAnotherPassword!
#wifi_02 = YetAnotherSSID/WithYetAnotherPassword!
#wifi_03 = ... (you get the idea!)

# Time settings. Adopt to your local timezone
clk_dst = 1                                          # Offset during daylight saving time (hours)
clk_offset = 1                                       # Offset with respect to UTC in hours
clk_server = pool.ntp.org                            # Time server to be used

# Mqtt support as implemented with the original ESP32Radio version
mqqprefix = none
mqttbroker = none
mqttpasswd = none
mqttport = 1883
mqttuser = none

# Just some presets.
# My favorite site to look for station URLs is http://fmstream.org
# Note that the comments at the end (after '#') are important: if given, all to the right
# will be used as station name on the OdroidGo-Display.
# preset_00 to preset_99 can be defined
# first presets should be set to your favorite stations
# gaps are not a problem. Just not nice...
 
preset = 0
preset_00 = st01.dlf.de/dlf/01/128/mp3/stream.mp3 #  Deutschlandfunk
preset_01 = st02.dlf.de/dlf/02/128/mp3/stream.mp3 #  Deutschlandradio
preset_02 = www.ndr.de/resources/metadaten/audio/m3u/ndrkultur.m3u  #  NDR Kultur
preset_03 = avw.mdr.de/streams/284310-0_mp3_high.m3u #  MDR Kultur
preset_04 = www.ndr.de/resources/metadaten/audio/m3u/ndrinfo.m3u #  NDR Info
preset_05 = streams.br.de/br-klassik_2.m3u  #   BR Klassik
preset_06 = avw.mdr.de/streams/284350-0_aac_high.m3u #  MDR Klassik
preset_07 = live.radioart.com/fCello_works.mp3       #   Cello Works
preset_08 = stream.radioparadise.com/mp3-192         #   Radio Paradise
preset_09 = direct.fipradio.fr/live/fip-midfi.mp3    #   FIP
preset_10 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
preset_11 = streamplus25.leonex.de:26116 # Radio Okerwelle
preset_12 = live.helsinki.at:8088/live160.ogg # Radio Helsinki (Graz)
preset_13 = 92.27.224.83:8000/ # Legacy 90.1
preset_14 = sc-60s.1.fm:8015 # 20 - 1fm 50s/60s
preset_15 = radio-hannover.divicon-stream.com/live/mp3-192/Homepage/play.m3u # Radio Hannover
preset_16 = stream.saw-musikwelt.de/saw-80er/mp3-128/listenliveeu/stream.m3u # Radio SAW 80er
preset_17 = streams.harmonyfm.de/harmonyfm/mp3/hqlivestream.m3u # Harmony FM
preset_18 = www.memoryradio.de:4000/                 #   Memoryradio 1
preset_19 = www.memoryradio.de:5000/                 #   Memoryradio 2
preset_20 = www.ndr.de/resources/metadaten/audio/aac/ndr2.m3u # NDR 2
preset_21 = metafiles.gl-systemhaus.de/hr/hr1_2.m3u  #   HR1
preset_22 = 178.77.76.9:8000/128.mp3 #Radio fresh80s
preset_23 = player.ffn.de/comedy.mp3 # Radio ffn Comedy
preset_24 = cdn.peoplemesh.net:9000/livestream2.mp3 # Detektor.fm Musik
preset_25 = stream.hoerradar.de/detektorfm-wort-mp3-128 # Detektor.fm Wort
preset_26 = 139.18.27.192:8000/mephisto976_livestream.mp3 # Mephisto
preset_27 = mp3ad.egofm.c.nmdn.net/egofm_128/livestream.mp3 # Ego.FM
preset_28 = fc.macjingle.net:8200/ # Radio Technikum
preset_30 = rs27.stream24.net/radio38.mp3 # Radio 38
preset_31 = 188.94.97.91/radio21_wolfsburg.mp3 # Radio 21
preset_32 = www.ndr.de/resources/metadaten/audio/aac/ndr2.m3u # NDR 2
preset_33 = www.ndr.de/resources/metadaten/audio/aac/ndrblue.m3u # NDR Blue
preset_34 = stream.rockland-digital.de/rockland/mp3-128/liveradio/ # Rockland Magdeburg
preset_35 = stream.saw-musikwelt.de/saw/mp3-128/listenliveeu/stream.m3u # Radio SAW
preset_36 = stream.saw-musikwelt.de/saw-deutsch/mp3-128/listenliveeu/stream.m3u # Radio SAW Deutsch
preset_37 = stream.saw-musikwelt.de/saw-rock/mp3-128/listenliveeu/stream.m3u # Radio SAW Rock
preset_38 = stream.saw-musikwelt.de/saw-80er/mp3-128/listenliveeu/stream.m3u # Radio SAW 80er
preset_39 = streams.harmonyfm.de/harmonyfm/mp3/hqlivestream.m3u # Harmony FM
preset_40 = player.ffn.de/ffn.mp3 # Radio ffn
preset_41 = sc-60s.1.fm:8015 # 20 - 1fm 50s/60s
preset_42 = mp3stream1.apasf.apa.at:8000/ # ORF FM4
preset_43 = cast1.citrus3.com:8866                   #   Irish Radio
preset_44 = streams.rsa-sachsen.de/rsa-ostrock/aac-64/listenlive/play.m3u # OstRock
preset_45 = stream.sunshine-live.de/live/aac-64/listenlive/play.m3u # Sunshine live
preset_46 = broadcast.infomaniak.ch/jazzradio-high.mp3 # Jazz Radio Premium
preset_47 = www.fro.at:8008/fro-128.ogg # Radio FRO
preset_48 = 148.163.81.10:8006/ # Zenith Classic Rock
preset_49 = 205.164.62.15:10032                      #   1.FM - GAIA, 64k
preset_50 = direct.fipradio.fr/live/fip-webradio4.mp3 #  FIP Latin
preset_51 = stream1.virtualisan.net/6140/live.mp3    #   Folkradio.HU
preset_52 = relay.publicdomainproject.org:80/jazz_swing.mp3 #  Swissradio Jazz & Swing
preset_53 = 167.114.246.177:8123/stream              #   Blasmusik
preset_54 = 212.77.178.166:80                        #  Radio Heimatmelodie
preset_55 = stream.srg-ssr.ch/m/drsmw/mp3_128        #   SRF Musikwelle
preset_56 = www.ndr.de/resources/metadaten/audio/m3u/ndr1niedersachsen.m3u #  NDR1 Niedersachsen
preset_57 = sc2b-sjc.1.fm:10020                      #  1fm Samba Brasil
preset_58 = 1a-entspannt.radionetz.de:8000/1a-entspannt.mp3      #   Entspannt
preset_59 = tx.planetradio.co.uk/icecast.php?i=absolute60s.mp3     #   Absolute 60s
#
toneha = 5
tonehf = 2
tonela = 15
tonelf = 13
#
volume = 96
#

)=====" ;
