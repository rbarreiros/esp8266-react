#ifndef _RELAYSTATESERVICE_H_
#define _RELAYSTATESERVICE_H_

#include <HttpEndpoint.h>
#include <StatefulService.h>
#include <MqttPubSub.h>
#include <WebSocketTxRx.h>
#include <FSPersistence.h>
#include "GarageMqttSettingsService.h"

// TODO Move blue led to wifi status

#define GARAGE_SETTINGS_ENDPOINT_PATH "/rest/garageState"
#define GARAGE_SETTINGS_SOCKET_PATH "/ws/garageState"

#define DEFAULT_RELAY_STATE false
#define DEFAULT_RELAY_AUTO_OFF true
#define DEFAULT_RELAY_TIMER 1000

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

#define GARAGE_STATE_SETTINGS_FILE "/config/garageState.json"

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
    bool relayAutoOff;
    unsigned long relayOnTimer;

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
                       SecurityManager* securityManager,
                       espMqttClientAsync* mqttClient,
                       FS *fs,
                       GarageMqttSettingsService* garageMqttSettingsService);
    void begin();
    void loop();

private:
    HttpEndpoint<GarageState>   m_httpEndpoint;
    MqttPubSub<GarageState>     m_mqttPubSub;
    WebSocketTxRx<GarageState>  m_webSocket;
    espMqttClientAsync*         m_mqttClient;
    FSPersistence<GarageState>  m_fs;
    GarageMqttSettingsService*  m_garageMqttSettingsService;

    GarageState::GarageStatus_t m_lastEsState = GarageState::STATUS_ERROR;

    void registerConfig();
    void onConfigUpdate();
    void updateEndstops();
};

#endif