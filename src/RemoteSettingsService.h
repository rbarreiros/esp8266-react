#ifndef _REMOTESETTINGSSERVICE_H_
#define _REMOTESETTINGSSERVICE_H_

#include <StatefulService.h>
#include <HttpEndpoint.h>
#include <MqttPubSub.h>
#include <WebSocketTxRx.h>
#include <FSPersistence.h>
#include <AsyncTimer.h>

extern AsyncTimer Timer; // Global timer

#include "RFRemoteController.h"
#include "GarageStateService.h"

#define REMOTE_SETTINGS_FILE            "config/remotes.json"
#define REMOTE_SETTINGS_ENDPOINT_PATH   "/rest/remoteSettings"
#define REMOTE_SETTINGS_SOCKET_PATH     "/ws/remoteSettings"

#define REMOTE_CRUD_CREATE_ENDPOINT_PATH       "/rest/remote/create"
#define REMOTE_CRUD_READ_ENDPOINT_PATH         "/rest/remote/read"
#define REMOTE_CRUD_UPDATE_ENDPOINT_PATH       "/rest/remote/update"
#define REMOTE_CRUD_DELETE_ENDPOINT_PATH       "/rest/remote/delete"

#define REMOTE_DEFAULT_PAIRING_TIMEOUT  60000 // 60 seconds, 1 minute

struct Remote
{
    uint8_t button;
    char description[64]; // must be null terminated!
    uint8_t serial[4];
};

using RemoteList = std::vector<Remote>;

class RemoteSettings
{
public:
    RemoteList remotes;
    unsigned long pairingTimeout; // add to settings

    static void read(RemoteSettings& settings, JsonObject& root)
    {
        root["pairing_timeout"] = settings.pairingTimeout;

        JsonArray remotes = root["remotes"].to<JsonArray>();
        for(Remote rem : settings.remotes)
        {
            JsonObject r = remotes.add<JsonObject>();
            r["button"] = rem.button;
            r["description"] = rem.description;
            r["serial"] = rem.serial;
        }
    }

    static StateUpdateResult update(JsonObject& root, RemoteSettings& settings)
    {
        StateUpdateResult changed = StateUpdateResult::UNCHANGED;

        // Save this for batch operations and general settings

        unsigned long timeout = root["pairing_timeout"] | REMOTE_DEFAULT_PAIRING_TIMEOUT;

        if(timeout != settings.pairingTimeout)
        {
            settings.pairingTimeout = timeout;
            changed = StateUpdateResult::CHANGED;
        }

        return changed;
    }

    // Websocket
    // Pairing
    //static void wsRead(RemoteSettings& settings, JsonObject& root);
    //static StateUpdateResult wsUpdate(JsonObject& root, RemoteSettings& settings);

    // MQTT

    static void haRead(RemoteSettings& settings, JsonObject& root)
    {}

    static StateUpdateResult haUpdate(JsonObject& root, RemoteSettings& settings)
    {
        // batch operations, probably to be ignored in HA
        return StateUpdateResult::UNCHANGED;
    }
};

class RemoteSettingsService : public StatefulService<RemoteSettings>
{
public:
    RemoteSettingsService(
        AsyncWebServer *server,
        SecurityManager *security,
        FS *fs,
        GarageStateService *garage,
        RfRemoteController *remoteCtrl
    );

    void begin();
    
    RemoteList& getRemotes() { return _state.remotes; }
    String getDescription(RemotePacket packet, RemoteSerial serial);
    bool isValid(RemotePacket packet, RemoteSerial serial);

private:
    HttpEndpoint<RemoteSettings>    m_httpEndpoint;
    WebSocketTxRx<RemoteSettings>   m_webSocket; // Only used for pairing remotes
    FSPersistence<RemoteSettings>   m_fs;
    SecurityManager*                m_security;
    GarageStateService*             m_garageService;
    RfRemoteController*             m_remoteCtrl;

    AsyncCallbackJsonWebHandler     m_createHandler;
    AsyncCallbackJsonWebHandler     m_updateHandler;
    AsyncCallbackJsonWebHandler     m_deleteHandler;

    // Single remote crud operations
    void createRemote(AsyncWebServerRequest *request, JsonVariant &json);
    void readRemote(AsyncWebServerRequest *request);
    void updateRemote(AsyncWebServerRequest *request, JsonVariant &json);
    void deleteRemote(AsyncWebServerRequest *request, JsonVariant &json);
    
    //void onRemoteReceived(RemotePacket packet, RemoteSerial serial);
};



#endif