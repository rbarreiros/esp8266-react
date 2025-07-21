#include "RemoteStateService.h"
#include <AsyncTimer.h>
#include <SettingValue.h>

namespace SettingValue {
    String getUniqueId();
}

extern AsyncTimer Timer;

RemoteStateService::RemoteStateService(
    AsyncWebServer* server, 
    SecurityManager* security,
    espMqttClientAsync* mqttClient, 
    RfRemoteController* rfctrl,
    RemoteSettingsService* remoteSettings,
    GarageStateService* garageService)
    :
    m_mqttClient{mqttClient},
    m_websocket
    {
        RemoteState::read,
        RemoteState::update,
        this,
        server,
        REMOTE_STATE_SOCKET_PATH,
        security,
        AuthenticationPredicates::IS_AUTHENTICATED
    },
    m_rfctrl{rfctrl},
    m_remoteSettings{remoteSettings},
    m_garage{garageService},
    m_wasPairing{false},
    m_mqttConsolidatedPubSub{RemoteStateService::consolidatedRead, 
                             RemoteStateService::consolidatedUpdate, this, m_mqttClient},
    m_mqttPairingPubSub{&RemoteState::haPairingRead, &RemoteState::haPairingUpdate, this, m_mqttClient},
    m_mqttRemotePubSub{&RemoteState::haRead, &RemoteState::haUpdate, this, m_mqttClient}
{
    m_rfctrl->addCallback(std::bind(&RemoteStateService::onRemoteReceived, this, std::placeholders::_1, std::placeholders::_2));

    m_mqttClient->onConnect(
        std::bind(&RemoteStateService::registerConfig, this));

    addUpdateHandler([&](const String& originId) { onStateUpdate(); }, false);
}

void RemoteStateService::begin()
{
    _state.isPairing = false;
    _state.isValid = false;
    _state.error = "";
}

void RemoteStateService::onRemoteReceived(RemotePacket packet, RemoteSerial serial)
{
    bool isValid = false;
    Remote rem = this->m_remoteSettings->getRemote(packet, serial);
    if(rem.button > 0 && rem.serial.toString().length() > 0)
        isValid = true;

    // Are we pairing ?
    if(_state.isPairing)
    {
        if(!m_remoteSettings->addRemote(packet, serial))
        {
            update([&](RemoteState& state) {
                state.rem = rem;
                state.isValid = isValid;
                state.error = "Remote already exists.";
                state.isPairing = false; // Cancel pairing anyway.
                return StateUpdateResult::CHANGED;
            }, "remotestate");
        } else {
            update([&](RemoteState& state) {
                state.rem = rem;
                state.isValid = isValid;
                state.error = "";
                state.isPairing = false;
                return StateUpdateResult::CHANGED;
            }, "remotestate");
        }
        Serial.println("Received packet while pairing, stopped pairing.");
    }
    else
    {
        if(rem.button == 0)
        {
            rem.button = packet.button;
            rem.description = "";
            rem.serial = RfRemoteController::getSerial(packet);
        }

        update([&](RemoteState& state) {
            state.rem = rem;
            state.isValid = isValid;
            return StateUpdateResult::CHANGED;
        }, "remotestate");

        if(isValid)
        {
            m_garage->update([&](GarageState& state) {
                state.relayOn = true;
                return StateUpdateResult::CHANGED;
            }, "remotestate");
        }
    }
}

void RemoteStateService::onStateUpdate()
{
    // Pairing started on the web or mqtt?
    if(_state.isPairing && !m_wasPairing)
    {
        Serial.println("Remote Pairing started");

        // Start timeout timer
        Timer.setTimeout([this]() {
            update([&](RemoteState& state) {
                state.isPairing = false;
                state.error = "Pairing timed out.";
                return StateUpdateResult::CHANGED;
            }, "timer");

            m_wasPairing = false;
        }, m_remoteSettings->getPairingTimeout());

        m_wasPairing = true;
    }

    // Canceled
    if(!_state.isPairing && m_wasPairing)
    {
        Serial.println("Remote Pairing canceled");
        m_wasPairing = false;
    }
}

void RemoteStateService::getDevice(JsonObject& dev)
{
    dev["ids"][0] = SettingValue::getUniqueId();
    dev["name"] = "Buttler Remote Bridge";  // Shouldn't be configurable ? header or web ?
    dev["sw"] = ESP.getSdkVersion();     // should be this firmware version, for
                                         // now will do.
    dev["mf"] = "Buttler Services LDA";  // jest, should be configurable ? :D
    dev["mdl"] = "Garage Remote Manager";
    dev["sn"] = dev["ids"][0];
    dev["cu"] = "http://" + WiFi.localIP().toString();  // we should check if it's connected.... no?
}

