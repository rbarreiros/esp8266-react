#ifndef _REMOTE_STATE_SERVICE_H_
#define _REMOTE_STATE_SERVICE_H_

#include <StatefulService.h>
#include <WebSocketTxRx.h>
#include <RemoteManager.h>

#include "RFRemoteController.h"
#include "RemoteSettingsService.h"
#include "GarageStateService.h"

#define REMOTE_STATE_SOCKET_PATH "/ws/garageState"

class RemoteState
{
public:
    RemotePacket rxPacket;
    RemoteSerial rxSerial;
    String description;
    bool isValid;
    bool isPairing;


    static void read(RemoteState& settings, JsonObject& root)
    {
        char serial[9] = {0};
        snprintf(serial, 8, serial, "%02X%02X%02X%02X", settings.rxSerial.ser[0],
                                                        settings.rxSerial.ser[1],
                                                        settings.rxSerial.ser[2],
                                                        settings.rxSerial.ser[3]);
        serial[8] = '\0';

        root["remote_button"] = settings.rxPacket.button;
        root["remote_serial"] = serial;
        root["remote_description"] = settings.description;
        root["remote_updated_at"] = millis();
    }

    static StateUpdateResult update(JsonObject& root, RemoteState& relayState)
    {
        return StateUpdateResult::UNCHANGED;
    }

    static void haRead(RemoteState& settings, JsonObject& root)
    {}

    static StateUpdateResult haUpdate(JsonObject& root, RemoteState& relayState)
    {
        return StateUpdateResult::UNCHANGED;
    }

};

class RemoteStateService : public StatefulService<RemoteState>
{
public:
    RemoteStateService(AsyncWebServer* server, 
                        SecurityManager* security, 
                        RfRemoteController* rfctrl,
                        RemoteSettingsService* remoteSettings,
                        GarageStateService* garageService);
    void begin();

private:
    WebSocketTxRx<RemoteState>  m_websocket;
    RfRemoteController*         m_rfctrl;
    RemoteSettingsService*      m_remoteSettings;
    GarageStateService*         m_garage;

    void onRemoteReceived(RemotePacket packet, RemoteSerial serial);
};


#endif