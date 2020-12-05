#include "ESPNowWrapper.h"
#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}
#else if defined(ESP32)
#include <WiFi.h>
extern "C" {
  #include <esp_now.h>
  #include <esp_wifi.h>
}
#endif

#if defined(ESP8266)
void OnDataRecv(unsigned char* mac_addr, unsigned char *data, unsigned char data_len);
void OnDataSend(unsigned char *mac_addr, unsigned char status);
#else
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void OnDataSend(const uint8_t *mac_addr, esp_now_send_status_t status);
#endif

ESPNowWrapperClass::ESPNowWrapperClass() {
	_channel = 0xff;
	_recvCbUser = NULL;
	_sendCbUser = NULL;
	_firstServiceHandler = _lastServiceHandler = _lastSendingHandler = NULL;
	_tx = 0;
}

void ESPNowWrapperClass::addServiceHandler(ESPNowServiceHandlerClass* handler) {
	handler->next(NULL);
	handler->prev(_lastServiceHandler);
	if (_lastServiceHandler)
		_lastServiceHandler->next(handler);
	else
		_firstServiceHandler = handler;
	_lastServiceHandler = handler;
//	addPeer(handler->mac());
}



espnowwrapper_err_t ESPNowWrapperClass::begin(uint8_t channel = 0) {
//esp_err_t result = ESP_OK;
	if (_channel != 0xff)
		return ESPNOWWRAPPER_ERRBUSY;
	if (WL_CONNECTED == WiFi.status()) {
		if (channel)
			if (channel != WiFi.channel()) 
				return ESPNOWWRAPPER_ERRCHANNEL;
		_channel = 128 + WiFi.channel();
	}  else {
		if (!channel)
			channel = 1;
		_channel = channel; 
#if defined(ESP8266)
		wifi_station_disconnect();
		wifi_set_opmode(STATION_MODE);
		wifi_set_channel(channel);
		Serial.print("\r\n\r\nESP8266 Device MAC: ");
		Serial.println(WiFi.macAddress());
#else if defined(ESP32) 
		WiFi.persistent(false);
		WiFi.disconnect();
		WiFi.mode(WIFI_OFF);
//		delay(100);
		WiFi.mode(WIFI_STA);
		esp_wifi_set_promiscuous(true);
		esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
		esp_wifi_set_promiscuous(false);
		WiFi.disconnect();
//	Serial.printf("\r\nSetup WiFi ok? Channel = %d\r\n", WiFi.channel());
#endif
	}
	if (0 != esp_now_init()) {
		_channel = 0xff;
		return ESPNOWWRAPPER_ERRINIT;
	}
#if defined(ESP8266)
	esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
//	esp_now_register_recv_cb([this](uint8_t *mac, uint8_t *data, uint8_t len)
//		{
//		this->_recvCb((const uint8_t *)mac, (const uint8_t *)data, (const uint8_t)len);
//		});
#endif
#if defined(ESP32)
//    void *p = (void *)[this](const unsigned char *mac, const unsigned char *data, int len)
//		{
//		this->_recvCb((const uint8_t *)mac, (const uint8_t *)data, (const uint8_t)len);
//		};
//	esp_now_register_recv_cb((esp_now_recv_cb_t)p);

#endif
	esp_now_register_recv_cb(OnDataRecv);	
	esp_now_register_send_cb(OnDataSend);
	_lastSendingHandler = NULL;
	return ESPNOWWRAPPER_OK;//addPeer(broadcastMac);	
}	

espnowwrapper_err_t ESPNowWrapperClass::end() {
	if (0xff == _channel)
		return ESPNOWWRAPPER_ERRINIT;
	if (0 == esp_now_deinit()) {
		_channel = 0xff;
		ESPNowServiceHandlerClass* p = _firstServiceHandler;
		while (p) {
			p->gotDisconnected();
			p = p->next();
		}
		return ESPNOWWRAPPER_OK;
	}
	return ESPNOWWRAPPER_ERRBUSY;
}


void ESPNowWrapperClass::loop() {
ESPNowServiceHandlerClass* p = _firstServiceHandler;
bool dbgFlag = false;
bool txPossible = canSend();
	while (p) {
		if (dbgFlag |= p->hasRX())
			p->flushRX();
		else if (txPossible && (p->hasTX())) {
			if (ESPNOWWRAPPER_OK == p->flushTX())
				txPossible = false;
		}
		p = p->next();
	}
	if (dbgFlag)
		Serial.println("Loop done!");
}

uint8_t ESPNowWrapperClass::channel() {
	return _channel;
	if ((_channel == 0xff) || (_channel < 0x80))
			return _channel;
	return _channel & 0x7f;
}

