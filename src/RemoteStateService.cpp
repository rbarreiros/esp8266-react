#include "RemoteStateService.h"
#include <AsyncTimer.h>

extern AsyncTimer Timer;

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
    m_garage{garageService},
    m_wasPairing{false}
{
    m_rfctrl->addCallback(std::bind(&RemoteStateService::onRemoteReceived, this, std::placeholders::_1, std::placeholders::_2));
    addUpdateHandler([&](const char* originId) { onStateUpdate(); }, false);
}

void RemoteStateService::begin()
{
    _state.isPairing = false;
    _state.isValid = false;
    _state.error[REMOTE_MAX_ERROR_SIZE] = '\0';
}

void RemoteStateService::onRemoteReceived(RemotePacket packet, RemoteSerial serial)
{
    bool isValid = false;
    Remote rem = this->m_remoteSettings->getRemote(packet, serial);
    if(rem.button > 0 && strlen(serial.toString()) > 0)
        isValid = true;

    Serial.printf("Received Remote packet %s\r\n", rem.id());

    // Are we pairing ?
    if(_state.isPairing)
    {
        if(!m_remoteSettings->addRemote(packet, serial))
        {
            update([&](RemoteState& state) {
                state.rem = rem;
                state.isValid = isValid;
                size_t len = snprintf(state.error, REMOTE_MAX_ERROR_SIZE, "Remote already exists.");
                state.error[len == REMOTE_MAX_DESCRIPTION_SIZE ? len - 1 : len] = '\0';
                return StateUpdateResult::CHANGED;
            }, "remotestate");
        } else {
            update([&](RemoteState& state) {
                state.rem = rem;
                state.isValid = isValid;
                state.error[0] = '\0';
                state.isPairing = false;
                return StateUpdateResult::CHANGED;
            }, "remotestate");
        }
    }
    else
    {
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
                size_t len = snprintf(state.error, REMOTE_MAX_ERROR_SIZE, "Pairing timed out.");
                state.error[len == REMOTE_MAX_DESCRIPTION_SIZE ? len - 1 : len] = '\0';
                return StateUpdateResult::CHANGED;
            }, "timer");

            m_wasPairing = false;
        }, m_remoteSettings->getPairingTimeout());

        m_wasPairing = true;
    }
}