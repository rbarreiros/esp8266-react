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
    bool changed = false;

    /*
    const char* mqttPath = root["mqtt_path"].as<const char*>() | const_cast<const char*>(SettingValue::format("homeassistant/switch/#{unique_id}"));
    const char* name = root["name"].as<const char*>() | const_cast<const char*>(SettingValue::format("light-#{unique_id}"));
    const char* uniqueId = root["unique_id"].as<const char*>() | const_cast<const char*>(SettingValue::format("light-#{unique_id}"));

    if (strcmp(settings.mqttPath, mqttPath) != 0) {
        strncpy(settings.mqttPath, mqttPath, MQTT_PATH_SIZE - 1);
        settings.mqttPath[MQTT_PATH_SIZE - 1] = '\0';
        changed = true;
    }

    if (strcmp(settings.name, name) != 0) {
        strncpy(settings.name, name, NAME_SIZE - 1);
        settings.name[NAME_SIZE - 1] = '\0';
        changed = true;
    }

    if (strcmp(settings.uniqueId, uniqueId) != 0) {
        strncpy(settings.uniqueId, uniqueId, UNIQUE_ID_SIZE - 1);
        settings.uniqueId[UNIQUE_ID_SIZE - 1] = '\0';
        changed = true;
    }
    */
    return changed ? StateUpdateResult::CHANGED : StateUpdateResult::UNCHANGED;
}

GarageMqttSettingsService::GarageMqttSettingsService(AsyncWebServer* server, FS* fs, SecurityManager* securityManager) 
    :
    m_httpEndpoint(GarageMqttSettings::read, GarageMqttSettings::update, this, server, GARAGE_SETTINGS_PATH, securityManager, AuthenticationPredicates::IS_AUTHENTICATED),
    m_fsPersistence(GarageMqttSettings::read, GarageMqttSettings::update, this, fs, GARAGE_SETTINGS_FILE)
{}

void GarageMqttSettingsService::begin() 
{
    m_fsPersistence.readFromFS();
}