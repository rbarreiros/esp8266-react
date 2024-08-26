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

    Serial.printf("Received Remote packet %s\r\n", rem.id().c_str());

    // Are we pairing ?
    if(_state.isPairing)
    {
        if(!m_remoteSettings->addRemote(packet, serial))
        {
            update([&](RemoteState& state) {
                state.rem = rem;
                state.isValid = isValid;
                state.error = "Remote already exists.";
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
    Serial.printf("Got config update: Pairing: %d\r\n", _state.isPairing);

    // Pairing started on the web ?
    if(_state.isPairing && !m_wasPairing)
    {
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
}

void RemoteStateService::registerConfig()
{
    JsonDocument json;
    String payload;
    String uniqueId = SettingValue::getUniqueId();

    Serial.println("Registering Remote MQTT Stuff");

    JsonObject dev = json["device"].to<JsonObject>();
    
    dev["ids"][0] = SettingValue::getUniqueId();
    dev["name"] = "Buttler Remote Bridge";  // Shouldn't be configurable ? header or web ?
    dev["sw"] = ESP.getSdkVersion();     // should be this firmware version, for
                                         // now will do.
    dev["mf"] = "Buttler Services LDA";  // jest, should be configurable ? :D
    dev["mdl"] = "Garage Remote Manager";
    dev["sn"] = dev["ids"][0];
    dev["cu"] = "http://" + WiFi.localIP().toString();  // we should check if it's connected.... no?

    json["~"] = "homeassistant/device_automation/remotes_" + uniqueId + "/remote";
    json["automation_type"] = "trigger";
    json["type"] = "action";
    json["subtype"] = "turn_on";
    json["name"] = "Remote Bridge";
    json["uniq_id"] = "buttler_remotes_" + uniqueId;
    json["topic"] = "~/action";
    //json["val_tpl"] = "{{ value_json.state }}";

    serializeJson(json, payload);

    String config = json["~"].as<String>() + "/config";
    m_mqttClient->publish(config.c_str(), 0, false, payload.c_str());

    String pubTopic = json["~"].as<String>() + "/action";
    m_mqttRemotePubSub.configureTopics(pubTopic, "");
}