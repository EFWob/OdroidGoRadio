#include "ESPNowWrapper.h"
#if defined(ESP32)
#include <WiFi.h>
#endif
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

class ESPNowRadioClient: public ESPNowServiceHandlerClass  {
    public:
      ESPNowRadioClient(const uint8_t* mac);
      virtual void flushRX(const uint8_t *mac, const uint8_t packetId, const uint8_t *data, const uint8_t dataLen);
      bool initDone();
      void runState(int16_t stateNb);
      void listPresets();
      uint8_t getChannel();
      bool startRequest();
      void endRequest(bool waitResponse = false);
      void requestSetVolume(uint8_t val);
      void requestSetPreset(uint8_t val);
      void requestStatus();
    protected:
      void searchChannel();
    public:
      uint8_t _numPresets, _knownPresets, _channel, _minVolume, _maxVolume, _curVolume, _curPreset;
      char *_presets[100];
      char *_radioText = NULL;
      
}  espNowRadioClient(broadcastMac);

uint32_t volumeChangeTime = 0;
uint8_t volumeValue = 0;
uint32_t presetChangeTime = 0;
uint8_t presetValue = 0;
uint32_t statusChangeTime = 0;
uint32_t statusFastChangeTime = 0;
uint32_t statusUpdateBlockTime = 0;


#if defined(ARDUINO_T_Watch)
#define LILYGO_WATCH_LVGL
#define LILYGO_WATCH_2020_V1             //To use T-Watch2020, please uncomment this line
#include <LilyGoWatch.h>
bool pwrKeyFlag = false;

TTGOClass *ttgo;
lv_obj_t * volumeSlider, *radioText, *stationList; 



static void event_handler_volume(lv_obj_t * sldr, lv_event_t event)
{
   if(event == LV_EVENT_VALUE_CHANGED) {
      printf("Value: %d\n", lv_slider_get_value(sldr));
      volumeChangeTime = millis() + 50;
      statusUpdateBlockTime = volumeChangeTime + 2000;
      volumeValue = lv_slider_get_value(sldr);
      }
}

static void event_handler_preset(lv_obj_t * sldr, lv_event_t event)
{
   if(event == LV_EVENT_VALUE_CHANGED) {
      printf("Value: %d\n", lv_slider_get_value(sldr));
      presetChangeTime = millis() + 50;
      presetValue = lv_dropdown_get_selected(sldr);
      }
}


static void event_handler_text(lv_obj_t * sldr, lv_event_t event)
{
   if(event == LV_EVENT_RELEASED) {
      statusChangeTime = millis();
      }
}


void ttgoSetup() {
    ttgo->tft->printf("DONE!\r\n");

    pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] {
        pwrKeyFlag = true;
    }, FALLING);
    ttgo->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ,
                     true);
    ttgo->power->clearIRQ();

  
    ttgo->lvgl_begin();
    volumeSlider = lv_slider_create(lv_scr_act(), NULL);
    lv_obj_set_size(volumeSlider, 210, 10);
    lv_obj_set_pos(volumeSlider, 15, 212);
    lv_slider_set_range(volumeSlider, espNowRadioClient._minVolume , espNowRadioClient._maxVolume);
    lv_slider_set_value(volumeSlider, espNowRadioClient._curVolume, LV_ANIM_OFF);
//    lv_obj_align(slider, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_event_cb(volumeSlider, event_handler_volume);

    radioText = lv_textarea_create(lv_scr_act(), NULL);
    lv_obj_set_size(radioText, 230, 150);
    lv_obj_set_pos(radioText, 5, 45);
    lv_textarea_set_text(radioText, espNowRadioClient._radioText);
    lv_textarea_set_cursor_hidden(radioText, true);
    /*Set an initial text*/
    lv_obj_set_event_cb(radioText, event_handler_text);

    stationList = lv_dropdown_create(lv_scr_act(), NULL);
    lv_obj_set_size(stationList, 230, 35);
    lv_obj_set_pos(stationList, 5, 5);
    for (int i = 0;i < espNowRadioClient._numPresets;i++)
      lv_dropdown_add_option(stationList, espNowRadioClient._presets[i], i);
    lv_dropdown_set_selected(stationList, espNowRadioClient._curPreset);
    lv_obj_set_event_cb(stationList, event_handler_preset);
          
}
#endif


