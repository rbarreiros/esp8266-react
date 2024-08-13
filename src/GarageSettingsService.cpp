#include "GarageSettingsService.h"

GarageSettingsService::GarageSettingsService(AsyncWebServer *server, 
                                             SecurityManager *security, 
                                             FS *fs,
                                             GarageStateService* garageStateService)
    :
    m_httpEndpoint
    {
        GarageSettings::read,
        GarageSettings::update,
        this,
        server,
        GARAGE_SETTINGS_ENDPOINT_PATH,
        security,
        AuthenticationPredicates::IS_ADMIN
    },
    m_fs
    {
        GarageSettings::read,
        GarageSettings::update,
        this,
        fs,
        GARAGE_SETTINGS_FILE
    },
    m_garageStateService{garageStateService}
{
    addUpdateHandler([&](const char* originId) { onConfigUpdate(); });
}

void GarageSettingsService::begin()
{
    m_fs.readFromFS();

    // Update state service with saved config
    m_garageStateService->setRelayAutoOff(_state.relayAutoOff);
    m_garageStateService->setRelayOnTimer(_state.relayOnTimer);
}

void GarageSettingsService::onConfigUpdate()
{
    m_garageStateService->setRelayAutoOff(_state.relayAutoOff);
    m_garageStateService->setRelayOnTimer(_state.relayOnTimer);
}