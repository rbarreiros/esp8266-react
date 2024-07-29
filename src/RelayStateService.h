#ifndef _RELAYSTATESERVICE_H_
#define _RELAYSTATESERVICE_H_

#include <HttpEndpoint.h>
#include <StatefulService.h>
#include <MqttPubSub.h>
#include "RelayMqttSettingsService.h"

#define RELAY_SETTINGS_ENDPOINT_PATH "/rest/relayState"

#define RELAY_PIN 0
#define DEFAULT_RELAY_STATE false
#define OFF_STATE "OFF"
#define ON_STATE  "ON"
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

#define RELAY_AUTO_OFF true
#define RELAY_AUTO_OFF_TIME_MS  1000

class RelayState
{
public:
    bool relayOn;

    static void read(RelayState& settings, JsonObject& root);
    static StateUpdateResult update(JsonObject& root, RelayState& relayState);
    static void haRead(RelayState& settings, JsonObject& root);
    static StateUpdateResult haUpdate(JsonObject& root, RelayState& relayState);
};

class RelayStateService : public StatefulService<RelayState>
{
public:
    RelayStateService(AsyncWebServer* server,
                        SecurityManager* securityManager,
                        espMqttClientAsync* mqttClient,
                        RelayMqttSettingsService* relayMqttSettingsService);
    void begin();

private:
    HttpEndpoint<RelayState>    m_httpEndpoint;
    MqttPubSub<RelayState>      m_mqttPubSub;
    espMqttClientAsync*            m_mqttClient;
    RelayMqttSettingsService*   m_relayMqttSettingsService;

    void registerConfig();
    void onConfigUpdate();
};

#endif