#include "RelayMqttSettingsService.h"
#include <SettingValue.h>

void RelayMqttSettings::read(RelayMqttSettings& settings, JsonObject& root) 
{
    root["mqtt_path"] = settings.mqttPath;
    root["name"] = settings.name;
    root["unique_id"] = settings.uniqueId;
}

StateUpdateResult RelayMqttSettings::update(JsonObject& root, RelayMqttSettings& settings) 
{
    settings.mqttPath = root["mqtt_path"] | SettingValue::format("homeassistant/switch/#{unique_id}");
    settings.name = root["name"] | SettingValue::format("light-#{unique_id}");
    settings.uniqueId = root["unique_id"] | SettingValue::format("light-#{unique_id}");

    return StateUpdateResult::CHANGED;
}

RelayMqttSettingsService::RelayMqttSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager) 
    :
    m_httpEndpoint
    {
        RelayMqttSettings::read,
        RelayMqttSettings::update,
        this,
        server,
        RELAY_SETTINGS_PATH,
        securityManager,
        AuthenticationPredicates::IS_AUTHENTICATED
    },
    m_fsPersistence
    {
        RelayMqttSettings::read, 
        RelayMqttSettings::update, 
        this, 
        fs, 
        RELAY_SETTINGS_FILE
    } 
{}

void RelayMqttSettingsService::begin() 
{
  m_fsPersistence.readFromFS();
}
