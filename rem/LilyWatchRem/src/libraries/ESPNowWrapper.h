#ifndef __ESPNOWWRAPPER_H__
#define __ESPNOWWRAPPER_H__
#include <Arduino.h>
#define ESPNOWWRAPPER_SENDTIMEOUT 1000
#define ESPNOWSERVICE_MAGICID   'a'
#define ESPNOWSERVICE_DATALEN   240
#define ESPNOWSERVICE_BUFSIZE   (ESPNOWSERVICE_DATALEN + 8)
typedef enum {
	ESPNOWWRAPPER_OK,
	ESPNOWWRAPPER_ERRCHANNEL,
	ESPNOWWRAPPER_ERRBUSY,
	ESPNOWWRAPPER_ERRINIT,
	ESPNOWWRAPPER_ERRFULL,
	ESPNOWWRAPPER_ERRMEM,
	ESPNOWWRAPPER_ERREXIST, 
	ESPNOWWRAPPER_ERRARG,
	ESPNOWWRAPPER_ERRLEN,
	ESPNOWWRAPPER_ERRPEER,
	ESPNOWWRAPPER_ERRDATA
} espnowwrapper_err_t;

class ESPNowServiceHandlerClass {
	public:
		ESPNowServiceHandlerClass(const uint8_t *mac, uint16_t id);
		void dataCopy(ESPNowServiceHandlerClass *origin);
		uint8_t *getTXBuffer();
		const uint8_t* mac();
		void setMac(const uint8_t *mac);
		bool isBroadcastMac();
		bool isThisMac(const uint8_t* mac);
		espnowwrapper_err_t setTX(const uint8_t *data, uint8_t len, uint8_t packetId = 0, 
					uint8_t retries = 1, bool isServicePacket = false);
		void next(ESPNowServiceHandlerClass *next);
		ESPNowServiceHandlerClass* next();
		void prev(ESPNowServiceHandlerClass *prev);
		ESPNowServiceHandlerClass* prev();
		virtual void flushRX(const uint8_t *mac, const uint8_t packetId, const uint8_t *data, const uint8_t dataLen);
		void flushRX();
		espnowwrapper_err_t flushTX();
		void copyRx(const uint8_t* mac, const uint8_t* data, const uint8_t len);
		uint16_t id();
		bool hasRX();
		bool hasTX();
		bool isReadyToSend();
		void gotDisconnected();
		virtual void disconnected();
		virtual void sendCb(uint8_t status);
		virtual void sendFail();
		void clearTX(bool success = false);
	protected:
		uint8_t _mac[6];
		uint16_t _id;
		bool _rx, _isPeer;
		uint8_t _tx;
		uint8_t _rxdata[ESPNOWSERVICE_BUFSIZE], _data[ESPNOWSERVICE_BUFSIZE];
		uint8_t _dataLen, _rxdataLen;
		ESPNowServiceHandlerClass *_next, *_prev;
};

class ESPNowWrapperClass {
public:
    ESPNowWrapperClass();
	espnowwrapper_err_t begin(uint8_t channel);
	espnowwrapper_err_t end();
	espnowwrapper_err_t addPeer(const uint8_t *peerMac);
	espnowwrapper_err_t hasPeer(const uint8_t *peerMac);
	espnowwrapper_err_t send(const uint8_t *mac_addr, const uint8_t *data, int data_len, ESPNowServiceHandlerClass* lastTx = NULL);
	void setRecvCb(void (*recvCbUser)(const uint8_t *mac_addr, const uint8_t *data, int data_len));
	void setSendCb(void (*sendCbUser)(const uint8_t *mac_addr, const uint8_t status));
	void _recvCb(const uint8_t *mac_addr, const uint8_t *data, int data_len);
	void _sendCb(const uint8_t *mac_addr, const uint8_t status);
	uint8_t channel();
	void addServiceHandler(ESPNowServiceHandlerClass* handler);
	uint16_t _tx;
	bool canSend();
	void loop();
protected:
    uint32_t _lastSend;
	ESPNowServiceHandlerClass *_firstServiceHandler, *_lastServiceHandler, *_lastSendingHandler;
	uint8_t _channel;
	void (*_recvCbUser)(const uint8_t *mac_addr, const uint8_t *data, int data_len);
	void (*_sendCbUser)(const uint8_t *mac_addr, const uint8_t status);
};

extern ESPNowWrapperClass espnowWrapper; 
extern const uint8_t broadcastMac[6];

#endif