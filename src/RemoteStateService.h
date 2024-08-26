#ifndef _REMOTE_STATE_SERVICE_H_
#define _REMOTE_STATE_SERVICE_H_

#include <StatefulService.h>
#include <WebSocketTxRx.h>
#include <MqttPubSub.h>
#include <espMqttClientAsync.h>

#if FT_ENABLED(FT_NTP)
#include <time.h>
String formatTime(tm* time, const char* format);
#endif

#include "RFRemoteController.h"
#include "RemoteSettingsService.h"
#include "GarageStateService.h"

#define REMOTE_STATE_SOCKET_PATH "/ws/remoteState"

class RemoteState
{
public:
    Remote rem;
    bool isValid;
    bool isPairing;
    String error;

    static void read(RemoteState& settings, JsonObject& root)
    {
        root["remote_id"] = settings.rem.id();
        root["remote_button"] = settings.rem.button;
        root["remote_serial"] = settings.rem.getSerial();
        root["remote_description"] = settings.rem.description;
        root["pairing_error"] = settings.error;

#if FT_ENABLED(FT_NTP)
        time_t now = time(nullptr);
        root["remote_updated_at"] = formatTime(localtime(&now), "%FT%T");
#else
        // TODO
        root["remote_updated_at"] = millis();
#endif
    }

    static StateUpdateResult update(JsonObject& root, RemoteState& state)
    {
        bool newPairing = root["pairing"];

        if(state.isPairing != newPairing)
        {
            state.isPairing = newPairing;
            return StateUpdateResult::CHANGED;
        }

        return StateUpdateResult::UNCHANGED;
    }

    static void haRead(RemoteState& settings, JsonObject& root)
    {
        root["remote_button"] = settings.rem.button;
        root["remote_serial"] = settings.rem.getSerial();
        root["remote_description"] = settings.rem.description;
        root["remote_is_paired"] = settings.isValid;
        
#if FT_ENABLED(FT_NTP)
        time_t now = time(nullptr);
        root["remote_updated_at"] = formatTime(localtime(&now), "%FT%T");
#else
        // TODO
        root["remote_updated_at"] = millis();
#endif
    }

    static StateUpdateResult haUpdate(JsonObject& root, RemoteState& state)
    {
        return StateUpdateResult::UNCHANGED;
    }

    static void haPairingRead(RemoteState& settings, JsonObject& root)
    {
        root["state"] = settings.isPairing ? ON_STATE : OFF_STATE;
    }

    static StateUpdateResult haPairingUpdate(JsonObject& root, RemoteState& remoteState)
    {
        String state = root["state"];
        bool newState = false;

        if(state.equals(ON_STATE))
            newState = true;
        else if(!state.equals(OFF_STATE))
            return StateUpdateResult::ERROR;


        if(remoteState.isPairing != newState)
        {
            remoteState.isPairing = newState;
            return StateUpdateResult::CHANGED;
        }

        return StateUpdateResult::UNCHANGED;
    }
};

class RemoteStateService : public StatefulService<RemoteState>
{
public:
    RemoteStateService(AsyncWebServer* server, 
                        SecurityManager* security,
                        espMqttClientAsync* mqttClient, 
                        RfRemoteController* rfctrl,
                        RemoteSettingsService* remoteSettings,
                        GarageStateService* garageService);
    void begin();

private:
    espMqttClientAsync*         m_mqttClient;
    WebSocketTxRx<RemoteState>  m_websocket;
    RfRemoteController*         m_rfctrl;
    RemoteSettingsService*      m_remoteSettings;
    GarageStateService*         m_garage;
    bool                        m_wasPairing;
    MqttPubSub<RemoteState>     m_mqttPairingPubSub;
    MqttPubSub<RemoteState>     m_mqttRemotePubSub;

    void onRemoteReceived(RemotePacket packet, RemoteSerial serial);
    void onStateUpdate();
    void registerConfig();
    void registerDeviceTrigger();
    void registerPairingSwitch();
    void getDevice(JsonObject& dev);
};


#endif