void RemoteStateService::registerDeviceTrigger()
{
    JsonDocument json;
    String payload;
    String uniqueId = SettingValue::getUniqueId();

    JsonObject dev = json["device"].to<JsonObject>();
    getDevice(dev);    

    json["~"] = "homeassistant/device_automation/remotes_" + uniqueId + "/remote";
    json["automation_type"] = "trigger";
    json["type"] = "action";
    json["subtype"] = "turn_on";
    json["name"] = "Remote Bridge";
    json["uniq_id"] = "buttler_remotes_" + uniqueId;
    json["topic"] = "~/action";

    serializeJson(json, payload);

    String config = json["~"].as<String>() + "/config";
    m_mqttClient->publish(config.c_str(), 0, false, payload.c_str());

    String pubTopic = json["~"].as<String>() + "/action";
    m_mqttRemotePubSub.configureTopics(pubTopic, "");
}

void RemoteStateService::registerPairingSwitch()
{
    JsonDocument json;
    String configTopic, subTopic, pubTopic, payload;
    String uniqueId = SettingValue::getUniqueId();

    //----- Entity
    JsonObject dev = json["device"].to<JsonObject>();
    getDevice(dev);

    json["~"] = "homeassistant/switch/remote_pairing_" + uniqueId;
    json["name"] = "Garage Remote Pairing";
    json["uniq_id"] = "buttler_pairing_" + uniqueId;
    json["cmd_t"] = "~/set";
    json["stat_t"] = "~/state";
    json["val_tpl"] = "{{ value_json.state }}";
    json["pl_on"] = "{\"state\": \"" + String(ON_STATE) + "\"}";
    json["pl_off"] = "{\"state\": \"" + String(OFF_STATE) + "\"}";
    json["stat_on"] = String(ON_STATE);
    json["stat_off"] = String(OFF_STATE);

    configTopic = json["~"].as<String>() + "/config";
    subTopic = json["~"].as<String>() + "/set";
    pubTopic = json["~"].as<String>() + "/state";

    serializeJson(json, payload);

    m_mqttClient->publish(configTopic.c_str(), 0, false, payload.c_str());
    m_mqttPairingPubSub.configureTopics(pubTopic, subTopic);
}

void RemoteStateService::registerConsolidatedTopic()
{
    JsonDocument json;
    String configTopic, subTopic, pubTopic, payload;
    String uniqueId = SettingValue::getUniqueId();

    // Entity device info
    JsonObject dev = json["device"].to<JsonObject>();
    getDevice(dev);

    // Consolidated remote topic - all state in one efficient message
    json["~"] = "homeassistant/garage/remote_" + uniqueId;
    json["name"] = "Garage Remote System";
    json["uniq_id"] = "buttler_remote_" + uniqueId;
    json["cmd_t"] = "~/cmd";
    json["stat_t"] = "~/state";
    json["val_tpl"] = "{{ value_json }}";
    json["json_attr_t"] = "~/state";
    json["device_class"] = "remote";

    configTopic = json["~"].as<String>() + "/config";
    subTopic = json["~"].as<String>() + "/cmd";
    pubTopic = json["~"].as<String>() + "/state";

    serializeJson(json, payload);

    m_mqttClient->publish(configTopic.c_str(), 0, false, payload.c_str());
    m_mqttConsolidatedPubSub.configureTopics(pubTopic, subTopic);
}

// Consolidated MQTT payload readers
void RemoteStateService::consolidatedRead(RemoteState& state, JsonObject& root)
{
    // Compact consolidated payload - all remote data in one message
    JsonObject remote = root["rem"].to<JsonObject>();
    remote["id"] = state.rem.id();
    remote["btn"] = state.rem.button;
    remote["ser"] = state.rem.getSerial();
    remote["desc"] = state.rem.description;
    remote["valid"] = state.isValid;
    remote["pair"] = state.isPairing;
    
    if (!state.error.isEmpty()) {
        remote["err"] = state.error;
    }
    
    // Additional metadata
    root["ts"] = millis();
    root["v"] = 1; // Version for future compatibility
}

StateUpdateResult RemoteStateService::consolidatedUpdate(JsonObject& root, RemoteState& state)
{
    bool changed = false;
    
    // Handle pairing commands
    if (root["pair"].is<bool>()) {
        bool newPairing = root["pair"];
        if (state.isPairing != newPairing) {
            state.isPairing = newPairing;
            changed = true;
        }
    }
    
    // Handle remote commands
    if (root["cmd"].is<String>()) {
        String cmd = root["cmd"];
        if (cmd == "start_pairing") {
            state.isPairing = true;
            changed = true;
        } else if (cmd == "stop_pairing") {
            state.isPairing = false;
            changed = true;
        }
    }
    
    return changed ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
}

void RemoteStateService::registerConfig()
{
    // Register consolidated topic first (new efficient approach)
    registerConsolidatedTopic();
    
    // Keep individual topics for backward compatibility
    registerDeviceTrigger();
    registerPairingSwitch();
}