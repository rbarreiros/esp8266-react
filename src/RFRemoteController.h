#ifndef _RFREMOTECONTROLLER_H_
#define _RFREMOTECONTROLLER_H_

#include <RF433recv.h>
#include <RemoteManager.h>

#define DEFAULT_RED_LED_STATE false
#define RED_LED_PIN     16
#define RED_LED_ON      LOW
#define RED_LED_OFF     HIGH

#define RF_PIN          15

// Requires at least 3 receptions with the same code
// to start validation. Should also check counter
#define MIN_PACKETS_ACCEPTED 3 

struct Remote
{
    uint8_t button;
    char description[64]; // must be null terminated!
    uint8_t serial[4];
};

using RfRemoteControllerCallback = std::function<void (RemotePacket packet, RemoteSerial serial)>; 

class RfRemoteController
{
public:
    RfRemoteController();
    static RfRemoteController* getPtr() { return m_thisPtr; };

    void begin();
    void loop();
    void processCode(const BitVector *recorded);
    void addCallback(RfRemoteControllerCallback cb) { if(m_cb == nullptr) m_cb = cb; }

private:
    RF_manager  m_rf;
    static RfRemoteController* m_thisPtr;
    uint16_t    m_lastHashReceived;
    uint8_t     m_lastHashCount;

    RemotePacket getPacket(const BitVector *recorded);
    // For now we just have 1 callback, later on we might have a vector
    RfRemoteControllerCallback  m_cb;
};




#endif