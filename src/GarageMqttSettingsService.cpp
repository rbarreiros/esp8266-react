#include "GarageMqttSettingsService.h"
#include <SettingValue.h>

void GarageMqttSettings::read(GarageMqttSettings& settings, JsonObject& root) 
{
    root["mqtt_path"] = settings.mqttPath;
    root["name"] = settings.name;
    root["unique_id"] = settings.uniqueId;
}

StateUpdateResult GarageMqttSettings::update(JsonObject& root, GarageMqttSettings& settings) 
{
    settings.mqttPath = root["mqtt_path"] | SettingValue::format("homeassistant/switch/#{unique_id}");
    settings.name = root["name"] | SettingValue::format("light-#{unique_id}");
    settings.uniqueId = root["unique_id"] | SettingValue::format("light-#{unique_id}");

    return StateUpdateResult::CHANGED;
}

GarageMqttSettingsService::GarageMqttSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager) 
    :
    m_httpEndpoint
    {
        GarageMqttSettings::read,
        GarageMqttSettings::update,
        this,
        server,
        GARAGE_SETTINGS_PATH,
        securityManager,
        AuthenticationPredicates::IS_AUTHENTICATED
    },
    m_fsPersistence
    {
        GarageMqttSettings::read, 
        GarageMqttSettings::update, 
        this, 
        fs, 
        GARAGE_SETTINGS_FILE
    } 
{}

void GarageMqttSettingsService::begin() 
{
  m_fsPersistence.readFromFS();
}
