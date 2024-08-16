#ifndef _GARAGEMQTTSETTINGSERVICE_H_
#define _GARAGEMQTTSETTINGSERVICE_H_

#include <HttpEndpoint.h>
#include <StatefulService.h>
#include <FSPersistence.h>

#define GARAGE_MQTT_SETTINGS_FILE "/config/brokerSettings.json"
#define GARAGE_MQTT_SETTINGS_PATH "/rest/brokerSettings"

class GarageMqttSettings
{
public:
    String mqttPath;
    String name;
    String uniqueId;

    static void read(GarageMqttSettings& settings, JsonObject& root);
    static StateUpdateResult update(JsonObject& root, GarageMqttSettings& settings);
};

class GarageMqttSettingsService : public StatefulService<GarageMqttSettings>
{
public:
    GarageMqttSettingsService(AsyncWebServer* server,
                             FS* fs,
                             SecurityManager* securityManager);
    void begin();
private:
    HttpEndpoint<GarageMqttSettings>     m_httpEndpoint;
    FSPersistence<GarageMqttSettings>    m_fsPersistence;
};

#endif