bool statusUpdated = false;
void performanceLoop(void);


void recvCbFunction(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    char s[255];
  memcpy(s+1, data, data_len);
  s[data_len +1] = 0;
  Serial.printf("Got something from {%02X, %02X, %02X, %02X, %02X, %02X}, len=%d, content=",
    mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5],
       data_len);
    
}

void sendCbFunction(const uint8_t *mac_addr, const uint8_t status) {
  if (NULL == mac_addr)
    Serial.printf("Send status to <NULL> = %d\r\n", status);
  else
    Serial.printf("Send status to {%02X, %02X, %02X, %02X, %02X, %02X} = %d\r\n", 
    mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], status);
}


ESPNowRadioClient::ESPNowRadioClient(const uint8_t* mac): ESPNowServiceHandlerClass(mac, 0x6161) {
         espnowWrapper.addServiceHandler(this);
         _numPresets = _knownPresets = _channel = _minVolume = _maxVolume = _curVolume = 0;
         _curPreset = 0xff;
         for(int i = 0;i < 100;i++)
          _presets[i] = NULL;
      };

bool ESPNowRadioClient::initDone() {
  if (isBroadcastMac()) {
    return false;
  }
  if (_numPresets > _knownPresets) {
    if (!_tx) {
      uint8_t x = 0;
      setTX((const uint8_t*)&x, 1, _knownPresets,10);
    }
    return false;
  }
  return true;
}
  
void ESPNowRadioClient::listPresets() {
  Serial.println();Serial.printf("Number Presets: %d (expected: %d)\r\n", _knownPresets, _numPresets);
  for(int i = 0;i < _knownPresets;i++)
    Serial.printf("%02d: %s\r\n", i, _presets[i]);
}

uint8_t ESPNowRadioClient::getChannel() {
  return _channel;
}

bool ESPNowRadioClient::startRequest() {
  if (getChannel()) {
//    setCpuFrequencyMhz(80);
    if (0 == espnowWrapper.begin(getChannel() & 0x7f)) {
      espnowWrapper.setRecvCb(recvCbFunction);
      espnowWrapper.setSendCb(sendCbFunction);
      Serial.printf("ESP-Now-Init war erfolgreich, Channel = %d!\r\n", espnowWrapper.channel());  
      return true;
    } else Serial.printf("ESP-Now init fail!?!?!?\r\n");
  } else Serial.println("GetChannel() fail!?!?!?!");
  return false;
}

void ESPNowRadioClient::endRequest(bool responseWait) {
  if (getChannel()) {
          for(int i = 0;(responseWait || hasTX()) && (i < 1000);i++) {
            espnowWrapper.loop();
            performanceLoop();
            delay(1);
          }
  
  
  }
    espnowWrapper.end();
    if (getChannel() < 128)
      WiFi.mode(WIFI_OFF);
  //setCpuFrequencyMhz(10);
}

void ESPNowRadioClient::requestSetVolume(uint8_t val) {
  if (startRequest()) {
    setTX(&val, 1, 201, 10);
    endRequest();
  }
}

void ESPNowRadioClient::requestSetPreset(uint8_t val) {
  if (startRequest()) {
    setTX(&val, 1, 200, 10);
    endRequest();
  }
}

void ESPNowRadioClient::requestStatus() {
  if (startRequest()) {
    uint8_t val = 0;
    setTX(&val, 1, 100, 10);
    endRequest(true);
  }
}


