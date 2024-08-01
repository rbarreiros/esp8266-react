#include "GarageStateService.h"
#include <AsyncTimer.h>

extern AsyncTimer Timer;

void GarageState::read(GarageState& settings, JsonObject& root)
{
    root["relay_on"] = settings.relayOn;
    root["relay_auto_off"] = settings.relayAutoOff;
    root["relay_on_timer"] = settings.relayOnTimer;
    root["endstop_closed"] = settings.endstopClosed;
    root["endstop_open"] = settings.endstopOpen;
    root["status"] = settings.status;
}

StateUpdateResult GarageState::update(JsonObject& root, GarageState& garageState)
{
    StateUpdateResult changed = StateUpdateResult::UNCHANGED;

    bool relayNewState = root["relay_on"] | DEFAULT_RELAY_STATE;
    bool relayOffNewState = root["relay_auto_off"] | DEFAULT_RELAY_AUTO_OFF;
    unsigned long relayTimerNewState = root["relay_on_timer"] | DEFAULT_RELAY_TIMER;

    if(garageState.relayOn != relayNewState)
    {
        garageState.relayOn = relayNewState;
        changed = StateUpdateResult::CHANGED;
    }

    if(garageState.relayAutoOff != relayOffNewState)
    {
        garageState.relayAutoOff = relayOffNewState;
        changed = StateUpdateResult::CHANGED;
    }

    if(garageState.relayOnTimer != relayTimerNewState)
    {
        garageState.relayOnTimer = relayTimerNewState;
        changed = StateUpdateResult::CHANGED;
    }

    return changed;
}

void GarageState::haRead(GarageState& settings, JsonObject& root)
{
    //root["state"] = settings.relayOn ? ON_STATE : OFF_STATE;
}

StateUpdateResult GarageState::haUpdate(JsonObject& root, GarageState& GarageState)
{
    /*
    String state = root["state"];

    bool newState = false;
    if(state.equalsIgnoreCase(ON_STATE))
        newState = true;
    else if(!state.equalsIgnoreCase(OFF_STATE))
        return StateUpdateResult::ERROR;

    if(GarageState.relayOn != newState)
    {
        GarageState.relayOn = newState;
        return StateUpdateResult::CHANGED;
    }

    return StateUpdateResult::UNCHANGED;    
    */
   return StateUpdateResult::UNCHANGED;
}

///

GarageStateService::GarageStateService(AsyncWebServer* server,
                                     SecurityManager* securityManager,
                                     espMqttClientAsync* mqttClient,
                                     FS *fs,
                                     GarageMqttSettingsService* garageMqttSettingsService)
    :
    m_httpEndpoint {
        GarageState::read,
        GarageState::update,
        this,
        server,
        GARAGE_SETTINGS_ENDPOINT_PATH,
        securityManager,
        AuthenticationPredicates::IS_AUTHENTICATED
    },
    m_mqttPubSub {
        GarageState::haRead,
        GarageState::haUpdate,
        this,
        mqttClient
    },
    m_webSocket
    {
        GarageState::read,
        GarageState::update,
        this,
        server,
        GARAGE_SETTINGS_SOCKET_PATH,
        securityManager,
        AuthenticationPredicates::IS_AUTHENTICATED
    },
    m_mqttClient {mqttClient},
    m_fs 
    {
        GarageState::read,
        GarageState::update,
        this,
        fs,
        GARAGE_STATE_SETTINGS_FILE
    },
    m_garageMqttSettingsService {garageMqttSettingsService}
{

    // Configure MQTT Callback
    m_mqttClient->onConnect(std::bind(&GarageStateService::registerConfig, this));

    // Update handler
    m_garageMqttSettingsService->addUpdateHandler([&](const String& originId) { registerConfig(); }, false);

    // settings service update handler
    addUpdateHandler([&](const String& originId) { onConfigUpdate(); }, false);
}

void GarageStateService::begin()
{
    // Read from FS first, not before setting pinmode
    // to disable relay if it was save as ON
    m_fs.readFromFS();
    if(_state.relayOn)
        _state.relayOn = false;

    // Configure relay Pin
    pinMode(RELAY_PIN, OUTPUT);

    // Configure endstops
    pinMode(ENDSTOP_CLOSED_PIN, INPUT);
    pinMode(ENDSTOP_OPEN_PIN, INPUT);

    _state.relayOn = DEFAULT_RELAY_STATE;

    updateEndstops();
    onConfigUpdate();
}

void GarageStateService::loop()
{
    updateEndstops();
}

void GarageStateService::registerConfig()
{
    if(!m_mqttClient->connected())
        return;
    
    String configTopic;
    String subTopic;
    String pubTopic;

}

void GarageStateService::onConfigUpdate()
{

    digitalWrite(RELAY_PIN, _state.relayOn ? RELAY_ON : RELAY_OFF);

    if(_state.relayOn && _state.relayAutoOff)
    {
        Timer.setTimeout([this]() {
            update([&](GarageState& state){
                state.relayOn = false;
                return StateUpdateResult::CHANGED;
            }, "timer");
        }, _state.relayOnTimer);
    }
}

void GarageStateService::updateEndstops()
{
    // check current endstop status
    _state.endstopOpen = digitalRead(ENDSTOP_OPEN_PIN) == ENDSTOP_OPEN_ON;
    _state.endstopClosed = digitalRead(ENDSTOP_CLOSED_PIN) == ENDSTOP_CLOSED_ON;

    // Error, should never happen....
    if(_state.endstopOpen && _state.endstopClosed)
    {
        m_lastEsState = GarageState::STATUS_ERROR;
    }
    // Gate open
    else if(_state.endstopOpen && !_state.endstopClosed)
    {
        m_lastEsState = GarageState::STATUS_OPEN;
    }
    else if(!_state.endstopOpen && _state.endstopClosed)
    {
        m_lastEsState = GarageState::STATUS_CLOSED;
    }
    else if(!_state.endstopOpen && !_state.endstopClosed)
    {
        // midway opening
        if(m_lastEsState == GarageState::STATUS_CLOSED)
            m_lastEsState = GarageState::STATUS_OPENING;
        else 
        // midway closing
        if(m_lastEsState == GarageState::STATUS_OPEN)
            m_lastEsState = GarageState::STATUS_CLOSING;

        // Any other status continues to stay in that status
        // untill endstops status change.
    }

    _state.status = m_lastEsState;
}