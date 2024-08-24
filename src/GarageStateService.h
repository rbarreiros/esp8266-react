#ifndef _GARAGESTATESERVICE_H_
#define _GARAGESTATESERVICE_H_

#include <MqttPubSub.h>
#include <StatefulService.h>
#include <WebSocketTxRx.h>
#include <GarageMqttSettingsService.h>

#include "RFRemoteController.h"

#define GARAGE_STATE_SOCKET_PATH "/ws/garageState"

#define DEFAULT_RELAY_STATE false

#define RELAY_PIN 16
#define RELAY_ON HIGH
#define RELAY_OFF LOW

#define OFF_STATE "OFF"
#define ON_STATE "ON"

// N.C endstops
// pull up to 5v

#define ENDSTOP_CLOSED_PIN 4
#define ENDSTOP_CLOSED_ON LOW
#define ENDSTOP_CLOSED_OFF HIGH
#define ENDSTOP_OPEN_PIN 2
#define ENDSTOP_OPEN_ON LOW
#define ENDSTOP_OPEN_OFF HIGH

// HA relay as a switch
// HA status as sensor
// HA endstops as binary_sensor

class GarageState
{
   public:
    enum GarageStatus_t
    {
        STATUS_ERROR = 0,
        STATUS_CLOSED,
        STATUS_OPENING,
        STATUS_CLOSING,
        STATUS_OPEN
    };

    static const String m_statusString[5];

    static String stateToString(GarageStatus_t state) 
        { return m_statusString[static_cast<uint8_t>(state)]; }

    bool relayOn;
    GarageStatus_t status = STATUS_ERROR;  // current status of garage door
    bool endstopClosed = false;            // status of close endstop
    bool endstopOpen = false;              // status of open endstop

    static void read(GarageState& settings, JsonObject& root)
    {
        root["relay_on"] = settings.relayOn;
        root["endstop_closed"] = settings.endstopClosed;
        root["endstop_open"] = settings.endstopOpen;
        root["status"] = settings.status;
    }

    static StateUpdateResult update(JsonObject& root, GarageState& garageState)
    {
        StateUpdateResult changed = StateUpdateResult::UNCHANGED;

        bool relayNewState = root["relay_on"] | DEFAULT_RELAY_STATE;

        if (garageState.relayOn != relayNewState)
        {
            garageState.relayOn = relayNewState;
            changed = StateUpdateResult::CHANGED;
        }

        return changed;
    }

    static void haRelayRead(GarageState& settings, JsonObject& root)
    {
        root["state"] = settings.relayOn ? ON_STATE : OFF_STATE;
        
        Serial.printf("Setting state to %s.\r\n", root["state"].as<const char*>());
    }

    static StateUpdateResult haRelayUpdate(JsonObject& root,
                                      GarageState& GarageState)
    {
        serializeJson(root, Serial);
        Serial.println("Got HA Update");
        return StateUpdateResult::UNCHANGED;
    }

    static void haStatusRead(GarageState& settings, JsonObject& root)
    {
        root["state"] = GarageState::stateToString(settings.status);
    }

    static void haEndstopOpenRead(GarageState& settings, JsonObject& root)
    {
        root["state"] = settings.endstopOpen ? ON_STATE : OFF_STATE;
    }

    static void haEndstopClosedRead(GarageState& settings, JsonObject& root)
    {
        root["state"] = settings.endstopClosed ? ON_STATE : OFF_STATE;
    }

    static StateUpdateResult haDummyUpdate(JsonObject& root,
                                      GarageState& GarageState)
    {
        return StateUpdateResult::UNCHANGED;
    }
};

class GarageStateService : public StatefulService<GarageState>
{
   public:
    GarageStateService(AsyncWebServer* server, SecurityManager* securityManager,
                       espMqttClientAsync* mqttClient, GarageMqttSettingsService* garageMqttSettings);
    void begin();
    void loop();

    void setRelayAutoOff(bool off) { m_relayAutoOff = off; }
    void setRelayOnTimer(unsigned long timer) { m_relayOnTimer = timer; }
    bool getRelayAutoOff() { return m_relayAutoOff; }
    unsigned long getRelayOnTimer() { return m_relayOnTimer; }

   private:
    WebSocketTxRx<GarageState>  m_webSocket;
    espMqttClientAsync*         m_mqttClient;
    GarageMqttSettingsService*  m_garageMqttSettings;
    MqttPubSub<GarageState>     m_mqttRelayPubSub;
    //MqttPubSub<GarageState>     m_mqttStatusPubSub;
    //MqttPubSub<GarageState>     m_mqttEndstopOpenPubSub;
    //MqttPubSub<GarageState>     m_mqttEndstopClosedPubSub;

    GarageState::GarageStatus_t m_lastEsState = GarageState::STATUS_ERROR;
    bool m_relayAutoOff;
    unsigned long m_relayOnTimer;

    void registerConfig();
    void onConfigUpdate();
    void updateEndstops();
};

#endif