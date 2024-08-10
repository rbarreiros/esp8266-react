#ifndef _GARAGESTATESERVICE_H_
#define _GARAGESTATESERVICE_H_

#include <StatefulService.h>
#include <WebSocketTxRx.h>

#include "RFRemoteController.h"

#define GARAGE_STATE_SOCKET_PATH "/ws/garageState"

#define DEFAULT_RELAY_STATE false

#define RELAY_PIN       5
#define RELAY_ON        HIGH
#define RELAY_OFF       LOW

#define OFF_STATE "OFF"
#define ON_STATE  "ON"

// N.C endstops
// pull up to 5v

#define ENDSTOP_CLOSED_PIN  4
#define ENDSTOP_CLOSED_ON   LOW
#define ENDSTOP_CLOSED_OFF  HIGH
#define ENDSTOP_OPEN_PIN    2
#define ENDSTOP_OPEN_ON     LOW
#define ENDSTOP_OPEN_OFF    HIGH

class GarageState
{
public:
    enum GarageStatus_t {
        STATUS_ERROR = 0,
        STATUS_CLOSED,
        STATUS_OPENING,
        STATUS_CLOSING,
        STATUS_OPEN
    };

    bool relayOn;
    GarageStatus_t status = STATUS_ERROR;   // current status of garage door
    bool endstopClosed = false;             // status of close endstop 
    bool endstopOpen = false;               // status of open endstop

    static void read(GarageState& settings, JsonObject& root);
    static StateUpdateResult update(JsonObject& root, GarageState& relayState);
    static void haRead(GarageState& settings, JsonObject& root);
    static StateUpdateResult haUpdate(JsonObject& root, GarageState& relayState);
};

class GarageStateService : public StatefulService<GarageState>
{
public:
    GarageStateService(AsyncWebServer* server,
                       SecurityManager* securityManager);
    void begin();
    void loop();

    void setRelayAutoOff(bool off) { m_relayAutoOff = off; }
    void setRelayOnTimer(unsigned long timer) { m_relayOnTimer = timer; }
    bool getRelayAutoOff() { return m_relayAutoOff; }
    unsigned long getRelayOnTimer() { return m_relayOnTimer; }

private:
    WebSocketTxRx<GarageState>  m_webSocket;

    GarageState::GarageStatus_t m_lastEsState = GarageState::STATUS_ERROR;
    bool                        m_relayAutoOff;
    unsigned long               m_relayOnTimer;




    void registerConfig();
    void onConfigUpdate();
    void updateEndstops();
};

#endif