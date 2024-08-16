#include "GarageStateService.h"

#include <AsyncTimer.h>

extern AsyncTimer Timer;

// SettingValue.cpp
namespace SettingValue {
    String getUniqueId();
}

const String GarageState::m_statusString[5] = {
    "Error",
    "Closed",
    "Opening",
    "Closing",
    "Open"
};

GarageStateService::GarageStateService(
    AsyncWebServer* server, SecurityManager* securityManager,
    espMqttClientAsync* mqttClient,
    GarageMqttSettingsService* garageMqttSettings)
    : m_webSocket{GarageState::read,
                  GarageState::update,
                  this,
                  server,
                  GARAGE_STATE_SOCKET_PATH,
                  securityManager,
                  AuthenticationPredicates::IS_AUTHENTICATED},
      m_mqttClient{mqttClient},
      m_garageMqttSettings{garageMqttSettings},
      m_mqttRelayPubSub{GarageState::haRelayRead, GarageState::haRelayUpdate,
                        this, m_mqttClient},
      m_mqttStatusPubSub{GarageState::haStatusRead,
                         GarageState::haDummyUpdate, this, m_mqttClient}
    /*
      m_mqttEndstopOpenPubSub{GarageState::haEndstopOpenRead,
                         GarageState::haDummyUpdate, this, m_mqttClient},
      m_mqttEndstopClosedPubSub{GarageState::haEndstopClosedRead,
                         GarageState::haDummyUpdate, this, m_mqttClient}
                         */
{
    // Configure MQTT Callback
    m_mqttClient->onConnect(
        std::bind(&GarageStateService::registerConfig, this));

    // Update handler on MQTT connection
    m_garageMqttSettings->addUpdateHandler(
        [&](const String& originId) { registerConfig(); }, false);

    // settings service update handler
    addUpdateHandler([&](const String& originId) { onConfigUpdate(); }, false);
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

void GarageStateService::loop() { updateEndstops(); }

void GarageStateService::registerConfig()
{
    if (!m_mqttClient->connected()) return;

    JsonDocument json;
    String mqttPath;
    String name;
    String uniqueId;
    String configTopic, subTopic, pubTopic;

    //----- Entity
    JsonObject dev = json["device"].to<JsonObject>();
    dev["ids"][0] = SettingValue::getUniqueId();
    dev["name"] = "Buttler_Garage";  // Shouldn't be configurable ? header or web ?
    dev["sw"] = ESP.getSdkVersion();     // should be this firmware version, for
                                         // now will do.
    dev["mf"] = "Buttler Services LDA";  // jest, should be configurable ? :D
    dev["mdl"] = "Garage_Remote_Manager";
    dev["sn"] = dev["ids"][0];
    dev["cu"] = "http://" + WiFi.localIP().toString();  // we should check if it's connected.... no?

    // Get MQTT saved settings
    // TODO
    m_garageMqttSettings->read(
        [&](GarageMqttSettings& settings)
        {
            mqttPath = settings.mqttPath;
            name = settings.name;
            uniqueId = settings.uniqueId;
        });

    // TEMP
    uniqueId = SettingValue::getUniqueId();

    // Relay (switch)
    json["~"] = "homeassistant/switch/garage_door_" + uniqueId;
    json["name"] = "garage_door_" + uniqueId;
    json["uniq_id"] = "buttler_" + uniqueId;
    json["cmd_t"] = "~/set";
    json["stat_t"] = "~/state";
    json["schema"] = "json";
    json["val_tpl"] = "{{ value_json.state }}";
    json["pl_on"] = "{\"state\": \"" + String(ON_STATE) + "\"}";
    json["pl_off"] = "{\"state\": \"" + String(OFF_STATE) + "\"}";

    configTopic = json["~"] + "/config";
    subTopic = json["~"] + "/set";
    pubTopic = json["~"] + "/state";

    // Publish Relay
    String payload;
    Serial.printf("Sending config to %s \r\n", configTopic.c_str());
    serializeJson(json, payload);
    m_mqttClient->publish(configTopic.c_str(), 0, false, payload.c_str());
    
    m_mqttRelayPubSub.configureTopics(pubTopic, subTopic);


    Serial.printf("Memory:  Relay %p   ---   State %p\r\n", &m_mqttRelayPubSub, &m_mqttStatusPubSub);

    /*
    // Status now (sensor)
    json["~"] = "homeassistant/sensor/garage_status_" + uniqueId;
    json["name"] = "garage_status_" + uniqueId;
    json.remove("cmd_t");
    json["device_class"] = "enum";

    for(uint8_t i = 0; i < 5; i++)
        json["options"][i] = GarageState::m_statusString[i];

    configTopic = json["~"] + "/config";
    subTopic.clear();
    pubTopic = json["~"] + "/state";

    // publish Status
    Serial.printf("Sending config to %s \r\n", configTopic.c_str());
    serializeJson(json, payload);
    m_mqttClient->publish(configTopic.c_str(), 0, false, payload.c_str());
    m_mqttStatusPubSub.configureTopics(pubTopic, subTopic);

    // Endstops (Sensor)
    json["~"] = "homeassistant/binary_sensor/garage_endstop_open_" + uniqueId;
    json["name"] = "garage_endstop_open_" + uniqueId;
    json.remove("device_class");
    json.remove("options");

    configTopic = json["~"] + "/config";
    pubTopic = json["~"] + "/state";

    // publish Endstop Open
    Serial.printf("Sending config to %s \r\n", configTopic.c_str());
    serializeJson(json, payload);
    m_mqttClient->publish(configTopic.c_str(), 0, false, payload.c_str());
    m_mqttEndstopOpenPubSub.configureTopics(pubTopic, subTopic);

    // Endstops (Sensor)
    json["~"] = "homeassistant/binary_sensor/garage_endstop_closed_" + uniqueId;
    json["name"] = "garage_endstop_closed_" + uniqueId;

    configTopic = json["~"] + "/config";
    pubTopic = json["~"] + "/state";

    // publish Endstop Closed
    Serial.printf("Sending config to %s \r\n", configTopic.c_str());
    serializeJson(json, payload);
    m_mqttClient->publish(configTopic.c_str(), 0, false, payload.c_str());
    m_mqttEndstopClosedPubSub.configureTopics(pubTopic, subTopic);
    */
}

void GarageStateService::onConfigUpdate()
{
    digitalWrite(RELAY_PIN, _state.relayOn ? RELAY_ON : RELAY_OFF);

    if (_state.relayOn && m_relayAutoOff)
    {
        Timer.setTimeout(
            [this]()
            {
                update(
                    [&](GarageState& state)
                    {
                        state.relayOn = false;
                        return StateUpdateResult::CHANGED;
                    },
                    "timer");
            },
            m_relayOnTimer);
    }
}

void GarageStateService::updateEndstops()
{
    // check current endstop status
    _state.endstopOpen = digitalRead(ENDSTOP_OPEN_PIN) == ENDSTOP_OPEN_ON;
    _state.endstopClosed = digitalRead(ENDSTOP_CLOSED_PIN) == ENDSTOP_CLOSED_ON;

    // Error, should never happen....
    if (_state.endstopOpen && _state.endstopClosed)
    {
        m_lastEsState = GarageState::STATUS_ERROR;
    }
    // Gate open
    else if (_state.endstopOpen && !_state.endstopClosed)
    {
        m_lastEsState = GarageState::STATUS_OPEN;
    }
    else if (!_state.endstopOpen && _state.endstopClosed)
    {
        m_lastEsState = GarageState::STATUS_CLOSED;
    }
    else if (!_state.endstopOpen && !_state.endstopClosed)
    {
        // midway opening
        if (m_lastEsState == GarageState::STATUS_CLOSED)
            m_lastEsState = GarageState::STATUS_OPENING;
        else
            // midway closing
            if (m_lastEsState == GarageState::STATUS_OPEN)
                m_lastEsState = GarageState::STATUS_CLOSING;

        // Any other status continues to stay in that status
        // untill endstops status change.
    }

    // When state changes, update websocket
    if (_state.status != m_lastEsState)
    {
        _state.status = m_lastEsState;

        update(
            [&](GarageState& state)
            {
                state.status = m_lastEsState;
                state.endstopOpen = _state.endstopOpen;
                state.endstopClosed = _state.endstopClosed;
                return StateUpdateResult::CHANGED;
            },
            "stateservice");
    }
}