bool ESPNowWrapperClass::canSend() {
	if (0xff == _channel)
		return false;
	if (_tx)
		if ((millis() - _lastSend) > ESPNOWWRAPPER_SENDTIMEOUT)
			_tx = 0;
	return 0 == _tx;
}

void ESPNowWrapperClass::_recvCb(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
bool done = false;
	if ((data_len >= 4) && (*data == ESPNOWSERVICE_MAGICID)) {
		ESPNowServiceHandlerClass* p = _firstServiceHandler;
		ESPNowServiceHandlerClass* bc = NULL;
		uint16_t id = *((uint16_t *)(data + 1));
		while (p && !done) {
			if (p->id() == id) {
				if (done = p->isThisMac(mac_addr))
					p->copyRx(mac_addr, data, data_len);
				else if (p->isBroadcastMac())
					bc = p;
			}
		    p = p->next();
		}
		if (!done)
			if (done = (NULL != bc))
				bc -> copyRx(mac_addr, data, data_len);
	    
	}
	if (!done)
		if (_recvCbUser)
			_recvCbUser(mac_addr, data, data_len);
}

void ESPNowWrapperClass::_sendCb(const uint8_t *mac_addr, const uint8_t status) {
	if (_tx)
		_tx--;
	if (_lastSendingHandler) 
//		if (_lastSendingHandler.isThisMac(mac_addr))
		{
			_lastSendingHandler->clearTX(0 == status);
			_lastSendingHandler->sendCb(status);
		}
	if (_sendCbUser)
		_sendCbUser(mac_addr, status);
}


espnowwrapper_err_t ESPNowWrapperClass::addPeer(const uint8_t *peerMac) {
//esp_err_t result = ESP_OK;
	int result;
	if (0xff == _channel)
		return ESPNOWWRAPPER_ERRINIT;
#if defined(ESP8266)
	result = esp_now_add_peer((unsigned char*)peerMac, ESP_NOW_ROLE_COMBO, (_channel & 0x7f), NULL, 0);
#else
	esp_now_peer_info_t peer;	
	memcpy( &peer.peer_addr, peerMac, 6 );
	peer.channel = (_channel & 0x7f);
	peer.encrypt = 0;
	peer.ifidx = ESP_IF_WIFI_STA;
	result = esp_now_add_peer(&peer);
#endif
	if (0 == result)
		return ESPNOWWRAPPER_OK;
#if defined(ESP8266)
	return ESPNOWWRAPPER_ERRBUSY;
#else
	if (ESP_ERR_ESPNOW_FULL == result)
		return ESPNOWWRAPPER_ERRFULL;
	if (ESP_ERR_ESPNOW_NO_MEM == result)
		return ESPNOWWRAPPER_ERRMEM;
	if (ESP_ERR_ESPNOW_EXIST == result)
		return ESPNOWWRAPPER_ERREXIST; 
	if (ESP_ERR_ESPNOW_ARG == result)
		return ESPNOWWRAPPER_ERRARG;
#endif
}

espnowwrapper_err_t ESPNowWrapperClass::hasPeer(const uint8_t *peerMac) {
//esp_err_t result = ESP_OK;
	int result;
	if (0xff == _channel)
		return ESPNOWWRAPPER_ERRINIT;
#if defined(ESP8266)
	result = esp_now_is_peer_exist((unsigned char*)peerMac);
	if (result)
		return ESPNOWWRAPPER_OK;
	return ESPNOWWRAPPER_ERRPEER;
#else
	esp_now_peer_info_t peer;	
    if (ESP_OK == esp_now_get_peer(peerMac, &peer))
		return ESPNOWWRAPPER_OK;
	return ESPNOWWRAPPER_ERRPEER;
#endif
	return ESPNOWWRAPPER_ERRBUSY;
}


#if defined(ESP8266)
void OnDataRecv(unsigned char* mac_addr, unsigned char *data, unsigned char data_len)
#else
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
#endif
{
	espnowWrapper._recvCb(mac_addr, data, data_len);
	return;
	/*
    esp_err_t sendResult = esp_now_send(slave.peer_addr, dataToSend,sizeof(dataToSend));
    if (sendResult == ESP_OK) {
      Serial.print("Success sendingS response");
    } else if (sendResult == ESP_ERR_ESPNOW_NOT_INIT) {
      // How did we get so far!!
      Serial.println("ESPNOW not Init.");
    } else if (sendResult == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Invalid Argument");
    } else if (sendResult == ESP_ERR_ESPNOW_INTERNAL) {
      Serial.println("Internal Error");
    } else if (sendResult == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("ESP_ERR_ESPNOW_NO_MEM");
    } else if (sendResult == ESP_ERR_ESPNOW_NOT_FOUND) {
      Serial.println("Peer not found.");
    }
    else if (sendResult == ESP_ERR_ESPNOW_IF) {
      Serial.println("Interface Error.");
    } else {
      Serial.printf("\r\nNot sure what happened\t%d", sendResult);
    }
  } else {
        Serial.printf("\r\nReceived\t%d Bytes\t%d\n", data_len, data[0]);
    }
	*/
}