void ESPNowRadioClient::flushRX(const uint8_t *mac, const uint8_t packetId, const uint8_t *data, const uint8_t dataLen) {
  if (isBroadcastMac()) {
    setMac(mac);
    espnowWrapper.addPeer(mac);
//    clientHandler = new ESPNowRadioServer(mac);
//    clientHandler->dataCopy(this);
    _channel = espnowWrapper.channel();
    Serial.printf("New radio found: {%02X, %02X, %02X, %02X, %02X, %02X}\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
  } 
  Serial.printf("Handle radio server response: {%02X, %02X, %02X, %02X, %02X, %02X}, packetId = %d, Len = %d, data[0] = %d, Content = %s\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                  packetId, dataLen, data[0], data);
  if (100 == packetId) {
    Serial.printf("Volume: %2d (Min: %2d, Max: %3d), Preset: %d of %d, RadioText=%s\r\n", _curVolume = data[0], _minVolume = data[1], _maxVolume = data[2], 
       _curPreset = data[3], _numPresets = data[4], (char *)data + 5);
    if (_radioText)
      free(_radioText);
    _radioText = strdup((char *)data + 5);
    statusUpdated = true;   
  } else if (100 > packetId) {
    uint8_t nb = data[0] & 0x7f;
    Serial.printf("Found %d presets starting with preset %d\r\n", nb, packetId);
    char *s = (char *)data + 1;
    for(int i = 0;i < nb;i++, s += strlen(s) + 1) {
      if (i + packetId == _knownPresets) {
        if (_presets[_knownPresets] == NULL)
          if (_presets[_knownPresets] = strdup(s))
            _knownPresets++;
      }
      Serial.printf("Preset %02d: %s\r\n", i + packetId, s);
    }
    if (data[0] > 128)
      Serial.println("And thats all...."); 
  }
//  char tst[240];
//  int len = getPresetList((uint8_t *)tst, 240, 0);
//  clientHandler->setTX((const uint8_t *)tst, len, 'z');
//  clientHandler->setTX((const uint8_t *)"Hello from your radio!!!", 22, 41);
} ;



char toSend[255];
uint8_t sendMac[] = {0x30, 0xAE, 0xA4, 0xC3, 0xD9, 0x0C};

bool espnowStart(uint8_t chan) {
  if (0 == espnowWrapper.begin(chan)) {
//    espnowWrapper.addPeer(sendMac);
    espnowWrapper.setRecvCb(recvCbFunction);
    espnowWrapper.setSendCb(sendCbFunction);
    Serial.printf("ESP-Now-Init war erfolgreich, Channel = %d!\r\n", espnowWrapper.channel());  
    return true;
  }
  return false;
}



void setup() {
  int chanDelay = 5;
  Serial.begin(115200);
#if defined(ARDUINO_T_Watch)
    ttgo = TTGOClass::getWatch();
    //Initialize TWatch
    ttgo->begin();
    ttgo->tft->setTextSize(2);
    ttgo->openBL();
#endif
  toSend[0] = 0;
  uint8_t chan = 9;
  while (espNowRadioClient.isBroadcastMac()) {
    chan = chan + 1;
    if (14 == chan) {
      chan = 1;
      chanDelay++;
#if defined(ARDUINO_T_Watch)
      ttgo->tft->fillScreen(TFT_BLACK);
      ttgo->tft->setCursor(0,0);
#endif
    }
    Serial.printf("Scanning Channel %d\r\n", chan);
#if defined(ARDUINO_T_Watch)
    ttgo->tft->printf("Scanning Channel %d\r\n", chan);
#endif   
    if (espnowStart(chan)) {
      int i = 0;
      while((i < chanDelay) && espNowRadioClient.isBroadcastMac()) {
        uint8_t x = 0;
        i++;
        espNowRadioClient.setTX((const uint8_t *)&x, 1, 100);
        x = 200;
        while(espNowRadioClient.isBroadcastMac() && ++x) {
          espnowWrapper.loop();
          delay(1);
        }
      }
      if (espNowRadioClient.isBroadcastMac())
        espnowWrapper.end();
    }
  }
#if defined(ARDUINO_T_Watch)
  ttgo->tft->fillScreen(TFT_BLACK);
  ttgo->tft->setCursor(0,0);
  ttgo->tft->printf("Success! Channel: %d\r\nLoading defaults...\r\n", chan);
#endif
     espnowWrapper.end();
    delay(500);
#if defined(ESP8266)
        espNowRadioClient.alwaysSendBroadcast();
#endif

    espnowStart(chan);
    while(!espNowRadioClient.initDone()) {
      for(int i = 0;i < 100;i++) {
        espnowWrapper.loop();
        delay(1);
      }
    }
    espnowWrapper.end();
#if defined(ARDUINO_T_Watch)    
  ttgoSetup();
  setCpuFrequencyMhz(80);
#endif

}

void loop() {
  performanceLoop();
  if (volumeChangeTime)
    if (volumeChangeTime < millis()) 
    {
      Serial.printf("Now its time to change the volume to %d\r\n", volumeValue);
      espNowRadioClient.requestSetVolume(volumeValue);
      volumeChangeTime = 0;
      statusChangeTime = millis() + 2000;
    }
  if (presetChangeTime)
    if (presetChangeTime < millis()) 
    {
      Serial.printf("Now its time to change the preset to %d\r\n", presetValue);
      espNowRadioClient.requestSetPreset(presetValue);
      presetChangeTime = 0;
      statusChangeTime = millis() + 2000;
      statusFastChangeTime = millis() + 10000;
    }
    
  if (millis() > statusChangeTime) {
    Serial.println("Status Request!"); 
    espNowRadioClient.requestStatus();
    if (statusFastChangeTime > millis()) {
      Serial.println("Fast Status Update!");
      statusChangeTime = millis() + 2000;
    }
    else
      statusChangeTime = millis() + 15000;
  }
  while (Serial.available()) {
    int len = strlen(toSend);
    char c = Serial.read();
    if ((c == '\r') || (c == '\n')) {
      if (len > 0) {
        uint8_t packetId = 0xff;
        uint8_t len = 0;
//        int result = espnowWrapper.send(sendMac, (const unsigned char*)toSend, len+1); 
        if (toSend[0] == 'l') {
          espNowRadioClient.listPresets();
        } else if ((toSend[0] == 'v') || (toSend[0] == 'p')){
           len = 1;
           packetId = 200 + ((toSend[0] == 'v')?1:0);
           toSend[0] = atoi((char *)toSend + 1);
        } else {
          packetId = atoi(toSend);
          len = 1;
        }
        if (len) {
          espnowStart(espNowRadioClient.getChannel() & 0x7f);
 //         uint16_t id = atoi(toSend);
          uint16_t result = espNowRadioClient.setTX((const uint8_t*)toSend, len, packetId, 10);
          Serial.printf("TX set for client with id=%d has result=%d\r\n", packetId, result);
          for(int i = 0;i < 1500;i++) {
            espnowWrapper.loop();
            delay(1);
          }
          espnowWrapper.end();
        }
      }
      toSend[0] = 0;
    } else if (len < 249) {
      toSend[len++] = c;
      toSend[len] = 0;
    }
  }

#if defined(ARDUINO_T_Watch)
  ttgoLoop();  
#endif  
}



#if defined(ARDUINO_T_Watch)
void performanceLoop(void) {
    lv_task_handler();  
}


void ttgoLoop(void) {
    if (statusUpdated) {
    statusUpdated = false; 
    if (millis() > statusUpdateBlockTime) {
      lv_textarea_set_text(radioText, espNowRadioClient._radioText);
      lv_slider_set_value(volumeSlider, espNowRadioClient._curVolume, LV_ANIM_OFF);
      lv_dropdown_set_selected(stationList, espNowRadioClient._curPreset);    
//    lv_dropdown_set_show_selected(stationList, false);    
      lv_dropdown_set_text(stationList, espNowRadioClient._presets[espNowRadioClient._curPreset]);
      lv_event_send_refresh(stationList);
      Serial.printf("Preset = %d, RadioText = %s\r\n", espNowRadioClient._curPreset, espNowRadioClient._radioText);
    }
  }
  low_energy();

}

void low_energy()
{
    if (!pwrKeyFlag)
      return;
    ttgo->power->clearIRQ();
    pwrKeyFlag = false;
    if (ttgo->bl->isOn()) {
        ttgo->closeBL();
        ttgo->stopLvglTick();
//        ttgo->bma->enableStepCountInterrupt(false);
        ttgo->displaySleep();
        WiFi.mode(WIFI_OFF);
        setCpuFrequencyMhz (10); 
        while(!pwrKeyFlag)
          delay(100);
        setCpuFrequencyMhz (80); 
        ttgo->power->clearIRQ();
        pwrKeyFlag = false;  
        ttgo->startLvglTick();
        ttgo->displayWakeup();
        lv_disp_trig_activity(NULL);
        ttgo->openBL();
        statusChangeTime = millis() + 200;
    }
}
#else

void performanceLoop(void) {
  
}
#endif
