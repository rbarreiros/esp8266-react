#ifndef _GARAGEMQTTSETTINGSERVICE_H_
#define _GARAGEMQTTSETTINGSERVICE_H_

#include <HttpEndpoint.h>
#include <StatefulService.h>
#include <FSPersistence.h>

#define GARAGE_SETTINGS_FILE "/config/brokerSettings.json"
#define GARAGE_SETTINGS_PATH "/rest/brokerSettings"

#define MQTT_PATH_SIZE 128
#define NAME_SIZE 64
#define UNIQUE_ID_SIZE 64

class GarageMqttSettings
{
public:
    char mqttPath[MQTT_PATH_SIZE];
    char name[NAME_SIZE];
    char uniqueId[UNIQUE_ID_SIZE];

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