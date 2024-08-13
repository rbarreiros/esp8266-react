#include "GarageStateService.h"
#include <AsyncTimer.h>

extern AsyncTimer Timer;

void GarageState::read(GarageState& settings, JsonObject& root)
{
    root["relay_on"] = settings.relayOn;
    root["endstop_closed"] = settings.endstopClosed;
    root["endstop_open"] = settings.endstopOpen;
    root["status"] = settings.status;
}

StateUpdateResult GarageState::update(JsonObject& root, GarageState& garageState)
{
    StateUpdateResult changed = StateUpdateResult::UNCHANGED;

    bool relayNewState = root["relay_on"] | DEFAULT_RELAY_STATE;

    if(garageState.relayOn != relayNewState)
    {
        garageState.relayOn = relayNewState;
        changed = StateUpdateResult::CHANGED;
    }

    return changed;
}

void GarageState::haRead(GarageState& settings, JsonObject& root)
{
}

StateUpdateResult GarageState::haUpdate(JsonObject& root, GarageState& GarageState)
{
   return StateUpdateResult::UNCHANGED;
}

///

GarageStateService::GarageStateService(AsyncWebServer* server,
                                     SecurityManager* securityManager)
    :
    m_webSocket
    {
        GarageState::read,
        GarageState::update,
        this,
        server,
        GARAGE_STATE_SOCKET_PATH,
        securityManager,
        AuthenticationPredicates::IS_AUTHENTICATED
    }
{
    // Configure MQTT Callback
    // m_mqttClient->onConnect(std::bind(&GarageStateService::registerConfig, this));

    // Update handler
    //m_garageMqttSettingsService->addUpdateHandler([&](const String& originId) { registerConfig(); }, false);

    // settings service update handler
    addUpdateHandler([&](const char* originId) { onConfigUpdate(); }, false);
}

void GarageStateService::begin()
{
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

/*
void GarageStateService::registerConfig()
{
    if(!m_mqttClient->connected())
        return;
    
    String configTopic;
    String subTopic;
    String pubTopic;

}
*/

void GarageStateService::onConfigUpdate()
{
    digitalWrite(RELAY_PIN, _state.relayOn ? RELAY_ON : RELAY_OFF);

    if(_state.relayOn && m_relayAutoOff)
    {
        Timer.setTimeout([this]() {
            update([&](GarageState& state){
                state.relayOn = false;
                return StateUpdateResult::CHANGED;
            }, "timer");
        }, m_relayOnTimer);
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


    // When state changes, update websocket
    if(_state.status != m_lastEsState)
    {
        _state.status = m_lastEsState;

        update([&](GarageState& state)
        {
            state.status = m_lastEsState;
            state.endstopOpen = _state.endstopOpen;
            state.endstopClosed = _state.endstopClosed;
            return StateUpdateResult::CHANGED;
        }, "stateservice");

    }
}