#include "RelayStateService.h"
#include <AsyncTimer.h>

extern AsyncTimer Timer;

void RelayState::read(RelayState& settings, JsonObject& root)
{
    root["relay_on"] = settings.relayOn;
}

StateUpdateResult RelayState::update(JsonObject& root, RelayState& relayState)
{
    bool newState = root["relay_on"] | DEFAULT_RELAY_STATE;

    if(relayState.relayOn != newState)
    {
        relayState.relayOn = newState;
        return StateUpdateResult::CHANGED;
    }

    return StateUpdateResult::UNCHANGED;
}

void RelayState::haRead(RelayState& settings, JsonObject& root)
{
    root["state"] = settings.relayOn ? ON_STATE : OFF_STATE;
}

StateUpdateResult RelayState::haUpdate(JsonObject& root, RelayState& relayState)
{
    String state = root["state"];

    bool newState = false;
    if(state.equalsIgnoreCase(ON_STATE))
        newState = true;
    else if(!state.equalsIgnoreCase(OFF_STATE))
        return StateUpdateResult::ERROR;

    if(relayState.relayOn != newState)
    {
        relayState.relayOn = newState;
        return StateUpdateResult::CHANGED;
    }

    return StateUpdateResult::UNCHANGED;    
}

///

RelayStateService::RelayStateService(AsyncWebServer* server,
                                     SecurityManager* securityManager,
                                     espMqttClientAsync* mqttClient,
                                     RelayMqttSettingsService* relayMqttSettingsService)
    :
    m_httpEndpoint {
        RelayState::read,
        RelayState::update,
        this,
        server,
        RELAY_SETTINGS_ENDPOINT_PATH,
        securityManager,
        AuthenticationPredicates::IS_AUTHENTICATED
    },
    m_mqttPubSub {
        RelayState::haRead,
        RelayState::haUpdate,
        this,
        mqttClient
    },
    m_mqttClient {mqttClient},
    m_relayMqttSettingsService {relayMqttSettingsService}
{
    // Configure relay Pin
    pinMode(RELAY_PIN, OUTPUT);

    // Configure MQTT Callback
    m_mqttClient->onConnect(std::bind(&RelayStateService::registerConfig, this));

    // Update handler
    m_relayMqttSettingsService->addUpdateHandler([&](const String& originId) { registerConfig(); }, false);

    // settings service update handler
    addUpdateHandler([&](const String& originId) { onConfigUpdate(); }, false);
}

void RelayStateService::begin()
{
    _state.relayOn = DEFAULT_RELAY_STATE;
    onConfigUpdate();
}

void RelayStateService::registerConfig()
{
    if(!m_mqttClient->connected())
        return;
    
    String configTopic;
    String subTopic;
    String pubTopic;

}

void RelayStateService::onConfigUpdate()
{
    digitalWrite(RELAY_PIN, _state.relayOn ? RELAY_ON : RELAY_OFF);


// TODO - Mover para configuração web
#if RELAY_AUTO_OFF
    if(_state.relayOn)
    {
        Timer.setTimeout([]() {
            digitalWrite(RELAY_PIN, RELAY_OFF);
        }, RELAY_AUTO_OFF_TIME_MS);
    }
#endif
}