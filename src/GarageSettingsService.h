#ifndef _GARAGESETTINGSSERVICE_H_
#define _GARAGESETTINGSSERVICE_H_

#include <StatefulService.h>
#include <HttpEndpoint.h>
#include <FSPersistence.h>
#include <ESPAsyncWebServer.h>
#include "GarageStateService.h"

#define DEFAULT_RELAY_AUTO_OFF true
#define DEFAULT_RELAY_TIMER 1000

#define GARAGE_SETTINGS_FILE "/config/garageSettings.json"
#define GARAGE_SETTINGS_ENDPOINT_PATH "/rest/garageSettings"

// HA relay_on_timer as a number
// HA relay_auto_off as a switch

class GarageSettings
{
public:
    bool relayAutoOff;
    unsigned long relayOnTimer;

    static void read(GarageSettings& settings, JsonObject& root)
    {
        root["relay_auto_off"] = settings.relayAutoOff;
        root["relay_on_timer"] = settings.relayOnTimer;
    }

    static StateUpdateResult update(JsonObject& root, GarageSettings& settings)
    {
        bool newRelayAutoOff = root["relay_auto_off"] | DEFAULT_RELAY_AUTO_OFF;
        unsigned long newRelayOnTimer = root["relay_on_timer"] | DEFAULT_RELAY_TIMER;
        StateUpdateResult change = StateUpdateResult::UNCHANGED;

        if(newRelayAutoOff != settings.relayAutoOff)
        {
            settings.relayAutoOff = newRelayAutoOff;
            change = StateUpdateResult::CHANGED;
        }

        if(newRelayOnTimer != settings.relayOnTimer)
        {
            settings.relayOnTimer = newRelayOnTimer;
            change = StateUpdateResult::CHANGED;
        }

        return change;
    }

    static void haRead(GarageSettings& settings, JsonObject& root)
    {
        
    }


    static StateUpdateResult haUpdate(JsonObject& root, GarageSettings& settings)
    {
        return StateUpdateResult::UNCHANGED;
    }
};

class GarageSettingsService : public StatefulService<GarageSettings>
{
public:
    GarageSettingsService(AsyncWebServer *server, 
                          SecurityManager *security, 
                          FS *fs,
                          GarageStateService* m_garageStateService);
    void begin();

private:
    HttpEndpoint<GarageSettings>    m_httpEndpoint;
    FSPersistence<GarageSettings>   m_fs;
    GarageStateService*             m_garageStateService;

    void onConfigUpdate();
};

#endif