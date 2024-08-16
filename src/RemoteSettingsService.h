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

// HA pairing_timeout as a number
// HA remotes are remote entity

struct Remote
{
    uint8_t button;
    String description; // must be null terminated!
    RemoteSerial serial;

    String id() {
        static char buff[11];
        snprintf(buff, 11, "%02X%02X%02X%02X%02X",
            button, serial.ser[0], serial.ser[1], serial.ser[2], serial.ser[3]);

        return String(buff);
    }

    Remote()
        : button{0}, description{}, serial{0}
    {}

    Remote(uint8_t button, String description, RemoteSerial serial)
        : button{button}, description{description}, serial{serial}
    {}

    bool isEqual(RemoteSerial ser)
    {
        return serial == ser;
    }

    void setSerial(RemoteSerial ser)
    {
        serial = ser;
    }

    void setSerial(String ser)
    {
        for(size_t i = 0, j = 0; i < ser.length(); i += 2, j++)
        {
            char hex[3] = {ser[i], ser[i+1], '\0'};
            serial.ser[j] = (uint8_t)strtol(hex, NULL, 16);
        }
    }

    String getSerial()
    {
        return serial.toString();
    }

    inline bool operator==(const Remote& other)
    {
        return (serial == other.serial) && (button == other.button);
    }
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
        for(auto& rem : settings.remotes)
        {
            JsonObject r = remotes.add<JsonObject>();
            r["id"] = rem.id();
            r["button"] = rem.button;
            r["description"] = rem.description;
            r["serial"] = rem.serial.toString();
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

    // Writes to FS
    static void readFs(RemoteSettings& settings, JsonObject& root)
    {
        root["pairing_timeout"] = settings.pairingTimeout;

        JsonArray remotes = root["remotes"].to<JsonArray>();
        for(Remote rem : settings.remotes)
        {
            JsonObject r = remotes.add<JsonObject>();
            r["button"] = rem.button;
            r["description"] = rem.description;
            r["serial"] = rem.getSerial();
        }
    }

    // Reads from FS
    static StateUpdateResult updateFs(JsonObject& root, RemoteSettings& settings)
    {
        settings.pairingTimeout = root["pairing_timeout"] | REMOTE_DEFAULT_PAIRING_TIMEOUT;

        settings.remotes.clear();
        if(root["remotes"].is<JsonArray>())
        {
            for(JsonVariant rem : root["remotes"].as<JsonArray>())
            {
                Remote r;
                r.button = rem["button"].as<uint8_t>();
                r.description = rem["description"].as<String>();
                r.setSerial(rem["serial"].as<String>());

                settings.remotes.push_back(r);
            }
        }

        return StateUpdateResult::UNCHANGED;
    }

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
    unsigned long getPairingTimeout() { return _state.pairingTimeout; }

    bool addRemote(RemotePacket packet, RemoteSerial serial);
    bool addRemote(RemotePacket packet, RemoteSerial serial, String description);
    bool editRemote(String id, String description);
    Remote getRemote(String id);
    Remote getRemote(RemotePacket packet, RemoteSerial serial);
    bool delRemote(String id);

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