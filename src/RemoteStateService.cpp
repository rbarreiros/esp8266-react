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