#include "RemoteStateService.h"


RemoteStateService::RemoteStateService(
    AsyncWebServer* server, 
    SecurityManager* security, 
    RfRemoteController* rfctrl,
    RemoteSettingsService* remoteSettings,
    GarageStateService* garageService)
    :
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
    m_garage{garageService}
{
    m_rfctrl->addCallback(std::bind(&RemoteStateService::onRemoteReceived, this, std::placeholders::_1, std::placeholders::_2));
}

void RemoteStateService::begin()
{
    _state.isPairing = false;
    _state.isValid = false;
    _state.description = "";
    _state.rxPacket = {0};
    _state.rxSerial = {0};
}

void RemoteStateService::onRemoteReceived(RemotePacket packet, RemoteSerial serial)
{
    bool isValid = this->m_remoteSettings->isValid(packet, serial);

    update([&](RemoteState& state) {
        state.rxPacket = packet;
        state.rxSerial = serial;
        state.description = this->m_remoteSettings->getDescription(packet, serial);
        state.isValid = isValid;
        // Do not mess with isPairing, handled somewhere else
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