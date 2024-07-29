#ifndef _RELAYMQTTSETTINGSERVICE_H_
#define _RELAYMQTTSETTINGSERVICE_H_

#include <HttpEndpoint.h>
#include <StatefulService.h>
#include <FSPersistence.h>

#define RELAY_SETTINGS_FILE "/config/relay.json"
#define RELAY_SETTINGS_PATH "/rest/relaySettings"

class RelayMqttSettings
{
public:
    String mqttPath;
    String name;
    String uniqueId;

    static void read(RelayMqttSettings& settings, JsonObject& root);
    static StateUpdateResult update(JsonObject& root, RelayMqttSettings& settings);
};

class RelayMqttSettingsService : public StatefulService<RelayMqttSettings>
{
public:
    RelayMqttSettingsService(AsyncWebServer* server,
                             FS* fs,
                             SecurityManager* securityManager);
    void begin();
private:
    HttpEndpoint<RelayMqttSettings>     m_httpEndpoint;
    FSPersistence<RelayMqttSettings>    m_fsPersistence;
};

#endif