#if defined(ESP8266)
void OnDataSend(unsigned char *mac_addr, unsigned char status) {
    espnowWrapper._sendCb((const uint8_t *)mac_addr, (const uint8_t)status);
#else
void OnDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
    espnowWrapper._sendCb((const uint8_t *)mac_addr, (const uint8_t)status);
#endif
	if (espnowWrapper._tx)
		espnowWrapper._tx--;
}

espnowwrapper_err_t ESPNowWrapperClass::send(const uint8_t *mac_addr, const uint8_t *data, int data_len,
					ESPNowServiceHandlerClass* lastTx) {
	if (0xff == _channel) {
		Serial.println("Error channel for ESPNow-Send!");
	    if (lastTx)
			lastTx->sendFail();
     	return ESPNOWWRAPPER_ERRINIT;
	}
#if defined(ESP8266)
	if (0 == esp_now_send((unsigned char*)mac_addr, (unsigned char *)data, data_len)) {
#else
	uint8_t res;
	Serial.printf("Send <%ld:%ld> len %d\r\n", mac_addr, data, data_len);
	if (0 != (res = esp_now_send(mac_addr, data, data_len))) {
		Serial.printf("ESP32 send fails with %d\r\n", res);
		if (ESP_NOW_SEND_FAIL)
			Serial.printf("Das ist Send Fail!\r\n");
	}
	if (0 == res) {
#endif
		_tx++;
		_lastSend = millis();
		_lastSendingHandler = lastTx;
		return ESPNOWWRAPPER_OK;
	} else if (lastTx)
		lastTx->sendFail();
	return ESPNOWWRAPPER_ERRBUSY;
}

void ESPNowWrapperClass::setRecvCb(void (*recvCbUser)(const uint8_t *mac_addr, const uint8_t *data, int data_len)) {
	_recvCbUser = recvCbUser;
}

void ESPNowWrapperClass::setSendCb(void (*sendCbUser)(const uint8_t *mac_addr, const uint8_t status)){
	_sendCbUser = sendCbUser;
}

ESPNowServiceHandlerClass::ESPNowServiceHandlerClass(const uint8_t *mac, uint16_t id) {
	setMac(mac);
	_id = id;
	_next = _prev = NULL;
	_rx = _isPeer = false;
	_tx = 0;
}

/*ESPNowServiceHandlerClass::ESPNowServiceHandlerClass(uint8_t *mac, ESPNowServiceHandlerClass *origin) {
	setMac(mac);
	_id = origin->_id;
	_rx = origin->_rx;
	_tx = origin->_tx;
	_isPeer = origin->_isPeer;
	memcpy(_data, origin->_data, ESPNOWSERVICE_BUFSIZE);
}*/

void ESPNowServiceHandlerClass::dataCopy(ESPNowServiceHandlerClass *origin) {
	//setMac(mac);
	_id = origin->_id;
	_rx = origin->_rx;
	_tx = origin->_tx;
	_isPeer = origin->_isPeer;
	_dataLen = origin->_dataLen;
	_rxdataLen = origin->_rxdataLen;
	memcpy(_data, origin->_data, _dataLen);
	memcpy(_rxdata, origin->_rxdata, _rxdataLen);
}


void ESPNowServiceHandlerClass::setMac(const uint8_t *mac) {
	memcpy(_mac, mac, 6);
	_isPeer = false;
}

void ESPNowServiceHandlerClass::disconnected() {
}


bool ESPNowServiceHandlerClass::isThisMac(const uint8_t* mac) {
	return 0 == memcmp(_mac, mac, 6);
}

bool ESPNowServiceHandlerClass::isBroadcastMac() {
	return 0 == memcmp(_mac, broadcastMac, 6);
}

espnowwrapper_err_t ESPNowServiceHandlerClass::setTX(const uint8_t *data, uint8_t len, uint8_t packetId, uint8_t retries, bool isServicePacket) {
espnowwrapper_err_t result;
	if ((len > ESPNOWSERVICE_DATALEN) || ((len == 0) && !isServicePacket))
		return ESPNOWWRAPPER_ERRLEN;
	if (espnowWrapper.channel() == 255) {
		_rx  = _isPeer = false;
		_tx = 0;
		return ESPNOWWRAPPER_ERRINIT;
	}
	if (!_isPeer)
		if (ESPNOWWRAPPER_OK != espnowWrapper.hasPeer(_mac))
			if (ESPNOWWRAPPER_OK != espnowWrapper.addPeer(_mac))
				return ESPNOWWRAPPER_ERRPEER;
	_isPeer = true;
	_data[0] = ESPNOWSERVICE_MAGICID;
	_data[3] = packetId;
	*((uint16_t *)(_data + 1)) = _id;
	_data[4 + len] = 0;
	if ((0 != len) && (_data + 4 != data))
		memcpy(_data+4, data, len);
	_tx = retries;
//	_rx = false;
	_dataLen = len + 5;
	return ESPNOWWRAPPER_OK;
//	espnowWrapper.send(_mac, _data, len + 5);
}

espnowwrapper_err_t ESPNowServiceHandlerClass::flushTX() {
	if (!_tx)
		return ESPNOWWRAPPER_ERRDATA;
	if (espnowWrapper.channel() == 255) {
		_rx = _isPeer = false;
		_tx = 0;
		return ESPNOWWRAPPER_ERRINIT;
	}
	if (!_isPeer) {
		if (ESPNOWWRAPPER_OK != espnowWrapper.hasPeer(_mac))
			if (ESPNOWWRAPPER_OK != espnowWrapper.addPeer(_mac))
				return ESPNOWWRAPPER_ERRPEER;
		_isPeer = true;
	}
	if (ESPNOWWRAPPER_OK != 	espnowWrapper.send(_mac, _data, _dataLen, this)) {
		clearTX();
		return ESPNOWWRAPPER_ERRINIT;
	}
	return ESPNOWWRAPPER_OK;
}

void ESPNowServiceHandlerClass::clearTX(bool success) {
	if (_tx)
		if (success)
			_tx = 0;
		else
			_tx--;
}

void ESPNowServiceHandlerClass::next(ESPNowServiceHandlerClass *next) {
	_next = next;
}

ESPNowServiceHandlerClass* ESPNowServiceHandlerClass::next() {
	return _next;
}

void ESPNowServiceHandlerClass::prev(ESPNowServiceHandlerClass *prev) {
	_prev = prev;
}

ESPNowServiceHandlerClass* ESPNowServiceHandlerClass::prev() {
	return _prev;
}

uint16_t ESPNowServiceHandlerClass::id() {
	return _id;
}

bool ESPNowServiceHandlerClass::hasRX() {
	return _rx;
}

bool ESPNowServiceHandlerClass::hasTX() {
	return _tx;
}

bool ESPNowServiceHandlerClass::isReadyToSend() {
	return !_tx;
}

uint8_t* ESPNowServiceHandlerClass::getTXBuffer() {
	return _data + 4;
}

void ESPNowServiceHandlerClass::sendCb(uint8_t status) {
}

void ESPNowServiceHandlerClass::sendFail() {
}

void ESPNowServiceHandlerClass::flushRX(const uint8_t *mac, const uint8_t packetId, const uint8_t *data, const uint8_t dataLen) {
/*	if (!_rx)
		return;
	_rx = false;
	if (_data[7])
		handleRx(_data, _data[6], _data + 8, _data[7]);
	else {
		//protocolDataHandling;
	}
*/
}

void ESPNowServiceHandlerClass::flushRX() {
	if (!_rx)
		return;
	if (_rxdataLen > 4)
		flushRX(_rxdata, _rxdata[9], _rxdata + 10, _rxdataLen - 5);
	else {
		//protocolDataHandling;
	}
	_rx = false;
//	if (isBroadcastMac())
//		Serial.println("RX jetzt auf false gesetzt für BroadcastMac");
}

void ESPNowServiceHandlerClass::gotDisconnected() {
	_tx = 0;
	_rx = _isPeer = false;
	disconnected();
}

void ESPNowServiceHandlerClass::copyRx(const uint8_t* mac, const uint8_t* data, const uint8_t len) {
	Serial.println("copyRX");
//	_tx = 0;
	_rx = true;
	_rxdataLen = len;
	memcpy(_rxdata, mac, 6);
	memcpy(_rxdata + 6, data, len);
}

const uint8_t* ESPNowServiceHandlerClass::mac() {
	return _mac;
}

ESPNowWrapperClass espnowWrapper; 
const uint8_t broadcastMac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};