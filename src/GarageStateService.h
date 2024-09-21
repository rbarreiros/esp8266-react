#ifndef _GARAGESTATESERVICE_H_
#define _GARAGESTATESERVICE_H_

#include <MqttPubSub.h>
#include <StatefulService.h>
#include <WebSocketTxRx.h>

#include "RFRemoteController.h"

#ifndef FACTORY_SYSTEM_RELAY_PIN
#define FACTORY_SYSTEM_RELAY_PIN 16
#endif

#ifndef FACTORY_SYSTEM_RELAY_SINK
#define FACTORY_SYSTEM_RELAY_SINK false
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_OPEN_PIN
#define FACTORY_SYSTEM_ENDSTOP_OPEN_PIN 2
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_OPEN_PULLUP
#define FACTORY_SYSTEM_ENDSTOP_OPEN_PULLUP true
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_OPEN_ON
#define FACTORY_SYSTEM_ENDSTOP_OPEN_ON LOW
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_OPEN_OFF
#define FACTORY_SYSTEM_ENDSTOP_OPEN_OFF HIGH
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_CLOSE_PIN
#define FACTORY_SYSTEM_ENDSTOP_CLOSE_PIN 4
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_CLOSE_PULLUP
#define FACTORY_SYSTEM_ENDSTOP_CLOSE_PULLUP true
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_CLOSE_ON
#define FACTORY_SYSTEM_ENDSTOP_CLOSE_ON LOW
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_CLOSE_OFF
#define FACTORY_SYSTEM_ENDSTOP_CLOSE_OFF HIGH
#endif

#ifndef FACTORY_SYSTEM_BARRIER_PIN
#define FACTORY_SYSTEM_BARRIER_PIN 17
#endif

#ifndef FACTORY_SYSTEM_BARRIER_PULLUP
#define FACTORY_SYSTEM_BARRIER_PULLUP true
#endif

#ifndef FACTORY_SYSTEM_BARRIER_ON
#define FACTORY_SYSTEM_BARRIER_ON LOW
#endif

#ifndef FACTORY_SYSTEM_BARRIER_OFF
#define FACTORY_SYSTEM_BARRIER_OFF HIGH
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_OPEN_LED
#define FACTORY_SYSTEM_ENDSTOP_OPEN_LED true
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_OPEN_LED_PIN
#define FACTORY_SYSTEM_ENDSTOP_OPEN_LED_PIN 19
#endif 

#ifndef FACTORY_SYSTEM_ENDSTOP_OPEN_LED_SINK
#define FACTORY_SYSTEM_ENDSTOP_OPEN_LED_SINK true
#endif 

#ifndef FACTORY_SYSTEM_ENDSTOP_CLOSE_LED
#define FACTORY_SYSTEM_ENDSTOP_CLOSE_LED true
#endif

#ifndef FACTORY_SYSTEM_ENDSTOP_CLOSE_LED_PIN
#define FACTORY_SYSTEM_ENDSTOP_CLOSE_LED_PIN 21
#endif 

#ifndef FACTORY_SYSTEM_ENDSTOP_CLOSE_LED_SINK
#define FACTORY_SYSTEM_ENDSTOP_CLOSE_LED_SINK true
#endif

#ifndef FACTORY_SYSTEM_BARRIER_LED
#define FACTORY_SYSTEM_BARRIER_LED true
#endif

#ifndef FACTORY_SYSTEM_BARRIER_LED_PIN
#define FACTORY_SYSTEM_BARRIER_LED_PIN 17
#endif

#ifndef FACTORY_SYSTEM_BARRIER_LED_SINK
#define FACTORY_SYSTEM_BARRIER_LED_SINK true
#endif

#define GARAGE_STATE_SOCKET_PATH "/ws/garageState"

#define DEFAULT_RELAY_STATE false

#define OFF_STATE "OFF"
#define ON_STATE "ON"

#define RELAY_ON  (FACTORY_SYSTEM_RELAY_SINK ? LOW : HIGH)
#define RELAY_OFF !RELAY_ON

#if FACTORY_SYSTEM_ENDSTOP_OPEN_LED == true
#define ENDSTOP_OPEN_LED_ON (FACTORY_SYSTEM_ENDSTOP_OPEN_LED_SINK ? LOW : HIGH)
#define ENDSTOP_OPEN_LED_OFF !ENDSTOP_OPEN_LED_ON
#endif

#if FACTORY_SYSTEM_ENDSTOP_CLOSE_LED == true
#define ENDSTOP_CLOSE_LED_ON (FACTORY_SYSTEM_ENDSTOP_CLOSE_LED_SINK ? LOW : HIGH)
#define ENDSTOP_CLOSE_LED_OFF !ENDSTOP_CLOSE_LED_ON
#endif

#if FACTORY_SYSTEM_BARRIER_LED == true
#define BARRIER_LED_ON (FACTORY_SYSTEM_BARRIER_LED_SINK ? LOW : HIGH)
#define BARRIER_LED_OFF !BARRIER_LED_ON
#endif

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
    bool barrierTriggered = false;          // status of IR barrier

    static void read(GarageState& settings, JsonObject& root)
    {
        root["relay_on"] = settings.relayOn;
        root["endstop_closed"] = settings.endstopClosed;
        root["endstop_open"] = settings.endstopOpen;
        root["barrier_triggered"] = settings.barrierTriggered;
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
                                      GarageState& garageState)
    {
        String state = root["state"];
        bool newState = false;

        if(state.equals(ON_STATE))
            newState = true;
        else if(!state.equals(OFF_STATE))
            return StateUpdateResult::ERROR;


        if(garageState.relayOn != newState)
        {
            garageState.relayOn = newState;
            return StateUpdateResult::CHANGED;
        }

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

    static void haBarrierTriggeredRead(GarageState& settings, JsonObject& root)
    {
        root["state"] = settings.barrierTriggered ? ON_STATE : OFF_STATE;
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
                       espMqttClientAsync* mqttClient);
    void begin();
    void loop();

    void setRelayAutoOff(bool off) { m_relayAutoOff = off; }
    void setRelayOnTimer(unsigned long timer) { m_relayOnTimer = timer; }
    bool getRelayAutoOff() { return m_relayAutoOff; }
    unsigned long getRelayOnTimer() { return m_relayOnTimer; }

   private:
    WebSocketTxRx<GarageState>  m_webSocket;
    espMqttClientAsync*         m_mqttClient;
    MqttPubSub<GarageState>     m_mqttBarrierPubSub;
    MqttPubSub<GarageState>     m_mqttEndstopClosedPubSub;
    MqttPubSub<GarageState>     m_mqttEndstopOpenPubSub;
    MqttPubSub<GarageState>     m_mqttStatusPubSub;
    MqttPubSub<GarageState>     m_mqttRelayPubSub;

    GarageState::GarageStatus_t m_lastEsState = GarageState::STATUS_ERROR;
    bool m_relayAutoOff;
    unsigned long m_relayOnTimer;

    void registerConfig();
    void onConfigUpdate();
    void updateEndstops();


    void getDevice(JsonObject& dev);
    void registerRelay();
    void registerStatus();
    void registerEndstopOpen();
    void registerEndstopClosed();
    void registerBarrier();
};

#endif